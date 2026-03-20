#ifndef WHOT_AI_AI_PLAYER_HPP
#define WHOT_AI_AI_PLAYER_HPP

#include "Core/Player.hpp"
#include "Game/GameState.hpp"
#include "Game/ActionTypes.hpp"
#include "AI/Strategy.hpp"
#include "AI/DifficultyLevel.hpp"
#include <memory>

namespace whot::ai {

class AIPlayer {
public:
    AIPlayer(const std::string& id, const std::string& name, 
             DifficultyLevel difficulty);
    
    // Decision making
    game::GameAction decideAction(const game::GameState& state);
    
    // Specific decisions
    std::optional<size_t> chooseCardToPlay(const game::GameState& state);
    core::Suit chooseSuitForWhotCard(const game::GameState& state);
    bool shouldDefendAgainstPick(const game::GameState& state,
                                  const core::Card& attackCard);
    
    // Strategy management
    void setDifficulty(DifficultyLevel level);
    DifficultyLevel getDifficulty() const;
    void setStrategy(std::unique_ptr<Strategy> strategy);
    
    // Behavior
    void setThinkingDelay(int milliseconds);  // Simulate human-like delay
    
private:
    std::string id_;
    std::string name_;
    DifficultyLevel difficulty_;
    std::unique_ptr<Strategy> strategy_;
    int thinkingDelay_;
    
    // Helper methods
    std::vector<size_t> getPlayableCards(const game::GameState& state) const;
    size_t selectBestCard(const game::GameState& state,
                          const std::vector<size_t>& playableIndices);
    void applyRandomness(std::vector<size_t>& choices);
};

} // namespace whot::ai

#endif // WHOT_AI_AI_PLAYER_HPP
