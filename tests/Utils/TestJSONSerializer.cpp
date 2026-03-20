#include <gtest/gtest.h>
#include "Utils/JSONSerializer.hpp"

namespace whot::utils {

TEST(TestJSONSerializer, SerializeDeserializeObject_RoundTrip) {
    std::map<std::string, std::string> obj = {{"a", "1"}, {"b", "2"}};
    std::string json = JsonSerializer::serialize(obj);
    auto restored = JsonSerializer::deserializeObject(json);
    EXPECT_EQ(restored.size(), 2u);
    EXPECT_EQ(restored["a"], "1");
    EXPECT_EQ(restored["b"], "2");
}

TEST(TestJSONSerializer, SerializeDeserializeArray_RoundTrip) {
    std::vector<std::string> arr = {"x", "y"};
    std::string json = JsonSerializer::serialize(arr);
    auto restored = JsonSerializer::deserializeArray(json);
    EXPECT_EQ(restored.size(), 2u);
    EXPECT_EQ(restored[0], "x");
    EXPECT_EQ(restored[1], "y");
}

TEST(TestJSONSerializer, DeserializeObject_Malformed_ReturnsEmpty) {
    auto out = JsonSerializer::deserializeObject("not json");
    EXPECT_TRUE(out.empty());
}

TEST(TestJSONSerializer, DeserializeObject_EmptyString_ReturnsEmpty) {
    auto out = JsonSerializer::deserializeObject("");
    EXPECT_TRUE(out.empty());
}

TEST(TestJSONSerializer, GetString_MissingKey) {
    auto v = JsonSerializer::getString("{\"a\": \"x\"}", "b");
    EXPECT_FALSE(v.has_value());
}

TEST(TestJSONSerializer, GetString_Present) {
    auto v = JsonSerializer::getString("{\"a\": \"x\"}", "a");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, "x");
}

TEST(TestJSONSerializer, GetString_WrongType) {
    auto v = JsonSerializer::getString("{\"a\": 42}", "a");
    EXPECT_FALSE(v.has_value());
}

TEST(TestJSONSerializer, GetInt_MissingKey) {
    auto v = JsonSerializer::getInt("{\"a\": 1}", "b");
    EXPECT_FALSE(v.has_value());
}

TEST(TestJSONSerializer, GetInt_Present) {
    auto v = JsonSerializer::getInt("{\"a\": 42}", "a");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, 42);
}

TEST(TestJSONSerializer, GetBool_Present) {
    auto v = JsonSerializer::getBool("{\"a\": true}", "a");
    ASSERT_TRUE(v.has_value());
    EXPECT_TRUE(*v);
}

TEST(TestJSONSerializer, IsValidJson) {
    EXPECT_FALSE(JsonSerializer::isValidJson(""));
    EXPECT_FALSE(JsonSerializer::isValidJson("not json"));
    EXPECT_TRUE(JsonSerializer::isValidJson("{}"));
    EXPECT_TRUE(JsonSerializer::isValidJson("[]"));
    EXPECT_TRUE(JsonSerializer::isValidJson("{\"a\": 1}"));
}

TEST(TestJSONSerializer, CreateStringIntBoolArray) {
    std::string s = JsonSerializer::createString("k", "v");
    EXPECT_TRUE(JsonSerializer::isValidJson(s));
    std::string i = JsonSerializer::createInt("k", 42);
    EXPECT_TRUE(JsonSerializer::isValidJson(i));
    std::string b = JsonSerializer::createBool("k", true);
    EXPECT_TRUE(JsonSerializer::isValidJson(b));
    std::string a = JsonSerializer::createArray("k", {"a", "b"});
    EXPECT_TRUE(JsonSerializer::isValidJson(a));
}

TEST(TestJSONSerializer, EscapeUnescape) {
    std::string raw = "hello";
    std::string escaped = JsonSerializer::escape(raw);
    EXPECT_FALSE(escaped.empty());
    std::string unescaped = JsonSerializer::unescape(escaped);
    EXPECT_EQ(unescaped, raw);
}

TEST(TestJSONSerializer, Unescape_NotQuoted_ReturnsSame) {
    std::string s = "no quotes";
    EXPECT_EQ(JsonSerializer::unescape(s), s);
}

} // namespace whot::utils
