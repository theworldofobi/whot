#include "../../include/AI/Strategy.hpp"
#include "../../include/Core/GameConstants.hpp"
#include "../../include/Core/Hand.hpp"
#include "../../include/Utils/Random.hpp"
#include <algorithm>
#include <limits>

namespace whot::ai {

int Strategy::evaluateCardValue(const core::Card& card, const game::GameState& state) const {
    (void)state;
    core::SpecialAbility ab = card.getSpecialAbility();
    if (ab == core::SpecialAbility::GENERAL_MARKET) return 14;
    if (ab == core::SpecialAbility::HOLD_ON) return 10;
    if (card.getValue() == core::CardValue::EIGHT) return 8;
    if (ab == core::SpecialAbility::PICK_TWO) return 5;
    if (card.getValue() == core::CardValue::FIVE) return 4;
    if (ab == core::SpecialAbility::WHOT_CARD) return 12;
    return card.getNumericValue();
}

core::Player* Strategy::getPlayerClosestToWinning(const game::GameState& state) const {
    core::Player* best = nullptr;
    size_t minSize = std::numeric_limits<size_t>::max();
    for (core::Player* p : state.getActivePlayers()) {
        if (!p) continue;
        size_t s = p->getHand().size();
        if (s < minSize) { minSize = s; best = p; }
    }
    return best;
}

std::map<core::Suit, int> Strategy::countSuits(const core::Hand& hand) const {
    std::map<core::Suit, int> counts;
    for (core::Suit s : {core::Suit::CIRCLE, core::Suit::TRIANGLE, core::Suit::CROSS,
                         core::Suit::BLOCK, core::Suit::STAR})
        counts[s] = 0;
    for (size_t i = 0; i < hand.size(); ++i)
        counts[hand.getCard(i).getSuit()]++;
    return counts;
}

// --- RandomStrategy ---
size_t RandomStrategy::selectCard(const game::GameState& state,
                                  const std::vector<size_t>& playableCards) {
    (void)state;
    if (playableCards.empty()) return 0;
    return playableCards[whot::utils::Random::getInstance().nextInt(0, static_cast<int>(playableCards.size()) - 1)];
}

core::Suit RandomStrategy::selectSuit(const game::GameState& state) {
    (void)state;
    core::Suit suits[] = {core::Suit::CIRCLE, core::Suit::TRIANGLE, core::Suit::CROSS, core::Suit::BLOCK, core::Suit::STAR};
    return suits[whot::utils::Random::getInstance().nextInt(0, 4)];
}

bool RandomStrategy::shouldDefend(const game::GameState& state,
                                  const core::Card& attackCard,
                                  const std::vector<size_t>& defenseCards) {
    (void)state; (void)attackCard;
    if (defenseCards.empty()) return false;
    return whot::utils::Random::getInstance().nextBool(0.5);
}

std::string RandomStrategy::getStrategyName() const { return "Random"; }

// --- AggressiveStrategy ---
size_t AggressiveStrategy::selectCard(const game::GameState& state,
                                      const std::vector<size_t>& playableCards) {
    if (playableCards.empty()) return 0;
    size_t best = playableCards[0];
    int bestVal = -1;
    const core::Player* cur = state.getCurrentPlayer();
    if (!cur) return playableCards[0];
    for (size_t idx : playableCards) {
        const core::Card& c = cur->getHand().getCard(idx);
        int v = evaluateCardValue(c, state);
        if (v > bestVal) { bestVal = v; best = idx; }
    }
    return best;
}

core::Suit AggressiveStrategy::selectSuit(const game::GameState& state) {
    const core::Player* cur = state.getCurrentPlayer();
    if (!cur) return core::Suit::CIRCLE;
    auto counts = countSuits(cur->getHand());
    core::Suit best = core::Suit::CIRCLE;
    int maxCount = -1;
    for (const auto& [s, n] : counts)
        if (n > maxCount) { maxCount = n; best = s; }
    return best;
}

bool AggressiveStrategy::shouldDefend(const game::GameState& state,
                                      const core::Card& attackCard,
                                      const std::vector<size_t>& defenseCards) {
    (void)state; (void)attackCard;
    if (defenseCards.empty()) return false;
    return true;
}

std::string AggressiveStrategy::getStrategyName() const { return "Aggressive"; }

// --- DefensiveStrategy ---
size_t DefensiveStrategy::selectCard(const game::GameState& state,
                                      const std::vector<size_t>& playableCards) {
    if (playableCards.empty()) return 0;
    size_t best = playableCards[0];
    int bestVal = 999;
    const core::Player* cur = state.getCurrentPlayer();
    if (!cur) return playableCards[0];
    for (size_t idx : playableCards) {
        const core::Card& c = cur->getHand().getCard(idx);
        int v = c.getScoreValue();
        if (c.getSpecialAbility() == core::SpecialAbility::WHOT_CARD) v = 20;
        if (c.getSpecialAbility() == core::SpecialAbility::PICK_TWO || c.getValue() == core::CardValue::FIVE)
            v = -10;
        if (v < bestVal) { bestVal = v; best = idx; }
    }
    return best;
}

core::Suit DefensiveStrategy::selectSuit(const game::GameState& state) {
    const core::Player* cur = state.getCurrentPlayer();
    if (!cur) return core::Suit::CIRCLE;
    auto counts = countSuits(cur->getHand());
    core::Suit best = core::Suit::CIRCLE;
    int maxCount = -1;
    for (const auto& [s, n] : counts)
        if (n > maxCount) { maxCount = n; best = s; }
    return best;
}

bool DefensiveStrategy::shouldDefend(const game::GameState& state,
                                      const core::Card& attackCard,
                                      const std::vector<size_t>& defenseCards) {
    (void)state; (void)attackCard;
    if (defenseCards.empty()) return false;
    return true;
}

std::string DefensiveStrategy::getStrategyName() const { return "Defensive"; }

// --- BalancedStrategy ---
size_t BalancedStrategy::selectCard(const game::GameState& state,
                                     const std::vector<size_t>& playableCards) {
    if (playableCards.empty()) return 0;
    const core::Player* cur = state.getCurrentPlayer();
    if (!cur) return playableCards[0];
    bool useAggressive = cur->getHand().size() <= 3;
    if (useAggressive) {
        size_t best = playableCards[0];
        int bestVal = -1;
        for (size_t idx : playableCards) {
            int v = evaluateCardValue(cur->getHand().getCard(idx), state);
            if (v > bestVal) { bestVal = v; best = idx; }
        }
        return best;
    }
    size_t best = playableCards[0];
    int bestVal = 999;
    for (size_t idx : playableCards) {
        const core::Card& c = cur->getHand().getCard(idx);
        int v = c.getScoreValue();
        if (v < bestVal) { bestVal = v; best = idx; }
    }
    return best;
}

core::Suit BalancedStrategy::selectSuit(const game::GameState& state) {
    const core::Player* cur = state.getCurrentPlayer();
    if (!cur) return core::Suit::CIRCLE;
    auto counts = countSuits(cur->getHand());
    core::Suit best = core::Suit::CIRCLE;
    int maxCount = -1;
    for (const auto& [s, n] : counts)
        if (n > maxCount) { maxCount = n; best = s; }
    return best;
}

bool BalancedStrategy::shouldDefend(const game::GameState& state,
                                    const core::Card& attackCard,
                                    const std::vector<size_t>& defenseCards) {
    (void)state; (void)attackCard;
    return !defenseCards.empty();
}

std::string BalancedStrategy::getStrategyName() const { return "Balanced"; }

} // namespace whot::ai
