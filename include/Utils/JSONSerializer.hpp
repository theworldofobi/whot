#ifndef WHOT_UTILS_JSON_SERIALIZER_HPP
#define WHOT_UTILS_JSON_SERIALIZER_HPP

#include <string>
#include <map>
#include <vector>
#include <optional>

namespace whot::utils {

class JsonSerializer {
public:
    // Object serialization
    static std::string serialize(const std::map<std::string, std::string>& obj);
    static std::map<std::string, std::string> deserializeObject(
        const std::string& json);
    
    // Array serialization
    static std::string serialize(const std::vector<std::string>& arr);
    static std::vector<std::string> deserializeArray(const std::string& json);
    
    // Value extraction
    static std::optional<std::string> getString(const std::string& json,
                                                 const std::string& key);
    static std::optional<int> getInt(const std::string& json,
                                     const std::string& key);
    static std::optional<bool> getBool(const std::string& json,
                                       const std::string& key);
    
    // Value creation
    static std::string createString(const std::string& key,
                                     const std::string& value);
    static std::string createInt(const std::string& key, int value);
    static std::string createBool(const std::string& key, bool value);
    static std::string createArray(const std::string& key,
                                    const std::vector<std::string>& values);
    
    // Validation
    static bool isValidJson(const std::string& json);
    static std::string escape(const std::string& str);
    static std::string unescape(const std::string& str);
};

} // namespace whot::utils

#endif // WHOT_UTILS_JSON_SERIALIZER_HPP
