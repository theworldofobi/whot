#ifndef WHOT_CORE_HAND_HPP
#define WHOT_CORE_HAND_HPP

#include "Card.hpp"
#include <vector>
#include <memory>

namespace whot::core {

class Hand {
public:
    Hand();
    Hand(int numberOfCards);
    
    void addCard(std::unique_ptr<Card> card);
    void addCards(std::vector<std::unique_ptr<Card>> cards);
    std::unique_ptr<Card> playCard(size_t index);
    std::unique_ptr<Card> playCard(const Card& card);
    
    size_t size() const;
    bool isEmpty() const;
    bool hasCard(const Card& card) const;
    const Card& getCard(size_t index) const;
    
    std::vector<size_t> getPlayableCardIndices(const Card& callCard) const;
    bool hasPlayableCard(const Card& callCard) const;
    std::vector<const Card*> getCardsBySuit(Suit suit) const;
    std::vector<const Card*> getCardsByValue(CardValue value) const;
    std::vector<const Card*> getWhotCards() const;
    int calculateTotalScore() const;

    std::string toJson() const;
    static Hand fromJson(const std::string& json);
    
    auto begin();
    auto end();
    auto begin() const;
    auto end() const;
    
private:
    std::vector<std::unique_ptr<Card>> cards_;
    size_t size_;
    
    void sortCards();
};

} // namespace whot::core

#endif // WHOT_CORE_HAND_HPP