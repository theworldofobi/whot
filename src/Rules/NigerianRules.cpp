#include "../../include/Rules/NigerianRules.hpp"
#include "../../include/Core/GameConstants.hpp"

namespace whot::rules {

NigerianRules::NigerianRules()
    : startingCards_(core::DEFAULT_STARTING_CARDS)
    , allowDoubleDecking_(false)
    , allowDirectionChange_(true)
    , enforceTurnTimer_(false)
    , turnTimeSeconds_(core::DEFAULT_TURN_TIME_SECONDS)
{}

bool NigerianRules::cardsMatchSuit(const core::Card& card1, const core::Card& card2) const {
    return card1.getSuit() == card2.getSuit();
}

bool NigerianRules::cardsMatchValue(const core::Card& card1, const core::Card& card2) const {
    if (card1.isStarCard() && card2.isStarCard())
        return card1.getOuterStarValue() == card2.getOuterStarValue();
    return card1.getValue() == card2.getValue();
}

bool NigerianRules::canPlayCard(const core::Card& card,
                                const core::Card& callCard,
                                const core::Player& player,
                                std::optional<core::Suit> demandedSuit) const {
    (void)player;
    // Whot cards can play on anything
    if (card.isWhotCard()) return true;
    // If a suit is demanded (after Whot), must match that suit
    if (demandedSuit.has_value())
        return card.matchesSuit(demandedSuit.value());
    // Otherwise match suit OR number
    return card.canPlayOn(callCard);
}

bool NigerianRules::canDefendAgainstAttack(const core::Card& attackCard,
                                           const core::Card& defenseCard) const {
    // 2 can only defend against 2, 5 only against 5
    if (attackCard.getValue() == core::CardValue::TWO)
        return defenseCard.getValue() == core::CardValue::TWO;
    if (attackCard.getValue() == core::CardValue::FIVE)
        return defenseCard.getValue() == core::CardValue::FIVE;
    return false;
}

bool NigerianRules::validateDoubleDecking(const std::vector<const core::Card*>& cards,
                                          const core::Card& callCard) const {
    if (cards.empty()) return false;
    if (!allowDoubleDecking_) return false;
    // First card must match the call card
    if (!cards.front()->canPlayOn(callCard)) return false;
    int firstNum = cards.front()->isStarCard()
        ? cards.front()->getOuterStarValue()
        : cards.front()->getNumericValue();
    for (size_t i = 1; i < cards.size(); ++i) {
        int num = cards[i]->isStarCard()
            ? cards[i]->getOuterStarValue()
            : cards[i]->getNumericValue();
        if (num != firstNum) return false;
    }
    return true;
}

int NigerianRules::getDefaultStartingCards() const { return startingCards_; }
bool NigerianRules::allowDoubleDecking() const { return allowDoubleDecking_; }
bool NigerianRules::allowDirectionChange() const { return allowDirectionChange_; }
bool NigerianRules::enforceTurnTimer() const { return enforceTurnTimer_; }
int NigerianRules::getTurnTimeSeconds() const { return turnTimeSeconds_; }

int NigerianRules::getPickTwoCount() const { return 2; }
int NigerianRules::getPickThreeCount() const { return 3; }
bool NigerianRules::canChainPickCards() const { return true; }

std::vector<std::string> NigerianRules::getWhotCardAbilities() const {
    return {"change_suit", "change_direction"};
}

bool NigerianRules::allowsDirectionChangeOnPickCards() const { return allowDirectionChange_; }

bool NigerianRules::requiresLastCardDeclaration() const { return true; }
bool NigerianRules::requiresCheckUpDeclaration() const { return true; }
int NigerianRules::getDeclarationPenalty() const { return 2; }

int NigerianRules::calculateScore(const core::Hand& hand) const {
    return hand.calculateTotalScore();
}

int NigerianRules::getEliminationScore() const { return core::ELIMINATION_SCORE; }
std::string NigerianRules::getRuleName() const { return "Nigerian Whot"; }

void NigerianRules::setStartingCards(int count) {
    if (count >= core::NIGERIAN_MIN_CARDS && count <= core::NIGERIAN_MAX_CARDS)
        startingCards_ = count;
}
void NigerianRules::setAllowDoubleDecking(bool allow) { allowDoubleDecking_ = allow; }
void NigerianRules::setAllowDirectionChange(bool allow) { allowDirectionChange_ = allow; }
void NigerianRules::setEnforceTurnTimer(bool enforce) { enforceTurnTimer_ = enforce; }
void NigerianRules::setTurnTimeSeconds(int seconds) { turnTimeSeconds_ = seconds; }

} // namespace whot::rules
