#include "../../include/Core/GameConstants.hpp"
#include <string>

namespace whot::core {

std::string suitToString(Suit suit) 
{
    using namespace std::literals;
    std::string str = "";
    switch (suit) {
    case Suit::CIRCLE:
        str = "CIRCLE"s;
        break;
    case Suit::TRIANGLE:
        str = "TRIANGLE"s;
        break;
    case Suit::CROSS:
        str = "CROSS"s;
        break;
    case Suit::BLOCK:
        str = "BLOCK"s;
        break;
    case Suit::STAR:
        str = "STAR"s;
        break;
    case Suit::WHOT:
        str = "WHOT"s;
        break;
    }
    return str;
}

std::string cardValueToString(CardValue value) 
{
    return std::to_string(static_cast<int>(value));
}

Suit stringToSuit(const std::string& str)
{
    if (str == "CIRCLE")   return Suit::CIRCLE;
    if (str == "TRIANGLE") return Suit::TRIANGLE;
    if (str == "CROSS")    return Suit::CROSS;
    if (str == "BLOCK")    return Suit::BLOCK;
    if (str == "STAR")     return Suit::STAR;
    if (str == "WHOT")     return Suit::WHOT;
    return Suit::CIRCLE;  // default fallback
}

CardValue stringToCardValue(const std::string& str)
{
    // Named values (for readability)
    if (str == "ONE")      return CardValue::ONE;
    if (str == "TWO")      return CardValue::TWO;
    if (str == "THREE")    return CardValue::THREE;
    if (str == "FOUR")     return CardValue::FOUR;
    if (str == "FIVE")     return CardValue::FIVE;
    if (str == "SEVEN")    return CardValue::SEVEN;
    if (str == "EIGHT")    return CardValue::EIGHT;
    if (str == "TEN")      return CardValue::TEN;
    if (str == "ELEVEN")   return CardValue::ELEVEN;
    if (str == "TWELVE")   return CardValue::TWELVE;
    if (str == "THIRTEEN") return CardValue::THIRTEEN;
    if (str == "FOURTEEN") return CardValue::FOURTEEN;
    if (str == "TWENTY")   return CardValue::TWENTY;
    // Numeric strings (from toJson / JSON)
    try {
        int n = std::stoi(str);
        if (n >= 1 && n <= 14 && n != 6 && n != 9) return static_cast<CardValue>(n);
        if (n == 20) return CardValue::TWENTY;
    } catch (...) {}
    return CardValue::ONE;  // default fallback
}

} // namespace whot::core