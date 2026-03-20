#include "../../include/Core/Deck.hpp"
#include <algorithm>
#include <random>
#include <nlohmann/json.hpp>

namespace whot::core {

    Deck::Deck()
    {
        cards_.reserve(STANDARD_DECK_SIZE);
        std::random_device rd;
        rng_.seed(rd());
        createDeck();
        shuffle();
    }

    Deck::Deck(int numberOfDecks)
    {
        cards_.reserve(numberOfDecks * STANDARD_DECK_SIZE);
        std::random_device rd;
        rng_.seed(rd());
        for (int i = 0; i < numberOfDecks; ++i) createDeck();
        shuffle();
    }
    void Deck::createDeck()
    {
        createCircleCards();
        createTriangleCards();
        createCrossCards();
        createBlockCards();
        createStarCards();
        createWhotCards();
    }
    void Deck::createCircleCards()
    {
        // valid card values = 1, 2, 3, 4, 5, 7, 8, 10, 11, 12, 13, 14
        for (int i{1}; i <= 14; i++)
        {
            if (i == 6 || i == 9) { continue; }
            cards_.push_back(std::make_unique<Card>(Suit::CIRCLE, static_cast<CardValue>(i)));
        }
    }
    void Deck::createTriangleCards() 
    {
        // valid card values = 1, 2, 3, 4, 5, 7, 8, 10, 11, 12, 13, 14
        for (int i{1}; i <= 14; i++)
        {
            if (i == 6 || i == 9) { continue; }
            cards_.push_back(std::make_unique<Card>(Suit::TRIANGLE, static_cast<CardValue>(i)));
        }
    }
    void Deck::createCrossCards() 
    {
        // valid card values = 1, 2, 3, 5, 7, 10, 11, 13, 14
        for (int i{1}; i <= 14; i++)
        {
            if (i == 4 || i == 6 || i == 8 || i == 9 || i == 12) { continue; }
            cards_.push_back(std::make_unique<Card>(Suit::CROSS, static_cast<CardValue>(i)));
        }
    }
    void Deck::createBlockCards() 
    {
        // valid card values = 1, 2, 3, 5, 7, 10, 11, 13, 14
        for (int i{1}; i <= 14; i++)
        {
            if (i == 4 || i == 6 || i == 8 || i == 9 || i == 12) { continue; }
            cards_.push_back(std::make_unique<Card>(Suit::BLOCK, static_cast<CardValue>(i)));
        }
    }
    void Deck::createStarCards() 
    {
        // valid card values = 1, 2, 3, 4, 5, 7, 8
        for (int i{1}; i <= 8; i++)
        {
            if (i == 6) { continue; }
            cards_.push_back(std::make_unique<Card>(Suit::STAR, static_cast<CardValue>(i)));
        }
    }
    void Deck::createWhotCards() 
    {
        for (int i = 0; i < WHOT_CARDS_COUNT; ++i)
        {
            cards_.push_back(std::make_unique<Card>(Suit::WHOT, CardValue::TWENTY));
        }
    }

    void Deck::shuffle()
    {
        std::shuffle(cards_.begin(), cards_.end(), rng_);
    }

    std::unique_ptr<Card> Deck::draw()
    {
        if (cards_.empty()) return nullptr;
        std::unique_ptr<Card> top = std::move(cards_.back());
        cards_.pop_back();
        return top;
    }

    void Deck::addCard(std::unique_ptr<Card> card)
    {
        if (card) cards_.push_back(std::move(card));
    }

    void Deck::addCards(std::vector<std::unique_ptr<Card>> cards)
    {
        for (auto& card : cards) addCard(std::move(card));
    }

    size_t Deck::size() const { return cards_.size(); }
    bool Deck::isEmpty() const { return cards_.empty(); }

    const Card* Deck::peek() const
    {
        return cards_.empty() ? nullptr : cards_.back().get();
    }

    void Deck::initializeStandardDeck()
    {
        clear();
        createDeck();
    }

    void Deck::reset()
    {
        clear();
        createDeck();
        shuffle();
    }

    void Deck::clear()
    {
        cards_.clear();
    }

    void Deck::reshuffleFromDiscardPile(std::vector<std::unique_ptr<Card>> discardPile,
                                        const Card& currentCallCard)
    {
        for (auto& card : discardPile) {
            if (card && !(*card == currentCallCard))
                cards_.push_back(std::move(card));
        }
        shuffle();
    }

    std::string Deck::toJson() const
    {
        using json = nlohmann::json;
        json j = json::array();
        for (const auto& card : cards_)
            if (card) j.push_back(json::parse(card->toJson()));
        return j.dump();
    }

    Deck Deck::fromJson(const std::string& jsonStr)
    {
        using json = nlohmann::json;
        Deck d;
        d.cards_.clear();
        json j = json::parse(jsonStr);
        if (!j.is_array()) return d;
        for (const auto& item : j) {
            std::string cardStr = item.dump();
            auto card = Card::fromJson(cardStr);
            if (card) d.cards_.push_back(std::move(card));
        }
        return d;
    }

} // namespace whot::core
