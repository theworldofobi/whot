#include <gtest/gtest.h>
#include "Game/ScoreCalculator.hpp"
#include "Core/Player.hpp"
#include "Core/Hand.hpp"
#include "Core/Card.hpp"

namespace whot::game {

TEST(TestScoreCalculator, CalculateHandScore_Empty) {
    core::Hand hand;
    EXPECT_EQ(ScoreCalculator::calculateHandScore(hand), 0);
}

TEST(TestScoreCalculator, CalculateHandScore_WithCards) {
    core::Hand hand;
    hand.addCard(std::make_unique<core::Card>(core::Suit::CIRCLE, core::CardValue::FIVE));
    hand.addCard(std::make_unique<core::Card>(core::Suit::STAR, core::CardValue::FIVE));
    int score = ScoreCalculator::calculateHandScore(hand);
    EXPECT_GE(score, 15);
}

TEST(TestScoreCalculator, CalculateRoundScores) {
    core::Player p1("p1", "P1", core::PlayerType::HUMAN);
    core::Player p2("p2", "P2", core::PlayerType::HUMAN);
    p1.addToScore(10);
    p2.addToScore(5);
    std::vector<core::Player*> players = {&p1, &p2};
    RoundScores rs = ScoreCalculator::calculateRoundScores(players);
    EXPECT_EQ(rs.playerScores["p1"], 10);
    EXPECT_EQ(rs.playerScores["p2"], 5);
}

TEST(TestScoreCalculator, CheckEliminatedPlayers) {
    core::Player p1("p1", "P1", core::PlayerType::HUMAN);
    p1.addToScore(100);
    std::vector<core::Player*> players = {&p1};
    auto eliminated = ScoreCalculator::checkEliminatedPlayers(players, 100);
    EXPECT_EQ(eliminated.size(), 1u);
    EXPECT_EQ(eliminated[0], "p1");
}

TEST(TestScoreCalculator, DetermineRoundWinner) {
    core::Player p1("p1", "P1", core::PlayerType::HUMAN);
    core::Player p2("p2", "P2", core::PlayerType::HUMAN);
    p2.getHand().addCard(std::make_unique<core::Card>(core::Suit::CIRCLE, core::CardValue::ONE));
    std::vector<core::Player*> players = {&p1, &p2};
    std::string winner = ScoreCalculator::determineRoundWinner(players);
    EXPECT_EQ(winner, "p1");
}

} // namespace whot::game
