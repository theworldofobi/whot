#ifndef WHOT_GAME_GAME_ENGINE_HPP
#define WHOT_GAME_GAME_ENGINE_HPP

#include "Game/GameState.hpp"
#include "Game/ActionTypes.hpp"
#include "Game/RuleEngine.hpp"
#include "Game/TurnManager.hpp"
#include <memory>
#include <functional>
#include <map>

namespace whot::game {

using GameEventCallback = std::function<void(const std::string& eventType, 
                                              const std::string& eventData)>;

class GameEngine {
public:
    explicit GameEngine(std::unique_ptr<GameState> state);
    
    // Game flow
    void startGame();
    void startNewRound();
    ActionResult processAction(const GameAction& action);
    void endGame();
    
    // State access
    GameState* getState();
    const GameState* getState() const;
    
    // Event system
    void registerEventCallback(const std::string& eventType, GameEventCallback callback);
    void unregisterEventCallback(const std::string& eventType);
    
    // Validation
    bool isValidAction(const GameAction& action) const;
    std::vector<ActionType> getValidActionsForCurrentPlayer() const;
    
    // Game queries
    bool isGameActive() const;
    std::vector<std::string> getWinners() const;
    
private:
    std::unique_ptr<GameState> state_;
    std::unique_ptr<RuleEngine> ruleEngine_;
    std::unique_ptr<TurnManager> turnManager_;
    
    std::map<std::string, std::vector<GameEventCallback>> eventCallbacks_;
    
    // Action handlers
    ActionResult handlePlayCard(const GameAction& action);
    ActionResult handleDrawCard(const GameAction& action);
    ActionResult handleDeclaration(const GameAction& action);
    ActionResult handleSuitChoice(const GameAction& action);
    
    // Event dispatching
    void emitEvent(const std::string& eventType, const std::string& eventData);
    
    // Special card execution
    void executeSpecialCard(const core::Card& card, core::Player* player);
    void handlePickTwo(core::Player* nextPlayer);
    void handlePickThree(core::Player* nextPlayer);
    void handleHoldOn(core::Player* player);
    void handleSuspension();
    void handleGeneralMarket(core::Player* currentPlayer);
    void handleWhotCard(core::Player* player, core::Suit chosenSuit);
    
    // Validation helpers
    bool validateCardPlay(const core::Player* player, 
                          const core::Card& card) const;
    bool validateTurnTiming(const core::Player* player) const;
};

} // namespace whot::game

#endif // WHOT_GAME_GAME_ENGINE_HPP
