#ifndef WHOT_RULES_NIGERIAN_RULES_HPP
#define WHOT_RULES_NIGERIAN_RULES_HPP

#include "Core/Card.hpp"
#include "Core/Player.hpp"
#include "Core/Hand.hpp"
#include <vector>
#include <string>
#include <optional>

namespace whot::rules {

/**
 * Nigerian Whot Rules Implementation
 * 
 * This class implements the Nigerian variant of Whot, which is the most
 * popular version and considered Nigeria's national card game.
 * 
 * Key Rules:
 * - Players start with 3-6 cards (configurable)
 * - Cards can be played if they match suit OR number with the call card
 * - Special cards have unique effects:
 *   - 1 (Hold On): Skip all players, play again
 *   - 2 (Pick Two): Next player picks 2 cards (chainable)
 *   - 5 (Pick Three): Next player picks 3 cards (chainable)
 *   - 8 (Suspension): Skip next player
 *   - 14 (General Market): All other players pick 1 card
 *   - 20 (Whot): Change suit, optionally change direction
 * 
 * - Players must declare "Last card" when playing their second-to-last card
 * - Players must declare "Check up" when playing their last card
 * - Penalty for not declaring: Pick 2 more cards
 * 
 * - Optional: Double-decking (play multiple cards of same number)
 * - Optional: Direction change on Pick 2/3 cards
 * - Optional: 10-second turn timer
 */
class NigerianRules {
public:
    NigerianRules();
    
    // ========================================
    // Card Play Validation
    // ========================================
    
    /**
     * Check if a card can be played on the current call card
     * 
     * Rules:
     * - Whot cards (20) can play on anything
     * - Card must match suit OR number with call card
     * - If a suit is demanded (via Whot card), must match that suit
     * - Cannot play defensive cards (2, 5) if not defending
     */
    bool canPlayCard(const core::Card& card,
                     const core::Card& callCard,
                     const core::Player& player,
                     std::optional<core::Suit> demandedSuit = std::nullopt) const;
    
    /**
     * Check if a card can be used to defend against an attack card
     * 
     * Rules:
     * - 2 (Pick Two) can defend against another 2
     * - 5 (Pick Three) can defend against another 5
     * - Cannot cross-defend (2 vs 5 or vice versa)
     */
    bool canDefendAgainstAttack(const core::Card& attackCard,
                                const core::Card& defenseCard) const;
    
    /**
     * Validate double-decking sequence
     * 
     * Rules:
     * - All cards must have the same number
     * - First card must match the call card
     * - Cards change the suit to the last card played
     */
    bool validateDoubleDecking(const std::vector<const core::Card*>& cards,
                               const core::Card& callCard) const;
    
    // ========================================
    // Game Configuration
    // ========================================
    
    /** Default number of cards to deal at start (3-6 configurable) */
    int getDefaultStartingCards() const;
    
    /** Whether double-decking is allowed */
    bool allowDoubleDecking() const;
    
    /** Whether direction can be changed (on Whot or Pick cards) */
    bool allowDirectionChange() const;
    
    /** Whether turn timer is enforced */
    bool enforceTurnTimer() const;
    
    /** Turn time limit in seconds */
    int getTurnTimeSeconds() const;
    
    // ========================================
    // Special Card Rules
    // ========================================
    
    /**
     * Get the number of cards to pick for a Pick Two card
     * Accounts for chaining (multiple 2s played in sequence)
     */
    int getPickTwoCount() const;
    
    /**
     * Get the number of cards to pick for a Pick Three card (5-card)
     * Accounts for chaining (multiple 5s played in sequence)
     */
    int getPickThreeCount() const;
    
    /** Whether Pick cards (2 and 5) can be chained */
    bool canChainPickCards() const;
    
    /**
     * Get all abilities of the Whot card (20)
     * Returns: ["change_suit", "change_direction"]
     */
    std::vector<std::string> getWhotCardAbilities() const;
    
    /**
     * Check if direction change is allowed on Pick 2/3 cards
     * (Nigerian-specific feature)
     */
    bool allowsDirectionChangeOnPickCards() const;
    
    // ========================================
    // Declaration Rules
    // ========================================
    
    /** Whether player must say "Last card" when down to 1 card */
    bool requiresLastCardDeclaration() const;
    
    /** Whether player must say "Check up" when playing last card */
    bool requiresCheckUpDeclaration() const;
    
    /** Number of cards to pick as penalty for missing declaration */
    int getDeclarationPenalty() const;
    
    // ========================================
    // Scoring & Winning
    // ========================================
    
    /**
     * Calculate the score for a hand
     * - Regular cards: face value
     * - Star cards: inner value (doubled)
     * - Whot cards: 20
     */
    int calculateScore(const core::Hand& hand) const;
    
    /**
     * Score threshold for elimination (not used in basic Nigerian rules,
     * but included for multi-round games)
     */
    int getEliminationScore() const;
    
    /** Get the name of this rule variant */
    std::string getRuleName() const;
    
    // ========================================
    // Configuration Setters
    // ========================================
    
    void setStartingCards(int count);
    void setAllowDoubleDecking(bool allow);
    void setAllowDirectionChange(bool allow);
    void setEnforceTurnTimer(bool enforce);
    void setTurnTimeSeconds(int seconds);
    
private:
    // Configuration
    int startingCards_;
    bool allowDoubleDecking_;
    bool allowDirectionChange_;
    bool enforceTurnTimer_;
    int turnTimeSeconds_;
    
    // Helper methods
    bool cardsMatchSuit(const core::Card& card1, const core::Card& card2) const;
    bool cardsMatchValue(const core::Card& card1, const core::Card& card2) const;
};

} // namespace whot::rules

#endif // WHOT_RULES_NIGERIAN_RULES_HPP