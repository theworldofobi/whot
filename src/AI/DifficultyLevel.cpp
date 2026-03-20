#include "../../include/AI/DifficultyLevel.hpp"

namespace whot::ai {

DifficultyConfig::Config DifficultyConfig::getConfig(DifficultyLevel level) {
    switch (level) {
        case DifficultyLevel::EASY:
            return {2000, 0.7, 0.3};
        case DifficultyLevel::MEDIUM:
            return {1000, 0.3, 0.1};
        case DifficultyLevel::HARD:
            return {500, 0.0, 0.0};
        default:
            return {1000, 0.3, 0.1};
    }
}

std::unique_ptr<Strategy> DifficultyConfig::createStrategy(DifficultyLevel level) {
    switch (level) {
        case DifficultyLevel::EASY:
            return std::make_unique<RandomStrategy>();
        case DifficultyLevel::MEDIUM:
            return std::make_unique<BalancedStrategy>();
        case DifficultyLevel::HARD:
            return std::make_unique<AggressiveStrategy>();
        default:
            return std::make_unique<BalancedStrategy>();
    }
}

int DifficultyConfig::getThinkingDelay(DifficultyLevel level) {
    return getConfig(level).thinkingDelayMs;
}

double DifficultyConfig::getRandomnessFactor(DifficultyLevel level) {
    return getConfig(level).randomnessFactor;
}

bool DifficultyConfig::shouldMakeMistakes(DifficultyLevel level) {
    return getConfig(level).mistakeProbability > 0.0;
}

double DifficultyConfig::getMistakeProbability(DifficultyLevel level) {
    return getConfig(level).mistakeProbability;
}

} // namespace whot::ai
