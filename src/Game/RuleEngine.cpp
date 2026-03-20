#include "../../include/Game/RuleEngine.hpp"
#include "../../include/Core/GameConstants.hpp"

namespace whot::game {

RuleEngine::RuleEngine() : rules_(std::make_unique<rules::NigerianRules>()) {}

rules::NigerianRules& RuleEngine::getRules() { return *rules_; }
const rules::NigerianRules& RuleEngine::getRules() const { return *rules_; }

bool RuleEngine::hasPlayableCard(const GameState& state,
                                 const core::Player& player) const {
    const core::Card* call = state.getCallCard();
    if (!call) return false;
    std::optional<core::Suit> demanded = state.getDemandedSuit();
    for (size_t i = 0; i < player.getHand().size(); ++i) {
        const core::Card& c = player.getHand().getCard(i);
        if (rules_->canPlayCard(c, *call, player, demanded)) return true;
    }
    return false;
}

bool RuleEngine::hasDefenseCard(const GameState& state,
                                const core::Player& player) const {
    if (state.getActivePickCount() <= 0) return false;
    const core::Card* call = state.getCallCard();
    if (!call) return false;
    for (size_t i = 0; i < player.getHand().size(); ++i) {
        const core::Card& c = player.getHand().getCard(i);
        if (rules_->canDefendAgainstAttack(*call, c)) return true;
    }
    return false;
}

bool RuleEngine::canPlayCard(const GameState& state,
                             const core::Player& player,
                             const core::Card& card) const {
    const core::Card* call = state.getCallCard();
    if (!call) return false;
    return rules_->canPlayCard(card, *call, player, state.getDemandedSuit());
}

bool RuleEngine::mustDrawCard(const GameState& state,
                             const core::Player& player) const {
    if (state.getActivePickCount() > 0)
        return !hasDefenseCard(state, player);
    return !hasPlayableCard(state, player);
}

int RuleEngine::calculateDrawCount(const GameState& state,
                                    const core::Player& player) const {
    if (state.getActivePickCount() > 0 && !hasDefenseCard(state, player))
        return state.getActivePickCount();
    return 1;
}

bool RuleEngine::canDefendAgainstPick(const GameState& state,
                                     const core::Player& player,
                                     const core::Card& attackCard) const {
    (void)attackCard;
    if (state.getActivePickCount() <= 0) return false;
    return hasDefenseCard(state, player);
}

bool RuleEngine::canDoubleDesk(const GameState& state,
                              const core::Player& player,
                              const std::vector<const core::Card*>& cards) const {
    (void)player;
    const core::Card* call = state.getCallCard();
    if (!call) return false;
    return rules_->validateDoubleDecking(cards, *call);
}

bool RuleEngine::requiresLastCardDeclaration(const core::Player& player) const {
    return rules_->requiresLastCardDeclaration() &&
           player.getHand().size() == 2;
}

bool RuleEngine::requiresCheckUpDeclaration(const core::Player& player) const {
    return rules_->requiresCheckUpDeclaration() &&
           player.getHand().size() == 1;
}

int RuleEngine::getDeclarationPenalty() const {
    return rules_->getDeclarationPenalty();
}

int RuleEngine::calculateRoundScore(const core::Player& player) const {
    return rules_->calculateScore(player.getHand());
}

bool RuleEngine::isPlayerEliminated(const core::Player& player) const {
    return player.getCumulativeScore() >= rules_->getEliminationScore();
}

bool RuleEngine::enforceTurnTimer() const { return rules_->enforceTurnTimer(); }
int RuleEngine::getTurnTimeLimit() const { return rules_->getTurnTimeSeconds(); }

} // namespace whot::game
