#include "../../include/Utils/Validation.hpp"
#include <regex>
#include <algorithm>
#include <sstream>

namespace whot::utils {

bool Validator::isNotEmpty(const std::string& str) {
    return !str.empty();
}

bool Validator::hasMinLength(const std::string& str, size_t minLength) {
    return str.size() >= minLength;
}

bool Validator::hasMaxLength(const std::string& str, size_t maxLength) {
    return str.size() <= maxLength;
}

bool Validator::isAlphanumeric(const std::string& str) {
    if (str.empty()) return false;
    return std::all_of(str.begin(), str.end(), [](unsigned char c) {
        return std::isalnum(c);
    });
}

bool Validator::matchesPattern(const std::string& str, const std::string& pattern) {
    try {
        std::regex re(pattern);
        return std::regex_match(str, re);
    } catch (...) {
        return false;
    }
}

bool Validator::isInRange(int value, int min, int max) {
    return value >= min && value <= max;
}

bool Validator::isPositive(int value) {
    return value > 0;
}

bool Validator::validate(const std::string& value, ValidationRule rule) {
    return rule(value);
}

ValidationResult::ValidationResult() = default;

bool ValidationResult::isValid() const {
    return errors_.empty();
}

void ValidationResult::addError(const ValidationError& error) {
    errors_.push_back(error);
}

const std::vector<ValidationError>& ValidationResult::getErrors() const {
    return errors_;
}

std::string ValidationResult::getErrorMessage() const {
    std::ostringstream oss;
    for (size_t i = 0; i < errors_.size(); ++i) {
        if (i > 0) oss << "; ";
        oss << errors_[i].field;
        if (!errors_[i].message.empty()) oss << ": " << errors_[i].message;
    }
    return oss.str();
}

} // namespace whot::utils
