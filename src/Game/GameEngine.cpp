#include "../../include/Game/GameEngine.hpp"
#include "../../include/Game/ScoreCalculator.hpp"
#include "../../include/Core/GameConstants.hpp"
#include "../../include/Core/Card.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>

namespace whot::game {

GameEngine::GameEngine(std::unique_ptr<GameState> state)
    : state_(std::move(state))
    , ruleEngine_(std::make_unique<RuleEngine>())
{
    if (state_)
        turnManager_ = std::make_unique<TurnManager>(state_.get());
}

void GameEngine::startGame() {
    if (!state_) return;
    state_->initialize();
    state_->setPhase(GamePhase::STARTING);
}

void GameEngine::startNewRound() {
    if (!state_) return;
    state_->startRound();
    // Deal starting cards to each active player
    int nCards = state_->getConfig().startingCards;
    for (int round = 0; round < nCards; ++round) {
        for (auto* p : state_->getActivePlayers()) {
            if (!p) continue;
            if (state_->getDeck().isEmpty() && state_->needsReshufffle())
                state_->reshuffleDiscardPile();
            auto card = state_->getDeck().draw();
            if (card) p->getHand().addCard(std::move(card));
        }
    }
    // Flip first call card from deck
    if (!state_->getDeck().isEmpty()) {
        auto first = state_->getDeck().draw();
        if (first) state_->setCallCard(std::move(first));
    }
    if (turnManager_) turnManager_->startTurn();
    emitEvent("round_started", state_->toJson());
}

ActionResult GameEngine::processAction(const GameAction& action) {
    if (!isValidAction(action)) {
        ActionResult r;
        r.success = false;
        r.message = "Invalid action";
        return r;
    }
    switch (action.type) {
        case ActionType::PLAY_CARD: return handlePlayCard(action);
        case ActionType::DRAW_CARD: return handleDrawCard(action);
        case ActionType::DECLARE_LAST_CARD:
        case ActionType::DECLARE_CHECK_UP: return handleDeclaration(action);
        case ActionType::CHOOSE_SUIT: return handleSuitChoice(action);
        default: {
            ActionResult r; r.success = false; r.message = "Unhandled action"; return r;
        }
    }
}

void GameEngine::endGame() {
    if (state_) state_->endGame();
    emitEvent("game_ended", state_ ? state_->toJson() : "{}");
}

GameState* GameEngine::getState() { return state_.get(); }
const GameState* GameEngine::getState() const { return state_.get(); }

void GameEngine::registerEventCallback(const std::string& eventType, GameEventCallback callback) {
    eventCallbacks_[eventType].push_back(std::move(callback));
}

void GameEngine::unregisterEventCallback(const std::string& eventType) {
    eventCallbacks_.erase(eventType);
}

bool GameEngine::isValidAction(const GameAction& action) const {
    if (!state_ || !turnManager_) return false;
    if (action.playerId != turnManager_->getCurrentPlayerId()) return false;
    switch (action.type) {
        case ActionType::PLAY_CARD: {
            if (!action.cardIndex.has_value()) return false;
            core::Player* p = state_->getPlayer(action.playerId);
            if (!p || action.cardIndex.value() >= p->getHand().size()) return false;
            return validateCardPlay(p, p->getHand().getCard(action.cardIndex.value()));
        }
        case ActionType::DRAW_CARD:
            return ruleEngine_->mustDrawCard(*state_, *state_->getCurrentPlayer());
        case ActionType::DECLARE_LAST_CARD:
        case ActionType::DECLARE_CHECK_UP:
            return true;
        case ActionType::CHOOSE_SUIT:
            return action.chosenSuit.has_value();
        default: return false;
    }
}

std::vector<ActionType> GameEngine::getValidActionsForCurrentPlayer() const {
    std::vector<ActionType> out;
    if (!state_ || !turnManager_) return out;
    core::Player* p = state_->getCurrentPlayer();
    if (!p) return out;
    bool canPlay = false;
    for (size_t i = 0; i < p->getHand().size(); ++i) {
        if (ruleEngine_->canPlayCard(*state_, *p, p->getHand().getCard(i))) {
            canPlay = true; break;
        }
    }
    if (canPlay) out.push_back(ActionType::PLAY_CARD);
    if (ruleEngine_->mustDrawCard(*state_, *p))
        out.push_back(ActionType::DRAW_CARD);
    if (ruleEngine_->requiresLastCardDeclaration(*p))
        out.push_back(ActionType::DECLARE_LAST_CARD);
    if (ruleEngine_->requiresCheckUpDeclaration(*p))
        out.push_back(ActionType::DECLARE_CHECK_UP);
    return out;
}

bool GameEngine::isGameActive() const {
    return state_ && state_->getPhase() == GamePhase::IN_PROGRESS;
}

std::vector<std::string> GameEngine::getWinners() const {
    std::vector<std::string> out;
    if (!state_) return out;
    auto wid = state_->getWinnerId();
    if (wid.has_value()) out.push_back(wid.value());
    return out;
}

ActionResult GameEngine::handlePlayCard(const GameAction& action) {
    ActionResult r;
    r.success = false;
    if (!state_ || !action.cardIndex.has_value()) { r.message = "Invalid play"; return r; }
    core::Player* player = state_->getPlayer(action.playerId);
    if (!player) { r.message = "Player not found"; return r; }
    size_t idx = action.cardIndex.value();
    if (idx >= player->getHand().size()) { r.message = "Invalid card index"; return r; }
    const core::Card& card = player->getHand().getCard(idx);
    if (!validateCardPlay(player, card)) { r.message = "Card cannot be played"; return r; }

    std::unique_ptr<core::Card> played = player->getHand().playCard(idx);
    if (!played) { r.message = "Failed to play card"; return r; }

    state_->addToDiscardPile(std::move(played));
    state_->setCallCard(std::make_unique<core::Card>(card.getSuit(), card.getValue()));
    state_->clearDemandedSuit();

    executeSpecialCard(card, player);

    r.affectedPlayerIds.push_back(action.playerId);
    r.success = true;
    r.newStateJson = state_->toJson();
    emitEvent("card_played", r.newStateJson);

    if (state_->checkRoundEnd()) {
        auto winnerId = state_->getWinnerId();
        if (winnerId.has_value()) {
            core::Player* w = state_->getPlayer(winnerId.value());
            if (w) w->incrementGamesWon();
        }
        for (auto* p : state_->getActivePlayers()) {
            if (p) {
                p->addToScore(ScoreCalculator::calculateHandScore(p->getHand()));
                p->incrementGamesPlayed();
            }
        }
        state_->endRound();
        emitEvent("round_ended", state_->toJson());
    } else if (!turnManager_->canPlayAgain()) {
        turnManager_->endTurn();
    }

    return r;
}

ActionResult GameEngine::handleDrawCard(const GameAction& action) {
    ActionResult r;
    r.success = false;
    if (!state_) return r;
    core::Player* player = state_->getPlayer(action.playerId);
    if (!player) return r;
    int count = ruleEngine_->calculateDrawCount(*state_, *player);
    if (state_->needsReshufffle()) state_->reshuffleDiscardPile();
    for (int i = 0; i < count && !state_->getDeck().isEmpty(); ++i) {
        auto card = state_->getDeck().draw();
        if (card) player->getHand().addCard(std::move(card));
    }
    state_->resetActivePickCount();
    r.success = true;
    r.affectedPlayerIds.push_back(action.playerId);
    r.newStateJson = state_->toJson();
    turnManager_->endTurn();
    emitEvent("card_drawn", r.newStateJson);
    return r;
}

ActionResult GameEngine::handleDeclaration(const GameAction& action) {
    ActionResult r;
    r.success = true;
    core::Player* p = state_ ? state_->getPlayer(action.playerId) : nullptr;
    if (p) {
        if (action.type == ActionType::DECLARE_LAST_CARD) p->setSaidLastCard(true);
        if (action.type == ActionType::DECLARE_CHECK_UP) p->setSaidCheckUp(true);
    }
    r.newStateJson = state_ ? state_->toJson() : "{}";
    return r;
}

ActionResult GameEngine::handleSuitChoice(const GameAction& action) {
    ActionResult r;
    r.success = false;
    if (!action.chosenSuit.has_value()) return r;
    if (state_) state_->setDemandedSuit(action.chosenSuit.value());
    if (action.chosenDirection.has_value() && state_ && state_->getConfig().allowDirectionChange)
        state_->reverseDirection();
    r.success = true;
    r.newStateJson = state_ ? state_->toJson() : "{}";
    return r;
}

void GameEngine::emitEvent(const std::string& eventType, const std::string& eventData) {
    auto it = eventCallbacks_.find(eventType);
    if (it != eventCallbacks_.end())
        for (const auto& cb : it->second) cb(eventType, eventData);
    it = eventCallbacks_.find("*");
    if (it != eventCallbacks_.end())
        for (const auto& cb : it->second) cb(eventType, eventData);
}

void GameEngine::executeSpecialCard(const core::Card& card, core::Player* player) {
    (void)player;
    core::SpecialAbility ab = card.getSpecialAbility();
    core::CardValue val = card.getValue();

    if (ab == core::SpecialAbility::HOLD_ON) {
        if (turnManager_) turnManager_->enableMultipleActions();
        if (state_) state_->skipNextPlayer();
        return;
    }
    if (ab == core::SpecialAbility::PICK_TWO) {
        state_->setActivePickCount(state_->getActivePickCount() + 2);
        return;
    }
    if (val == core::CardValue::FIVE) {
        state_->setActivePickCount(state_->getActivePickCount() + 3);
        return;
    }
    if (val == core::CardValue::EIGHT) {
        if (state_) state_->skipNextPlayer();
        return;
    }
    if (ab == core::SpecialAbility::GENERAL_MARKET) {
        for (auto* p : state_->getActivePlayers()) {
            if (!p || p->getId() == player->getId()) continue;
            if (state_->getDeck().isEmpty() && state_->needsReshufffle()) state_->reshuffleDiscardPile();
            auto c = state_->getDeck().draw();
            if (c) p->getHand().addCard(std::move(c));
        }
        return;
    }
    if (ab == core::SpecialAbility::WHOT_CARD) {
        if (state_) state_->clearDemandedSuit();
    }
}

void GameEngine::handlePickTwo(core::Player* nextPlayer) { (void)nextPlayer; }
void GameEngine::handlePickThree(core::Player* nextPlayer) { (void)nextPlayer; }
void GameEngine::handleHoldOn(core::Player* player) { (void)player; }
void GameEngine::handleSuspension() {}
void GameEngine::handleGeneralMarket(core::Player* currentPlayer) { (void)currentPlayer; }
void GameEngine::handleWhotCard(core::Player* player, core::Suit chosenSuit) { (void)player; (void)chosenSuit; }

bool GameEngine::validateCardPlay(const core::Player* player, const core::Card& card) const {
    if (!state_ || !player) return false;
    const core::Card* call = state_->getCallCard();
    if (!call) return false;
    return ruleEngine_->canPlayCard(*state_, *player, card);
}

bool GameEngine::validateTurnTiming(const core::Player* player) const {
    if (!player || !ruleEngine_->enforceTurnTimer()) return true;
    return !player->hasExceededTurnTime(ruleEngine_->getTurnTimeLimit());
}

} // namespace whot::game
