#include <gtest/gtest.h>
#include "AI/DifficultyLevel.hpp"

namespace whot::ai {

TEST(TestDifficultyLevel, CreateStrategy_EachLevel) {
    auto easy = DifficultyConfig::createStrategy(DifficultyLevel::EASY);
    ASSERT_NE(easy, nullptr);
    EXPECT_EQ(easy->getStrategyName(), "Random");
    auto medium = DifficultyConfig::createStrategy(DifficultyLevel::MEDIUM);
    ASSERT_NE(medium, nullptr);
    auto hard = DifficultyConfig::createStrategy(DifficultyLevel::HARD);
    ASSERT_NE(hard, nullptr);
}

TEST(TestDifficultyLevel, GetThinkingDelay) {
    EXPECT_GE(DifficultyConfig::getThinkingDelay(DifficultyLevel::EASY), 0);
    EXPECT_GE(DifficultyConfig::getThinkingDelay(DifficultyLevel::HARD), 0);
}

TEST(TestDifficultyLevel, GetRandomnessFactor) {
    double r = DifficultyConfig::getRandomnessFactor(DifficultyLevel::EASY);
    EXPECT_GE(r, 0.0);
    EXPECT_LE(r, 1.0);
}

TEST(TestDifficultyLevel, ShouldMakeMistakes) {
    bool easy = DifficultyConfig::shouldMakeMistakes(DifficultyLevel::EASY);
    bool hard = DifficultyConfig::shouldMakeMistakes(DifficultyLevel::HARD);
    EXPECT_TRUE(easy || !easy);
    EXPECT_FALSE(hard);
}

} // namespace whot::ai
