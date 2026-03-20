#ifndef WHOT_AI_STRATEGY_HPP
#define WHOT_AI_STRATEGY_HPP

#include "Game/GameState.hpp"
#include "Core/Card.hpp"
#include <optional>
#include <vector>
#include <map>

namespace whot::ai {

class Strategy {
public:
    virtual ~Strategy() = default;
    
    // Card selection
    virtual size_t selectCard(const game::GameState& state,
                              const std::vector<size_t>& playableCards) = 0;
    
    virtual core::Suit selectSuit(const game::GameState& state) = 0;
    
    virtual bool shouldDefend(const game::GameState& state,
                              const core::Card& attackCard,
                              const std::vector<size_t>& defenseCards) = 0;
    
    virtual std::string getStrategyName() const = 0;
    
protected:
    // Utility functions for strategies
    int evaluateCardValue(const core::Card& card,
                          const game::GameState& state) const;
    
    core::Player* getPlayerClosestToWinning(const game::GameState& state) const;
    
    std::map<core::Suit, int> countSuits(const core::Hand& hand) const;
};

// Concrete strategies
class AggressiveStrategy : public Strategy {
public:
    size_t selectCard(const game::GameState& state,
                      const std::vector<size_t>& playableCards) override;
    
    core::Suit selectSuit(const game::GameState& state) override;
    
    bool shouldDefend(const game::GameState& state,
                      const core::Card& attackCard,
                      const std::vector<size_t>& defenseCards) override;
    
    std::string getStrategyName() const override;
};

class DefensiveStrategy : public Strategy {
public:
    size_t selectCard(const game::GameState& state,
                      const std::vector<size_t>& playableCards) override;
    
    core::Suit selectSuit(const game::GameState& state) override;
    
    bool shouldDefend(const game::GameState& state,
                      const core::Card& attackCard,
                      const std::vector<size_t>& defenseCards) override;
    
    std::string getStrategyName() const override;
};

class BalancedStrategy : public Strategy {
public:
    size_t selectCard(const game::GameState& state,
                      const std::vector<size_t>& playableCards) override;
    
    core::Suit selectSuit(const game::GameState& state) override;
    
    bool shouldDefend(const game::GameState& state,
                      const core::Card& attackCard,
                      const std::vector<size_t>& defenseCards) override;
    
    std::string getStrategyName() const override;
};

class RandomStrategy : public Strategy {
public:
    size_t selectCard(const game::GameState& state,
                      const std::vector<size_t>& playableCards) override;
    
    core::Suit selectSuit(const game::GameState& state) override;
    
    bool shouldDefend(const game::GameState& state,
                      const core::Card& attackCard,
                      const std::vector<size_t>& defenseCards) override;
    
    std::string getStrategyName() const override;
};

} // namespace whot::ai

#endif // WHOT_AI_STRATEGY_HPP
