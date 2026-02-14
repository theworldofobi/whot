#ifndef WHOT_GAME_RULE_ENGINE_HPP
#define WHOT_GAME_RULE_ENGINE_HPP

#include "Rules/NigerianRules.hpp"
#include "Game/GameState.hpp"
#include <memory>
#include <vector>

namespace whot::game {

/**
 * Rule Engine for Whot Game (Nigerian Rules)
 * 
 * This class wraps the Nigerian rules implementation and provides
 * game-state-aware validation methods. It acts as the bridge between
 * the game engine and the rule system.
 */
class RuleEngine {
public:
    RuleEngine();
    
    // Get access to the rules (for configuration)
    rules::NigerianRules& getRules();
    const rules::NigerianRules& getRules() const;
    
    // ========================================
    // Card Play Validation
    // ========================================
    
    /**
     * Check if a player can play a specific card
     * Considers: call card, demanded suit, active pick chains
     */
    bool canPlayCard(const GameState& state, 
                     const core::Player& player,
                     const core::Card& card) const;
    
    /**
     * Check if player must draw a card
     * True if: no playable cards OR under attack and cannot defend
     */
    bool mustDrawCard(const GameState& state,
                      const core::Player& player) const;
    
    /**
     * Calculate how many cards player must draw
     * Accounts for: active pick chains (2s and 5s)
     */
    int calculateDrawCount(const GameState& state,
                           const core::Player& player) const;
    
    // ========================================
    // Special Moves
    // ========================================
    
    /**
     * Check if player can defend against a Pick attack
     * Must have matching pick card (2 for 2, 5 for 5)
     */
    bool canDefendAgainstPick(const GameState& state,
                              const core::Player& player,
                              const core::Card& attackCard) const;
    
    /**
     * Validate double-decking move
     * All cards must have same number, first must match call card
     */
    bool canDoubleDesk(const GameState& state,
                       const core::Player& player,
                       const std::vector<const core::Card*>& cards) const;
    
    // ========================================
    // Declaration Validation
    // ========================================
    
    /** Check if player must declare "Last card" */
    bool requiresLastCardDeclaration(const core::Player& player) const;
    
    /** Check if player must declare "Check up" */
    bool requiresCheckUpDeclaration(const core::Player& player) const;
    
    /** Get penalty for missing declaration */
    int getDeclarationPenalty() const;
    
    // ========================================
    // Scoring
    // ========================================
    
    /** Calculate score for player's current hand */
    int calculateRoundScore(const core::Player& player) const;
    
    /** Check if player should be eliminated (for multi-round games) */
    bool isPlayerEliminated(const core::Player& player) const;
    
    // ========================================
    // Turn Timing
    // ========================================
    
    /** Check if turn timer is enforced */
    bool enforceTurnTimer() const;
    
    /** Get turn time limit in seconds */
    int getTurnTimeLimit() const;
    
private:
    std::unique_ptr<rules::NigerianRules> rules_;
    
    // Helper methods
    bool hasPlayableCard(const GameState& state,
                         const core::Player& player) const;
    bool hasDefenseCard(const GameState& state,
                        const core::Player& player) const;
};

} // namespace whot::game

#endif // WHOT_GAME_RULE_ENGINE_HPP