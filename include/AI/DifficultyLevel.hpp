#ifndef WHOT_AI_DIFFICULTY_LEVEL_HPP
#define WHOT_AI_DIFFICULTY_LEVEL_HPP

#include "Strategy.hpp"
#include <memory>

namespace whot::ai {

enum class DifficultyLevel {
    EASY,
    MEDIUM,
    HARD
};

class DifficultyConfig {
public:
    static std::unique_ptr<Strategy> createStrategy(DifficultyLevel level);
    
    static int getThinkingDelay(DifficultyLevel level);
    static double getRandomnessFactor(DifficultyLevel level);
    static bool shouldMakeMistakes(DifficultyLevel level);
    static double getMistakeProbability(DifficultyLevel level);
    
private:
    struct Config {
        int thinkingDelayMs;
        double randomnessFactor;
        double mistakeProbability;
    };
    
    static Config getConfig(DifficultyLevel level);
};

} // namespace whot::ai

#endif // WHOT_AI_DIFFICULTY_LEVEL_HPP
