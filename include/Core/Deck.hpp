#ifndef WHOT_CORE_DECK_HPP
#define WHOT_CORE_DECK_HPP

#include "Card.hpp"
#include <vector>
#include <memory>
#include <random>

namespace whot::core {

class Deck {
public:
    Deck();
    explicit Deck(int numberOfDecks);  // For games with many players
    void createDeck();

    void shuffle();
    std::unique_ptr<Card> draw();
    void addCard(std::unique_ptr<Card> card);
    void addCards(std::vector<std::unique_ptr<Card>> cards);

    size_t size() const;
    bool isEmpty() const;
    const Card* peek() const;
    
    void initializeStandardDeck();
    void reset();
    void clear();
    
    void reshuffleFromDiscardPile(std::vector<std::unique_ptr<Card>> discardPile, 
                                   const Card& currentCallCard);
    
    std::string toJson() const;
    static Deck fromJson(const std::string& json);
    
private:
    std::vector<std::unique_ptr<Card>> cards_;
    std::mt19937 rng_;
    
    void createCircleCards();
    void createTriangleCards();
    void createCrossCards();
    void createBlockCards();
    void createStarCards();
    void createWhotCards();
};

} // namespace whot::core

#endif // WHOT_CORE_DECK_HPP
