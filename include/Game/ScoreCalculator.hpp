#ifndef WHOT_GAME_SCORE_CALCULATOR_HPP
#define WHOT_GAME_SCORE_CALCULATOR_HPP

#include "Core/Player.hpp"
#include <vector>
#include <map>

namespace whot::game {

struct RoundScores {
    std::map<std::string, int> playerScores;
    std::string winnerId;
    std::vector<std::string> eliminatedPlayerIds;
};

struct GameScores {
    std::map<std::string, int> cumulativeScores;
    std::map<std::string, int> gamesWon;
    std::string overallWinnerId;
};

class ScoreCalculator {
public:
    // Round scoring
    static RoundScores calculateRoundScores(
        const std::vector<core::Player*>& players);
    
    static int calculateHandScore(const core::Hand& hand);
    
    // Game-level scoring
    static GameScores calculateGameScores(
        const std::vector<core::Player*>& players);
    
    // Elimination checking
    static std::vector<std::string> checkEliminatedPlayers(
        const std::vector<core::Player*>& players,
        int eliminationThreshold);
    
    // Winner determination
    static std::string determineRoundWinner(
        const std::vector<core::Player*>& players);
    
    static std::string determineGameWinner(
        const std::vector<core::Player*>& players);
};

} // namespace whot::game

#endif // WHOT_GAME_SCORE_CALCULATOR_HPP