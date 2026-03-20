#include "../../include/Core/Card.hpp"
#include <memory>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace whot::core {
    Card::Card(Suit suit, CardValue value) : suit_(suit)
                                           , value_(value)
                                           , specialAbility_(determineSpecialAbility()) {}

    Suit Card::getSuit() const { return suit_; }
    
    CardValue Card::getValue() const { return value_; }
    
    int Card::getNumericValue() const { return static_cast<int>(value_); }
    
    int Card::getScoreValue() const 
    {
        if (isStarCard()) { return getInnerStarValue(); }
        return getNumericValue();
    }
    
    SpecialAbility Card::getSpecialAbility() const { return specialAbility_; }
    
    bool Card::isWhotCard() const { return specialAbility_ == SpecialAbility::WHOT_CARD; }
    
    bool Card::isStarCard() const { return suit_ == Suit::STAR; }

    int Card::getOuterStarValue() const { return getNumericValue(); }
    
    int Card::getInnerStarValue() const { return 2 * getNumericValue(); }

    bool Card::canPlayOn(const Card& callCard) const
    {
        // Whot cards can play on anything (guide: "Whot cards can play on anything")
        if (isWhotCard()) return true;
        // Otherwise match suit OR number (guide: "Match suit OR match number")
        return matchesSuit(callCard.suit_) || matchesValue(callCard.value_);
    }
    
    bool Card::matchesSuit(Suit suit) const { return suit_ == suit;}
    
    bool Card::matchesValue(CardValue value) const { return value_ == value; }

    std::string Card::toString() const 
    {
        std::string card = suitToString(suit_) + " " + cardValueToString(value_);
        return card;
    }

    std::string Card::toJson() const 
    {
        json j;
        j["suit"] = suitToString(suit_);
        j["value"] = cardValueToString(value_);

        std::string jsonString = j.dump();
        return jsonString;
    }
    
    std::unique_ptr<Card> Card::fromJson(const std::string& jsonStr) 
    {
        json j = json::parse(jsonStr);
        
        Suit suit = stringToSuit(j.at("suit").get<std::string>());
        CardValue value = stringToCardValue(j.at("value").get<std::string>());
        auto card = std::make_unique<Card>(suit, value);
        
        return card;
    }
    
    bool Card::operator==(const Card& other) const 
    {
        return getSuit() == other.getSuit() && getValue() == other.getValue();
    }

    bool Card::operator!=(const Card& other) const 
    {
        return !(*this == other);
    }

    SpecialAbility Card::determineSpecialAbility() const
    {
        CardValue cv = getValue();
        switch (cv) {
        case CardValue::ONE:
            return SpecialAbility::HOLD_ON;
        case CardValue::TWO:
            return SpecialAbility::PICK_TWO;
        case CardValue::FOURTEEN:
            return SpecialAbility::GENERAL_MARKET;
        case CardValue::TWENTY:
            return SpecialAbility::WHOT_CARD;
        default:
            return SpecialAbility::NONE;
        }
    }

} // namespace whot::core