#include "../../include/Game/TurnManager.hpp"
#include "../../include/Core/GameConstants.hpp"
#include <algorithm>

namespace whot::game {

TurnManager::TurnManager(GameState* state)
    : state_(state)
    , allowMultipleActions_(false)
    , turnStartTime_(std::chrono::steady_clock::now())
{}

void TurnManager::startTurn() {
    turnStartTime_ = std::chrono::steady_clock::now();
    clearSkips();
}

void TurnManager::endTurn() {
    allowMultipleActions_ = false;
    if (state_) state_->advanceTurn();
}

void TurnManager::skipTurn() {
    if (state_) state_->skipNextPlayer();
}

void TurnManager::forceNextPlayer() {
    if (state_) state_->advanceTurn();
}

std::string TurnManager::getCurrentPlayerId() const {
    const core::Player* p = state_ ? state_->getCurrentPlayer() : nullptr;
    return p ? p->getId() : "";
}

bool TurnManager::isPlayerTurn(const std::string& playerId) const {
    return getCurrentPlayerId() == playerId;
}

void TurnManager::recordAction(const TurnAction& action) {
    actionHistory_.push(action);
    while (actionHistory_.size() > 100) actionHistory_.pop();
}

std::vector<TurnAction> TurnManager::getTurnHistory(int count) const {
    std::vector<TurnAction> out;
    std::queue<TurnAction> q = actionHistory_;
    while (!q.empty() && out.size() < static_cast<size_t>(count)) {
        out.push_back(q.front());
        q.pop();
    }
    return out;
}

const TurnAction* TurnManager::getLastAction() const {
    if (actionHistory_.empty()) return nullptr;
    return &actionHistory_.back();
}

void TurnManager::enableMultipleActions() { allowMultipleActions_ = true; }
void TurnManager::disableMultipleActions() { allowMultipleActions_ = false; }
bool TurnManager::canPlayAgain() const { return allowMultipleActions_; }

void TurnManager::queueSkip(const std::string& playerId) {
    skippedPlayers_.push_back(playerId);
}

bool TurnManager::isPlayerSkipped(const std::string& playerId) const {
    return std::find(skippedPlayers_.begin(), skippedPlayers_.end(), playerId) != skippedPlayers_.end();
}

void TurnManager::clearSkips() { skippedPlayers_.clear(); }

void TurnManager::advanceToNextActivePlayer() {
    if (state_) state_->advanceTurn();
}

std::chrono::seconds TurnManager::getRemainingTime() const {
    if (!state_) return std::chrono::seconds(0);
    int limit = state_->getConfig().turnTimeSeconds;
    if (limit <= 0) return std::chrono::seconds(9999);
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - turnStartTime_).count();
    int remaining = limit - static_cast<int>(elapsed);
    return std::chrono::seconds(remaining > 0 ? remaining : 0);
}

bool TurnManager::hasTurnExpired() const {
    return getRemainingTime().count() <= 0;
}

void TurnManager::resetTimer() {
    turnStartTime_ = std::chrono::steady_clock::now();
}

} // namespace whot::game
