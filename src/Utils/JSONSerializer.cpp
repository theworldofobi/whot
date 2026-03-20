#include "../../include/Utils/JSONSerializer.hpp"
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

namespace whot::utils {

using json = nlohmann::json;

std::string JsonSerializer::serialize(const std::map<std::string, std::string>& obj) {
    json j;
    for (const auto& [k, v] : obj) j[k] = v;
    return j.dump();
}

std::map<std::string, std::string> JsonSerializer::deserializeObject(const std::string& jsonStr) {
    std::map<std::string, std::string> out;
    if (!isValidJson(jsonStr)) return out;
    try {
        json j = json::parse(jsonStr);
        if (!j.is_object()) return out;
        for (auto it = j.begin(); it != j.end(); ++it) {
            if (it.value().is_string())
                out[it.key()] = it.value().get<std::string>();
            else
                out[it.key()] = it.value().dump();
        }
    } catch (...) {}
    return out;
}

std::string JsonSerializer::serialize(const std::vector<std::string>& arr) {
    json j = arr;
    return j.dump();
}

std::vector<std::string> JsonSerializer::deserializeArray(const std::string& jsonStr) {
    std::vector<std::string> out;
    if (!isValidJson(jsonStr)) return out;
    try {
        json j = json::parse(jsonStr);
        if (!j.is_array()) return out;
        for (const auto& item : j) {
            if (item.is_string())
                out.push_back(item.get<std::string>());
            else
                out.push_back(item.dump());
        }
    } catch (...) {}
    return out;
}

std::optional<std::string> JsonSerializer::getString(const std::string& jsonStr, const std::string& key) {
    if (!isValidJson(jsonStr)) return std::nullopt;
    try {
        json j = json::parse(jsonStr);
        if (j.contains(key) && j[key].is_string())
            return j[key].get<std::string>();
    } catch (...) {}
    return std::nullopt;
}

std::optional<int> JsonSerializer::getInt(const std::string& jsonStr, const std::string& key) {
    if (!isValidJson(jsonStr)) return std::nullopt;
    try {
        json j = json::parse(jsonStr);
        if (j.contains(key) && j[key].is_number_integer())
            return j[key].get<int>();
    } catch (...) {}
    return std::nullopt;
}

std::optional<bool> JsonSerializer::getBool(const std::string& jsonStr, const std::string& key) {
    if (!isValidJson(jsonStr)) return std::nullopt;
    try {
        json j = json::parse(jsonStr);
        if (j.contains(key) && j[key].is_boolean())
            return j[key].get<bool>();
    } catch (...) {}
    return std::nullopt;
}

std::string JsonSerializer::createString(const std::string& key, const std::string& value) {
    json j; j[key] = value; return j.dump();
}

std::string JsonSerializer::createInt(const std::string& key, int value) {
    json j; j[key] = value; return j.dump();
}

std::string JsonSerializer::createBool(const std::string& key, bool value) {
    json j; j[key] = value; return j.dump();
}

std::string JsonSerializer::createArray(const std::string& key, const std::vector<std::string>& values) {
    json j; j[key] = values; return j.dump();
}

bool JsonSerializer::isValidJson(const std::string& jsonStr) {
    if (jsonStr.empty()) return false;
    try {
        auto unused = json::parse(jsonStr);
        (void)unused;
        return true;
    } catch (...) {
        return false;
    }
}

std::string JsonSerializer::escape(const std::string& str) {
    return json(str).dump();
}

std::string JsonSerializer::unescape(const std::string& str) {
    if (str.size() < 2 || str.front() != '"' || str.back() != '"') return str;
    try {
        return json::parse(str).get<std::string>();
    } catch (...) {
        return str;
    }
}

} // namespace whot::utils
