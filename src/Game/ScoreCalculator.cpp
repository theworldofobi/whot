#include "../../include/Game/ScoreCalculator.hpp"
#include "../../include/Core/Hand.hpp"
#include "../../include/Core/GameConstants.hpp"
#include <algorithm>
#include <limits>

namespace whot::game {

int ScoreCalculator::calculateHandScore(const core::Hand& hand) {
    return hand.calculateTotalScore();
}

RoundScores ScoreCalculator::calculateRoundScores(
    const std::vector<core::Player*>& players) {
    RoundScores out;
    std::string winnerId = determineRoundWinner(players);
    out.winnerId = winnerId;
    for (core::Player* p : players) {
        if (!p) continue;
        out.playerScores[p->getId()] = p->getCurrentScore();
    }
    return out;
}

GameScores ScoreCalculator::calculateGameScores(
    const std::vector<core::Player*>& players) {
    GameScores out;
    out.overallWinnerId = determineGameWinner(players);
    for (core::Player* p : players) {
        if (!p) continue;
        out.cumulativeScores[p->getId()] = p->getCumulativeScore();
        out.gamesWon[p->getId()] = p->getGamesWon();
    }
    return out;
}

std::vector<std::string> ScoreCalculator::checkEliminatedPlayers(
    const std::vector<core::Player*>& players,
    int eliminationThreshold) {
    std::vector<std::string> out;
    for (core::Player* p : players) {
        if (p && p->getCumulativeScore() >= eliminationThreshold)
            out.push_back(p->getId());
    }
    return out;
}

std::string ScoreCalculator::determineRoundWinner(
    const std::vector<core::Player*>& players) {
    for (core::Player* p : players)
        if (p && p->getHand().isEmpty()) return p->getId();
    int minScore = std::numeric_limits<int>::max();
    std::string id;
    for (core::Player* p : players) {
        if (!p) continue;
        int s = p->getCurrentScore();
        if (s < minScore) { minScore = s; id = p->getId(); }
    }
    return id;
}

std::string ScoreCalculator::determineGameWinner(
    const std::vector<core::Player*>& players) {
    int maxWon = -1;
    std::string id;
    for (core::Player* p : players) {
        if (!p) continue;
        int w = p->getGamesWon();
        if (w > maxWon) { maxWon = w; id = p->getId(); }
    }
    return id;
}

} // namespace whot::game
