#ifndef WHOT_UTILS_VALIDATION_HPP
#define WHOT_UTILS_VALIDATION_HPP

#include <string>
#include <vector>
#include <functional>

namespace whot::utils {

struct ValidationError {
    std::string field;
    std::string message;
    std::string code;
};

class Validator {
public:
    // String validation
    static bool isNotEmpty(const std::string& str);
    static bool hasMinLength(const std::string& str, size_t minLength);
    static bool hasMaxLength(const std::string& str, size_t maxLength);
    static bool isAlphanumeric(const std::string& str);
    static bool matchesPattern(const std::string& str, const std::string& pattern);
    
    // Numeric validation
    static bool isInRange(int value, int min, int max);
    static bool isPositive(int value);
    
    // Collection validation
    template<typename T>
    static bool hasMinSize(const std::vector<T>& vec, size_t minSize) {
        return vec.size() >= minSize;
    }
    template<typename T>
    static bool hasMaxSize(const std::vector<T>& vec, size_t maxSize) {
        return vec.size() <= maxSize;
    }
    
    // Custom validation
    using ValidationRule = std::function<bool(const std::string&)>;
    static bool validate(const std::string& value, ValidationRule rule);
};

class ValidationResult {
public:
    ValidationResult();
    
    bool isValid() const;
    void addError(const ValidationError& error);
    const std::vector<ValidationError>& getErrors() const;
    std::string getErrorMessage() const;
    
private:
    std::vector<ValidationError> errors_;
};

} // namespace whot::utils

#endif // WHOT_UTILS_VALIDATION_HPP
