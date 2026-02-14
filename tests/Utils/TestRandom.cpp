#include <gtest/gtest.h>
#include "Utils/Random.hpp"

namespace whot::utils {

TEST(TestRandom, GetInstance) {
    Random& R = Random::getInstance();
    (void)R;
}

TEST(TestRandom, NextInt_SingleValue) {
    Random& R = Random::getInstance();
    R.seed(42);
    int v = R.nextInt(5, 5);
    EXPECT_EQ(v, 5);
}

TEST(TestRandom, NextInt_InRange) {
    Random& R = Random::getInstance();
    R.seed(12345);
    for (int i = 0; i < 50; ++i) {
        int v = R.nextInt(0, 10);
        EXPECT_GE(v, 0);
        EXPECT_LE(v, 10);
    }
}

TEST(TestRandom, NextInt_MinEqualsMax) {
    Random& R = Random::getInstance();
    EXPECT_EQ(R.nextInt(0, 0), 0);
    EXPECT_EQ(R.nextInt(-3, -3), -3);
}

TEST(TestRandom, Seed_Reproducibility) {
    Random& R = Random::getInstance();
    R.seed(999);
    int a = R.nextInt(1, 1000);
    R.seed(999);
    int b = R.nextInt(1, 1000);
    EXPECT_EQ(a, b);
}

TEST(TestRandom, NextDouble) {
    Random& R = Random::getInstance();
    R.seed(1);
    double v = R.nextDouble(0.0, 1.0);
    EXPECT_GE(v, 0.0);
    EXPECT_LE(v, 1.0);
}

TEST(TestRandom, NextBool) {
    Random& R = Random::getInstance();
    R.seed(2);
    bool v = R.nextBool(0.0);
    EXPECT_FALSE(v);
    R.seed(3);
    bool w = R.nextBool(1.0);
    EXPECT_TRUE(w);
}

TEST(TestRandom, GenerateId_Length) {
    Random& R = Random::getInstance();
    R.seed(4);
    std::string id = R.generateId(16);
    EXPECT_EQ(id.size(), 16u);
}

TEST(TestRandom, GenerateUUID_Format) {
    Random& R = Random::getInstance();
    R.seed(5);
    std::string u = R.generateUUID();
    EXPECT_EQ(u.size(), 36u);
    EXPECT_EQ(u[8], '-');
    EXPECT_EQ(u[13], '-');
}

} // namespace whot::utils
