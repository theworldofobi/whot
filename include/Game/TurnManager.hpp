#ifndef WHOT_GAME_TURN_MANAGER_HPP
#define WHOT_GAME_TURN_MANAGER_HPP

#include "Game/GameState.hpp"
#include "Game/ActionTypes.hpp"
#include <queue>
#include <chrono>
#include <vector>

namespace whot::game {

struct TurnAction {
    std::string playerId;
    ActionType action;
    std::chrono::system_clock::time_point timestamp;
    bool completed;
};

class TurnManager {
public:
    explicit TurnManager(GameState* state);
    
    // Turn control
    void startTurn();
    void endTurn();
    void skipTurn();
    void forceNextPlayer();
    
    // Turn state
    std::string getCurrentPlayerId() const;
    bool isPlayerTurn(const std::string& playerId) const;
    
    // Turn history
    void recordAction(const TurnAction& action);
    std::vector<TurnAction> getTurnHistory(int count = 10) const;
    const TurnAction* getLastAction() const;
    
    // Special turn mechanics
    void enableMultipleActions();  // For Hold On, straights
    void disableMultipleActions();
    bool canPlayAgain() const;
    
    void queueSkip(const std::string& playerId);
    bool isPlayerSkipped(const std::string& playerId) const;
    
    // Timing
    std::chrono::seconds getRemainingTime() const;
    bool hasTurnExpired() const;
    void resetTimer();
    
private:
    GameState* state_;
    std::queue<TurnAction> actionHistory_;
    std::vector<std::string> skippedPlayers_;
    
    bool allowMultipleActions_;
    std::chrono::steady_clock::time_point turnStartTime_;
    
    void clearSkips();
    void advanceToNextActivePlayer();
};

} // namespace whot::game

#endif // WHOT_GAME_TURN_MANAGER_HPP
