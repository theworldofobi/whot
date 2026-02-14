#include "../include/Application.hpp"
#include "../include/Core/Player.hpp"
#include "../include/Game/ActionTypes.hpp"
#include "../include/Network/MessageProtocol.hpp"
#include "../include/AI/AIPlayer.hpp"
#include "../include/AI/DifficultyLevel.hpp"
#include "Utils/Logger.hpp"
#include "Utils/Random.hpp"
#include "Persistence/Database.hpp"
#include "Game/GameEngine.hpp"
#include "Game/GameState.hpp"
#include <thread>
#include <chrono>
#include <sstream>
#include <nlohmann/json.hpp>

namespace whot {

Application::Application(const ApplicationConfig& config)
    : config_(config)
{
}

Application::~Application() = default;

void Application::initialize()
{
    setupDatabase();
    setupWebSocketHandlers();
    setupHttpRoutes();
    loadExistingGames();
}

void Application::run()
{
    if (wsServer_) wsServer_->start();
    if (httpServer_) httpServer_->start();
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void Application::shutdown()
{
    if (wsServer_ && wsServer_->isRunning()) wsServer_->stop();
    if (httpServer_ && httpServer_->isRunning()) httpServer_->stop();
    if (database_ && database_->isConnected()) database_->disconnect();
}

std::string Application::createGame(const game::GameConfig& gameConfig)
{
    auto state = std::make_unique<game::GameState>(gameConfig);
    std::string gameId = state->getGameId();
    state->initialize();
    std::string gameCode;
    for (int attempt = 0; attempt < 20; ++attempt) {
        gameCode = utils::Random::getInstance().generateGameCode(6);
        if (getGameIdByCode(gameCode).empty())
            break;
    }
    if (gameCode.empty())
        gameCode = utils::Random::getInstance().generateGameCode(6);
    state->setGameCode(gameCode);
    auto engine = std::make_unique<game::GameEngine>(std::move(state));
    {
        std::lock_guard<std::mutex> lock(gamesMutex_);
        if (activeGames_.size() >= static_cast<size_t>(config_.maxGamesPerServer))
            return {};
        activeGames_[gameId] = std::move(engine);
    }
    game::GameEngine* eng = getGame(gameId);
    if (gameRepo_ && eng)
        gameRepo_->saveGame(*eng->getState());
    return gameId;
}

std::string Application::getGameIdByCode(const std::string& gameCode) const
{
    if (gameCode.empty()) return {};
    std::lock_guard<std::mutex> lock(gamesMutex_);
    for (const auto& [gid, engine] : activeGames_) {
        if (engine && engine->getState() && engine->getState()->getGameCode() == gameCode)
            return gid;
    }
    return {};
}

void Application::addBotsToGame(const std::string& gameId, int botCount)
{
    game::GameEngine* engine = getGame(gameId);
    if (!engine || !engine->getState() || botCount <= 0) return;
    game::GameState* state = engine->getState();
    int maxBots = static_cast<int>(state->getConfig().maxPlayers) - static_cast<int>(state->getPlayerCount());
    if (maxBots <= 0) return;
    if (botCount > maxBots) botCount = maxBots;
    for (int i = 0; i < botCount; ++i) {
        std::ostringstream idSs, nameSs;
        idSs << "bot-" << gameId << "-" << i;
        nameSs << "Bot " << (i + 1);
        auto bot = std::make_unique<core::Player>(idSs.str(), nameSs.str(), core::PlayerType::AI_EASY);
        state->addPlayer(std::move(bot));
    }
    if (gameRepo_)
        gameRepo_->saveGame(*state);
}

void Application::runBotTurnsIfNeeded(const std::string& gameId)
{
    const int maxIterations = 50;
    for (int iter = 0; iter < maxIterations; ++iter) {
        game::GameEngine* engine = getGame(gameId);
        if (!engine || !engine->getState()) break;
        game::GameState* state = engine->getState();
        if (state->getPhase() != game::GamePhase::IN_PROGRESS) break;
        core::Player* current = state->getCurrentPlayer();
        if (!current || current->getType() == core::PlayerType::HUMAN) break;
        ai::DifficultyLevel level = ai::DifficultyLevel::EASY;
        if (current->getType() == core::PlayerType::AI_MEDIUM) level = ai::DifficultyLevel::MEDIUM;
        else if (current->getType() == core::PlayerType::AI_HARD) level = ai::DifficultyLevel::HARD;
        ai::AIPlayer ai(current->getId(), current->getName(), level);
        game::GameAction action = ai.decideAction(*state);
        game::ActionResult result = engine->processAction(action);
        if (!result.success) break;
        broadcastGameState(gameId);
        state = engine->getState();
        if (state->getPhase() == game::GamePhase::ROUND_ENDED && !state->checkGameEnd()) {
            engine->startNewRound();
            broadcastGameState(gameId);
        }
    }
}

bool Application::joinGame(const std::string& gameId, const std::string& playerId,
                           const std::string& playerName)
{
    game::GameEngine* engine = getGame(gameId);
    if (!engine) return false;
    game::GameState* state = engine->getState();
    if (!state) return false;
    if (state->getPhase() != game::GamePhase::LOBBY) return false;
    if (state->getPlayer(playerId)) return true;  // already in game
    if (state->getPlayerCount() >= static_cast<size_t>(state->getConfig().maxPlayers))
        return false;
    std::string name = playerName.empty() ? playerId : playerName;
    auto player = std::make_unique<core::Player>(playerId, name, core::PlayerType::HUMAN);
    state->addPlayer(std::move(player));
    if (gameRepo_)
        gameRepo_->saveGame(*state);
    return true;
}

bool Application::leaveGame(const std::string& gameId, const std::string& playerId)
{
    game::GameEngine* engine = getGame(gameId);
    if (!engine) return false;
    engine->getState()->removePlayer(playerId);
    if (wsServer_ && wsServer_->getSessionManager()) {
        std::string sid = wsServer_->getSessionManager()->getSessionIdForPlayer(playerId);
        if (!sid.empty()) {
            wsServer_->getSessionManager()->setGameId(sid, "");
            wsServer_->getSessionManager()->setPlayerId(sid, "");
        }
    }
    broadcastGameState(gameId);
    return true;
}

void Application::removeGame(const std::string& gameId)
{
    std::lock_guard<std::mutex> lock(gamesMutex_);
    activeGames_.erase(gameId);
    if (gameRepo_) gameRepo_->deleteGame(gameId);
    if (wsServer_ && wsServer_->getSessionManager())
        wsServer_->getSessionManager()->removeAllSessionsForGame(gameId);
}

void Application::handleClientMessage(const std::string& sessionId,
                                      const network::Message& message)
{
    using network::MessageType;
    switch (message.type) {
        case MessageType::JOIN_GAME:
            handleJoinGame(sessionId, message);
            break;
        case MessageType::START_GAME:
            handleStartGame(sessionId, message);
            break;
        case MessageType::LEAVE_GAME:
            handleLeaveGame(sessionId, message);
            break;
        case MessageType::PLAY_CARD:
        case MessageType::DRAW_CARD:
        case MessageType::DECLARE_LAST_CARD:
        case MessageType::DECLARE_CHECK_UP:
        case MessageType::CHOOSE_SUIT:
            handleGameAction(sessionId, message);
            break;
        default:
            break;
    }
}

network::WebSocketServer* Application::getWebSocketServer()
{
    return wsServer_.get();
}

network::HttpServer* Application::getHttpServer()
{
    return httpServer_.get();
}

void Application::setupWebSocketHandlers()
{
    wsServer_ = std::make_unique<network::WebSocketServer>(config_.websocketPort);
    wsServer_->setMessageHandler([this](const std::string& sessionId, const network::Message& msg) {
        handleClientMessage(sessionId, msg);
    });
}

void Application::setupHttpRoutes()
{
    httpServer_ = std::make_unique<network::HttpServer>(config_.httpPort);
    httpServer_->setStaticRoot(config_.staticFilesPath);
    httpServer_->addRoute(network::HttpMethod::GET, "/api/games",
        [this](const network::HttpRequest& r) { return handleGetGames(r); });
    httpServer_->addRoute(network::HttpMethod::POST, "/api/games",
        [this](const network::HttpRequest& r) { return handleCreateGame(r); });
    httpServer_->addRoute(network::HttpMethod::GET, "/api/health",
        [](const network::HttpRequest&) {
            return network::HttpResponse::json(200, "{\"status\":\"ok\"}");
        });
    httpServer_->addPatternRoute(network::HttpMethod::GET, "/api/games/:id",
        [this](const network::HttpRequest& r) {
            auto it = r.pathParams.find("id");
            return it != r.pathParams.end() ? handleGetGame(it->second)
                : network::HttpResponse::notFound("");
        });
    httpServer_->addPatternRoute(network::HttpMethod::POST, "/api/games/:id/join",
        [this](const network::HttpRequest& r) {
            auto it = r.pathParams.find("id");
            return it != r.pathParams.end() ? handleJoinGameHttp(it->second, r)
                : network::HttpResponse::notFound("");
        });
    httpServer_->addRoute(network::HttpMethod::POST, "/api/games/join",
        [this](const network::HttpRequest& r) { return handleJoinByCode(r); });
}

void Application::setupDatabase()
{
    database_ = persistence::DatabaseFactory::create(config_.dbConfig);
    if (database_ && database_->connect()) {
        database_->initializeSchema();
        gameRepo_ = std::make_unique<persistence::GameRepository>(database_.get());
        playerRepo_ = std::make_unique<persistence::PlayerRepository>(database_.get());
    }
}

void Application::loadExistingGames()
{
    if (!gameRepo_) return;
    auto recs = gameRepo_->getActiveGames();
    std::lock_guard<std::mutex> lock(gamesMutex_);
    for (const auto& rec : recs) {
        auto state = gameRepo_->loadGame(rec.gameId);
        if (state.has_value()) {
            auto statePtr = std::make_unique<game::GameState>(std::move(state.value()));
            activeGames_[rec.gameId] = std::make_unique<game::GameEngine>(std::move(statePtr));
        }
    }
}

void Application::handleJoinGame(const std::string& sessionId,
                                  const network::Message& message)
{
    auto payload = network::JoinGamePayload::fromJson(message.payload);
    std::string gameId = payload.gameId.empty() ? message.gameId : payload.gameId;
    if (gameId.empty() && payload.gameCode.has_value() && !payload.gameCode->empty())
        gameId = getGameIdByCode(payload.gameCode.value());
    std::string playerId = message.playerId.empty()
        ? utils::Random::getInstance().generateUUID()
        : message.playerId;
    std::string playerName = payload.playerName.empty() ? "Player" : payload.playerName;
    if (gameId.empty()) return;
    if (joinGame(gameId, playerId, playerName)) {
        if (wsServer_ && wsServer_->getSessionManager()) {
            wsServer_->getSessionManager()->setGameId(sessionId, gameId);
            wsServer_->getSessionManager()->setPlayerId(sessionId, playerId);
        }
        broadcastGameState(gameId);
    }
}

void Application::handleLeaveGame(const std::string& sessionId,
                                   const network::Message& message)
{
    std::string gameId = message.gameId;
    std::string playerId = message.playerId;
    if (wsServer_ && wsServer_->getSessionManager()) {
        const auto* sess = wsServer_->getSessionManager()->getSession(sessionId);
        if (sess) {
            if (gameId.empty()) gameId = sess->gameId;
            if (playerId.empty()) playerId = sess->playerId;
        }
    }
    if (!gameId.empty() && !playerId.empty())
        leaveGame(gameId, playerId);
}

void Application::handleStartGame(const std::string& sessionId,
                                   const network::Message& message)
{
    std::string gameId = message.gameId;
    std::string playerId = message.playerId;
    if (wsServer_ && wsServer_->getSessionManager()) {
        const auto* sess = wsServer_->getSessionManager()->getSession(sessionId);
        if (sess) {
            if (gameId.empty()) gameId = sess->gameId;
            if (playerId.empty()) playerId = sess->playerId;
        }
    }
    game::GameEngine* engine = getGame(gameId);
    if (!engine || !engine->getState() || playerId.empty()) {
        if (wsServer_) {
            network::Message errMsg;
            errMsg.type = network::MessageType::ERROR;
            errMsg.gameId = gameId;
            errMsg.playerId = playerId;
            errMsg.payload = network::ErrorPayload{"START_GAME", "Game not found or not in session", std::nullopt}.toJson();
            wsServer_->sendMessage(sessionId, errMsg);
        }
        return;
    }
    game::GameState* state = engine->getState();
    if (state->getPhase() != game::GamePhase::LOBBY) {
        if (wsServer_) {
            network::Message errMsg;
            errMsg.type = network::MessageType::ERROR;
            errMsg.gameId = gameId;
            errMsg.playerId = playerId;
            errMsg.payload = network::ErrorPayload{"START_GAME", "Game already started", std::nullopt}.toJson();
            wsServer_->sendMessage(sessionId, errMsg);
        }
        return;
    }
    if (state->getCreatorPlayerId() != playerId) {
        if (wsServer_) {
            network::Message errMsg;
            errMsg.type = network::MessageType::ERROR;
            errMsg.gameId = gameId;
            errMsg.playerId = playerId;
            errMsg.payload = network::ErrorPayload{"START_GAME", "Only the creator can start the game", std::nullopt}.toJson();
            wsServer_->sendMessage(sessionId, errMsg);
        }
        return;
    }
    if (state->getPlayerCount() < static_cast<size_t>(state->getConfig().minPlayers)) {
        if (wsServer_) {
            network::Message errMsg;
            errMsg.type = network::MessageType::ERROR;
            errMsg.gameId = gameId;
            errMsg.playerId = playerId;
            errMsg.payload = network::ErrorPayload{"START_GAME", "Not enough players to start", std::nullopt}.toJson();
            wsServer_->sendMessage(sessionId, errMsg);
        }
        return;
    }
    engine->startGame();
    engine->startNewRound();
    if (gameRepo_)
        gameRepo_->saveGame(*state);
    broadcastGameState(gameId);
    runBotTurnsIfNeeded(gameId);
}

void Application::handleGameAction(const std::string& sessionId,
                                   const network::Message& message)
{
    std::string gameId = message.gameId;
    std::string playerId = message.playerId;
    if (wsServer_ && wsServer_->getSessionManager()) {
        const auto* sess = wsServer_->getSessionManager()->getSession(sessionId);
        if (sess) {
            if (gameId.empty()) gameId = sess->gameId;
            if (playerId.empty()) playerId = sess->playerId;
        }
    }
    game::GameEngine* engine = getGame(gameId);
    if (!engine || playerId.empty()) return;
    game::GameAction action;
    action.playerId = playerId;
    action.type = game::ActionType::FORFEIT_TURN;
    using network::MessageType;
    switch (message.type) {
        case MessageType::PLAY_CARD: {
            auto pl = network::PlayCardPayload::fromJson(message.payload);
            action.type = game::ActionType::PLAY_CARD;
            action.cardIndex = pl.cardIndex;
            action.chosenSuit = pl.chosenSuit;
            break;
        }
        case MessageType::DRAW_CARD:
            action.type = game::ActionType::DRAW_CARD;
            break;
        case MessageType::DECLARE_LAST_CARD:
            action.type = game::ActionType::DECLARE_LAST_CARD;
            break;
        case MessageType::DECLARE_CHECK_UP:
            action.type = game::ActionType::DECLARE_CHECK_UP;
            break;
        case MessageType::CHOOSE_SUIT:
            action.type = game::ActionType::CHOOSE_SUIT;
            action.chosenSuit = network::PlayCardPayload::fromJson(message.payload).chosenSuit;
            break;
        default:
            return;
    }
    game::ActionResult result = engine->processAction(action);
    if (!result.success) {
        nlohmann::json errPayload;
        errPayload["message"] = result.message;
        errPayload["errorCode"] = "invalid_action";
        network::Message errMsg;
        errMsg.type = network::MessageType::ERROR;
        errMsg.gameId = gameId;
        errMsg.playerId = playerId;
        errMsg.payload = errPayload.dump();
        errMsg.timestamp = static_cast<uint64_t>(
            std::chrono::system_clock::now().time_since_epoch().count());
        wsServer_->sendMessage(sessionId, errMsg);
        return;
    }
    broadcastGameState(gameId);
    game::GameState* st = engine->getState();
    if (st && st->getPhase() == game::GamePhase::ROUND_ENDED && !st->checkGameEnd()) {
        engine->startNewRound();
        broadcastGameState(gameId);
    }
    runBotTurnsIfNeeded(gameId);
}

game::GameEngine* Application::getGame(const std::string& gameId)
{
    std::lock_guard<std::mutex> lock(gamesMutex_);
    auto it = activeGames_.find(gameId);
    return it != activeGames_.end() ? it->second.get() : nullptr;
}

const game::GameEngine* Application::getGame(const std::string& gameId) const
{
    std::lock_guard<std::mutex> lock(gamesMutex_);
    auto it = activeGames_.find(gameId);
    return it != activeGames_.end() ? it->second.get() : nullptr;
}

std::string Application::getGameCode(const std::string& gameId) const
{
    const game::GameEngine* eng = getGame(gameId);
    if (!eng || !eng->getState()) return {};
    return eng->getState()->getGameCode();
}

void Application::broadcastGameState(const std::string& gameId)
{
    game::GameEngine* engine = getGame(gameId);
    if (!engine || !wsServer_ || !wsServer_->getSessionManager()) return;
    const game::GameState* state = engine->getState();
    auto sessionIds = wsServer_->getSessionManager()->getSessionsForGame(gameId);
    for (const std::string& sessionId : sessionIds) {
        std::string playerId;
        const auto* sess = wsServer_->getSessionManager()->getSession(sessionId);
        if (sess) playerId = sess->playerId;
        std::string stateJson = state->toJsonForPlayer(playerId);
        network::Message msg;
        msg.type = network::MessageType::GAME_STATE_UPDATE;
        msg.gameId = gameId;
        msg.payload = "{\"gameStateJson\":" + stateJson + "}";
        msg.timestamp = static_cast<uint64_t>(
            std::chrono::system_clock::now().time_since_epoch().count());
        wsServer_->sendMessage(sessionId, msg);
    }
}

void Application::cleanupInactiveGames()
{
}

network::HttpResponse Application::handleGetGames(const network::HttpRequest&)
{
    std::vector<nlohmann::json> arr;
    {
        std::lock_guard<std::mutex> lock(gamesMutex_);
        for (const auto& [gid, engine] : activeGames_) {
            if (!engine || !engine->getState()) continue;
            const game::GameState* s = engine->getState();
            nlohmann::json o;
            o["gameId"] = gid;
            o["gameCode"] = s->getGameCode();
            o["phase"] = static_cast<int>(s->getPhase());
            o["playerCount"] = static_cast<int>(s->getPlayerCount());
            o["maxPlayers"] = s->getConfig().maxPlayers;
            o["joinable"] = (s->getPhase() == game::GamePhase::LOBBY &&
                s->getPlayerCount() < static_cast<size_t>(s->getConfig().maxPlayers));
            arr.push_back(std::move(o));
        }
    }
    return network::HttpResponse::json(200, nlohmann::json(arr).dump());
}

network::HttpResponse Application::handleCreateGame(const network::HttpRequest& request)
{
    game::GameConfig cfg;
    int botCount = 0;
    try {
        if (!request.body.empty()) {
            auto j = nlohmann::json::parse(request.body);
            if (j.contains("minPlayers")) cfg.minPlayers = j["minPlayers"].get<int>();
            if (j.contains("maxPlayers")) cfg.maxPlayers = j["maxPlayers"].get<int>();
            if (j.contains("startingCards")) cfg.startingCards = j["startingCards"].get<int>();
            if (j.contains("botCount")) botCount = j["botCount"].get<int>();
        }
    } catch (...) {}
    if (botCount < 0) botCount = 0;
    if (botCount >= cfg.maxPlayers) botCount = cfg.maxPlayers - 1;
    std::string gameId = createGame(cfg);
    if (gameId.empty())
        return network::HttpResponse::json(503, "{\"error\":\"Could not create game\"}");
    std::string playerId = utils::Random::getInstance().generateUUID();
    std::string playerName = "Player";
    try {
        if (!request.body.empty()) {
            auto j = nlohmann::json::parse(request.body);
            if (j.contains("playerName")) playerName = j["playerName"].get<std::string>();
        }
    } catch (...) {}
    if (!joinGame(gameId, playerId, playerName))
        return network::HttpResponse::json(500, "{\"error\":\"Could not join game\"}");
    if (botCount > 0)
        addBotsToGame(gameId, botCount);
    game::GameEngine* eng = getGame(gameId);
    nlohmann::json out;
    out["gameId"] = gameId;
    out["playerId"] = playerId;
    if (eng && eng->getState())
        out["gameCode"] = eng->getState()->getGameCode();
    return network::HttpResponse::json(201, out.dump());
}

network::HttpResponse Application::handleGetGame(const std::string& gameId)
{
    game::GameEngine* engine = getGame(gameId);
    if (!engine || !engine->getState())
        return network::HttpResponse::notFound("{\"error\":\"Game not found\"}");
    return network::HttpResponse::json(200, engine->getState()->toJson());
}

network::HttpResponse Application::handleJoinGameHttp(const std::string& gameId,
                                                       const network::HttpRequest& request)
{
    std::string playerName = "Player";
    try {
        if (!request.body.empty()) {
            auto j = nlohmann::json::parse(request.body);
            if (j.contains("playerName")) playerName = j["playerName"].get<std::string>();
        }
    } catch (...) {}
    std::string playerId = utils::Random::getInstance().generateUUID();
    if (!joinGame(gameId, playerId, playerName))
        return network::HttpResponse::json(400, "{\"error\":\"Cannot join game\"}");
    nlohmann::json out;
    out["gameId"] = gameId;
    out["playerId"] = playerId;
    return network::HttpResponse::json(200, out.dump());
}

network::HttpResponse Application::handleJoinByCode(const network::HttpRequest& request)
{
    std::string gameCode;
    std::string playerName = "Player";
    try {
        if (!request.body.empty()) {
            auto j = nlohmann::json::parse(request.body);
            if (j.contains("gameCode")) gameCode = j["gameCode"].get<std::string>();
            if (j.contains("playerName")) playerName = j["playerName"].get<std::string>();
        }
    } catch (...) {}
    if (gameCode.empty())
        return network::HttpResponse::json(400, "{\"error\":\"gameCode required\"}");
    std::string gameId = getGameIdByCode(gameCode);
    if (gameId.empty())
        return network::HttpResponse::json(404, "{\"error\":\"Game not found\"}");
    std::string playerId = utils::Random::getInstance().generateUUID();
    if (!joinGame(gameId, playerId, playerName))
        return network::HttpResponse::json(400, "{\"error\":\"Cannot join game\"}");
    nlohmann::json out;
    out["gameId"] = gameId;
    out["playerId"] = playerId;
    return network::HttpResponse::json(200, out.dump());
}

} // namespace whot
