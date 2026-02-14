#include <gtest/gtest.h>
#include "Utils/Validation.hpp"

namespace whot::utils {

TEST(TestValidation, IsNotEmpty) {
    EXPECT_FALSE(Validator::isNotEmpty(""));
    EXPECT_TRUE(Validator::isNotEmpty("a"));
}

TEST(TestValidation, HasMinLength) {
    EXPECT_FALSE(Validator::hasMinLength("", 1));
    EXPECT_TRUE(Validator::hasMinLength("a", 1));
    EXPECT_FALSE(Validator::hasMinLength("a", 2));
}

TEST(TestValidation, HasMaxLength) {
    EXPECT_TRUE(Validator::hasMaxLength("a", 1));
    EXPECT_FALSE(Validator::hasMaxLength("ab", 1));
}

TEST(TestValidation, IsAlphanumeric) {
    EXPECT_FALSE(Validator::isAlphanumeric(""));
    EXPECT_TRUE(Validator::isAlphanumeric("abc123"));
    EXPECT_FALSE(Validator::isAlphanumeric("a b"));
}

TEST(TestValidation, MatchesPattern) {
    EXPECT_TRUE(Validator::matchesPattern("abc", "[a-z]+"));
    EXPECT_FALSE(Validator::matchesPattern("123", "[a-z]+"));
}

TEST(TestValidation, MatchesPattern_InvalidRegex) {
    EXPECT_FALSE(Validator::matchesPattern("x", "[invalid"));
}

TEST(TestValidation, IsInRange) {
    EXPECT_TRUE(Validator::isInRange(5, 0, 10));
    EXPECT_TRUE(Validator::isInRange(0, 0, 10));
    EXPECT_FALSE(Validator::isInRange(11, 0, 10));
}

TEST(TestValidation, IsPositive) {
    EXPECT_TRUE(Validator::isPositive(1));
    EXPECT_FALSE(Validator::isPositive(0));
}

TEST(TestValidation, HasMinSize_HasMaxSize) {
    std::vector<int> two = {1, 2};
    EXPECT_TRUE(Validator::hasMinSize(two, 2));
    EXPECT_TRUE(Validator::hasMaxSize(two, 2));
    EXPECT_FALSE(Validator::hasMaxSize(two, 1));
}

TEST(TestValidation, Validate_CustomRule) {
    auto nonEmpty = [](const std::string& s) { return !s.empty(); };
    EXPECT_TRUE(Validator::validate("x", nonEmpty));
    EXPECT_FALSE(Validator::validate("", nonEmpty));
}

TEST(TestValidation, ValidationResult) {
    ValidationResult r;
    EXPECT_TRUE(r.isValid());
    ValidationError e1;
    e1.field = "f1";
    e1.message = "msg1";
    e1.code = "code1";
    r.addError(e1);
    EXPECT_FALSE(r.isValid());
    EXPECT_EQ(r.getErrors().size(), 1u);
    EXPECT_FALSE(r.getErrorMessage().empty());
}

} // namespace whot::utils
