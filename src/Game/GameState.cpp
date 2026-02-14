#include "../../include/Game/GameState.hpp"
#include "../../include/Core/GameConstants.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <sstream>

namespace whot::game {

namespace {
std::string generateGameId() {
    static int counter = 0;
    std::ostringstream oss;
    oss << "game-" << std::chrono::system_clock::now().time_since_epoch().count() << "-" << (counter++);
    return oss.str();
}
} // namespace

GameState::GameState(const GameConfig& config)
    : config_(config)
    , phase_(GamePhase::LOBBY)
    , currentPlayerIndex_(0)
    , direction_(PlayDirection::CLOCKWISE)
    , activePickCount_(0)
    , gameId_(generateGameId())
    , createdAt_(std::chrono::system_clock::now())
{}

void GameState::initialize() {
    phase_ = GamePhase::LOBBY;
    currentPlayerIndex_ = 0;
    direction_ = PlayDirection::CLOCKWISE;
    activePickCount_ = 0;
    demandedSuit_.reset();
    deck_.clear();
    discardPile_.clear();
    callCard_.reset();
}

void GameState::startRound() {
    phase_ = GamePhase::IN_PROGRESS;
    currentPlayerIndex_ = 0;
    direction_ = PlayDirection::CLOCKWISE;
    activePickCount_ = 0;
    demandedSuit_.reset();
    deck_.reset();
    discardPile_.clear();
    callCard_.reset();
    for (auto& p : players_) {
        if (p && p->getStatus() == core::PlayerStatus::ACTIVE)
            p->resetTurnFlags();
    }
}

void GameState::endRound() { phase_ = GamePhase::ROUND_ENDED; }
void GameState::endGame() { phase_ = GamePhase::GAME_ENDED; }

void GameState::addPlayer(std::unique_ptr<core::Player> player) {
    if (player && players_.size() < static_cast<size_t>(config_.maxPlayers)) {
        if (players_.empty())
            creatorPlayerId_ = player->getId();
        players_.push_back(std::move(player));
    }
}

void GameState::removePlayer(const std::string& playerId) {
    players_.erase(
        std::remove_if(players_.begin(), players_.end(),
            [&playerId](const std::unique_ptr<core::Player>& p) {
                return p && p->getId() == playerId;
            }),
        players_.end());
}

core::Player* GameState::getPlayer(const std::string& playerId) {
    for (auto& p : players_)
        if (p && p->getId() == playerId) return p.get();
    return nullptr;
}

const core::Player* GameState::getPlayer(const std::string& playerId) const {
    for (const auto& p : players_)
        if (p && p->getId() == playerId) return p.get();
    return nullptr;
}

core::Player* GameState::getCurrentPlayer() {
    if (players_.empty() || currentPlayerIndex_ < 0 ||
        static_cast<size_t>(currentPlayerIndex_) >= players_.size())
        return nullptr;
    return players_[static_cast<size_t>(currentPlayerIndex_)].get();
}

const core::Player* GameState::getCurrentPlayer() const {
    if (players_.empty() || currentPlayerIndex_ < 0 ||
        static_cast<size_t>(currentPlayerIndex_) >= players_.size())
        return nullptr;
    return players_[static_cast<size_t>(currentPlayerIndex_)].get();
}

std::vector<core::Player*> GameState::getActivePlayers() {
    std::vector<core::Player*> out;
    for (auto& p : players_)
        if (p && p->getStatus() == core::PlayerStatus::ACTIVE)
            out.push_back(p.get());
    return out;
}

std::vector<core::Player*> GameState::getActivePlayers() const {
    std::vector<core::Player*> out;
    for (const auto& p : players_)
        if (p && p->getStatus() == core::PlayerStatus::ACTIVE)
            out.push_back(p.get());
    return out;
}

size_t GameState::getPlayerCount() const { return players_.size(); }

int GameState::getNextPlayerIndex() const {
    if (players_.empty()) return 0;
    int n = static_cast<int>(players_.size());
    int next = currentPlayerIndex_;
    if (direction_ == PlayDirection::CLOCKWISE)
        next = (currentPlayerIndex_ + 1 + n) % n;
    else
        next = (currentPlayerIndex_ - 1 + n) % n;
    return next;
}

void GameState::advanceTurn() {
    if (players_.empty()) return;
    currentPlayerIndex_ = getNextPlayerIndex();
}

void GameState::skipNextPlayer() {
    advanceTurn();
}

void GameState::reverseDirection() {
    direction_ = (direction_ == PlayDirection::CLOCKWISE)
        ? PlayDirection::COUNTER_CLOCKWISE
        : PlayDirection::CLOCKWISE;
}

int GameState::getCurrentPlayerIndex() const { return currentPlayerIndex_; }
PlayDirection GameState::getPlayDirection() const { return direction_; }

core::Deck& GameState::getDeck() { return deck_; }
const core::Card* GameState::getCallCard() const { return callCard_.get(); }

void GameState::setCallCard(std::unique_ptr<core::Card> card) {
    callCard_ = std::move(card);
}

void GameState::addToDiscardPile(std::unique_ptr<core::Card> card) {
    if (card) discardPile_.push_back(std::move(card));
}

void GameState::setActivePickCount(int count) { activePickCount_ = count; }
int GameState::getActivePickCount() const { return activePickCount_; }
void GameState::resetActivePickCount() { activePickCount_ = 0; }

void GameState::setDemandedSuit(std::optional<core::Suit> suit) { demandedSuit_ = suit; }
std::optional<core::Suit> GameState::getDemandedSuit() const { return demandedSuit_; }
void GameState::clearDemandedSuit() { demandedSuit_.reset(); }

GamePhase GameState::getPhase() const { return phase_; }
void GameState::setPhase(GamePhase phase) { phase_ = phase; }
const GameConfig& GameState::getConfig() const { return config_; }
const std::string& GameState::getGameId() const { return gameId_; }
const std::string& GameState::getGameCode() const { return gameCode_; }
void GameState::setGameCode(const std::string& code) { gameCode_ = code; }
const std::string& GameState::getCreatorPlayerId() const { return creatorPlayerId_; }
void GameState::setCreatorPlayerId(const std::string& playerId) { creatorPlayerId_ = playerId; }

bool GameState::needsReshufffle() const {
    return deck_.isEmpty() && !discardPile_.empty();
}

void GameState::reshuffleDiscardPile() {
    if (callCard_ && !discardPile_.empty()) {
        deck_.reshuffleFromDiscardPile(std::move(discardPile_), *callCard_);
        discardPile_.clear();
    }
}

bool GameState::checkRoundEnd() const {
    for (const auto& p : players_)
        if (p && p->getHand().isEmpty()) return true;
    return false;
}

bool GameState::checkGameEnd() const {
    return phase_ == GamePhase::GAME_ENDED;
}

std::optional<std::string> GameState::getWinnerId() const {
    for (const auto& p : players_)
        if (p && p->getHand().isEmpty()) return p->getId();
    return std::nullopt;
}

void GameState::eliminatePlayers() {
    for (auto& p : players_) {
        if (p && p->getCurrentScore() >= config_.eliminationScore)
            p->setStatus(core::PlayerStatus::ELIMINATED);
    }
}

std::string GameState::toJson() const {
    using json = nlohmann::json;
    json j;
    j["gameId"] = gameId_;
    j["gameCode"] = gameCode_;
    j["creatorPlayerId"] = creatorPlayerId_;
    j["phase"] = static_cast<int>(phase_);
    j["currentPlayerIndex"] = currentPlayerIndex_;
    j["direction"] = (direction_ == PlayDirection::CLOCKWISE ? "clockwise" : "counter_clockwise");
    j["activePickCount"] = activePickCount_;
    if (demandedSuit_.has_value())
        j["demandedSuit"] = core::suitToString(demandedSuit_.value());
    j["players"] = json::array();
    for (const auto& p : players_)
        if (p) j["players"].push_back(json::parse(p->toJson()));
    if (callCard_) j["callCard"] = json::parse(callCard_->toJson());
    j["deckSize"] = deck_.size();
    j["discardPileSize"] = discardPile_.size();
    auto winnerId = getWinnerId();
    if (winnerId.has_value()) j["winnerId"] = winnerId.value();
    return j.dump();
}

std::string GameState::toJsonForPlayer(const std::string& viewerPlayerId) const {
    using json = nlohmann::json;
    json j;
    j["gameId"] = gameId_;
    j["gameCode"] = gameCode_;
    j["creatorPlayerId"] = creatorPlayerId_;
    j["phase"] = static_cast<int>(phase_);
    j["currentPlayerIndex"] = currentPlayerIndex_;
    j["direction"] = (direction_ == PlayDirection::CLOCKWISE ? "clockwise" : "counter_clockwise");
    j["activePickCount"] = activePickCount_;
    if (demandedSuit_.has_value())
        j["demandedSuit"] = core::suitToString(demandedSuit_.value());
    j["players"] = json::array();
    for (const auto& p : players_) {
        if (!p) continue;
        if (p->getId() == viewerPlayerId) {
            j["players"].push_back(json::parse(p->toJson()));
        } else {
            json pj = json::parse(p->toJson());
            pj["hand"] = json::object();
            pj["hand"]["count"] = p->getHand().size();
            j["players"].push_back(std::move(pj));
        }
    }
    if (callCard_) j["callCard"] = json::parse(callCard_->toJson());
    j["deckSize"] = deck_.size();
    j["discardPileSize"] = discardPile_.size();
    auto winnerId = getWinnerId();
    if (winnerId.has_value()) j["winnerId"] = winnerId.value();
    return j.dump();
}

std::unique_ptr<GameState> GameState::fromJson(const std::string& jsonStr) {
    using json = nlohmann::json;
    json j = json::parse(jsonStr);
    GameConfig cfg;
    cfg.minPlayers = j.value("minPlayers", 2);
    cfg.maxPlayers = j.value("maxPlayers", 8);
    cfg.startingCards = j.value("startingCards", 6);
    cfg.eliminationScore = j.value("eliminationScore", 100);
    auto state = std::make_unique<GameState>(cfg);
    state->gameId_ = j.value("gameId", "");
    state->gameCode_ = j.value("gameCode", "");
    state->creatorPlayerId_ = j.value("creatorPlayerId", "");
    state->phase_ = static_cast<GamePhase>(j.value("phase", 0));
    state->currentPlayerIndex_ = j.value("currentPlayerIndex", 0);
    state->direction_ = (j.value("direction", "clockwise") == "clockwise")
        ? PlayDirection::CLOCKWISE : PlayDirection::COUNTER_CLOCKWISE;
    state->activePickCount_ = j.value("activePickCount", 0);
    if (j.contains("demandedSuit"))
        state->demandedSuit_ = core::stringToSuit(j["demandedSuit"].get<std::string>());
    if (j.contains("players") && j["players"].is_array())
        for (const auto& pj : j["players"]) {
            auto p = core::Player::fromJson(pj.dump());
            if (p) state->players_.push_back(std::move(p));
        }
    if (j.contains("callCard") && !j["callCard"].is_null())
        state->callCard_ = core::Card::fromJson(j["callCard"].dump());
    return state;
}

} // namespace whot::game
