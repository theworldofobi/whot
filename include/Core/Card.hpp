#ifndef WHOT_CORE_CARD_HPP
#define WHOT_CORE_CARD_HPP

#include "GameConstants.hpp"
#include <memory>
#include <string>

namespace whot::core {

class Card {
public:
    Card(Suit suit, CardValue value);

    Suit getSuit() const;
    CardValue getValue() const;
    int getNumericValue() const;
    int getScoreValue() const;  // For star cards, returns doubled value
    SpecialAbility getSpecialAbility() const;
    bool isWhotCard() const;
    bool isStarCard() const;
    
    int getOuterStarValue() const;  // For star cards
    int getInnerStarValue() const;  // Doubled value for stars

    bool canPlayOn(const Card& callCard) const;
    bool matchesSuit(Suit suit) const;
    bool matchesValue(CardValue value) const;

    std::string toString() const;
    std::string toJson() const;
    static std::unique_ptr<Card> fromJson(const std::string& json);
    
    bool operator==(const Card& other) const;
    bool operator!=(const Card& other) const;
    
private:
    Suit suit_;
    CardValue value_;
    SpecialAbility specialAbility_;
    
    SpecialAbility determineSpecialAbility() const;
};

} // namespace whot::core

#endif // WHOT_CORE_CARD_HPP
