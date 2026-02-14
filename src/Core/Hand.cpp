#include "../../include/Core/Hand.hpp"
#include <algorithm>
#include <utility>
#include <nlohmann/json.hpp>

namespace whot::core {

    Hand::Hand() : size_(0) {}

    Hand::Hand(int numberOfCards) : size_(0)
    {
        cards_.reserve(static_cast<size_t>(numberOfCards));
    }

    void Hand::addCard(std::unique_ptr<Card> card)
    {
        if (card) {
            cards_.push_back(std::move(card));
            size_ = cards_.size();
        }
    }

    void Hand::addCards(std::vector<std::unique_ptr<Card>> cards)
    {
        for (auto& card : cards) addCard(std::move(card));
    }

    std::unique_ptr<Card> Hand::playCard(size_t index)
    {
        if (index >= cards_.size()) return nullptr;
        std::unique_ptr<Card> c = std::move(cards_[index]);
        cards_.erase(cards_.begin() + static_cast<std::ptrdiff_t>(index));
        size_ = cards_.size();
        return c;
    }

    std::unique_ptr<Card> Hand::playCard(const Card& card)
    {
        for (size_t i = 0; i < cards_.size(); ++i) {
            if (*cards_[i] == card) return playCard(i);
        }
        return nullptr;
    }

    void Hand::sortCards()
    {
        std::sort(cards_.begin(), cards_.end(), [](const std::unique_ptr<Card>& a, const std::unique_ptr<Card>& b) {
            if (static_cast<int>(a->getSuit()) != static_cast<int>(b->getSuit()))
                return static_cast<int>(a->getSuit()) < static_cast<int>(b->getSuit());
            return a->getNumericValue() < b->getNumericValue();
        });
    }

    size_t Hand::size() const { return cards_.size(); }

    bool Hand::isEmpty() const { return cards_.empty(); }

    bool Hand::hasCard(const Card& card) const
    {
        for (const auto& c : cards_)
            if (*c == card) return true;
        return false;
    }

    const Card& Hand::getCard(size_t index) const
    {
        return *cards_.at(index);
    }

    std::vector<size_t> Hand::getPlayableCardIndices(const Card& callCard) const
    {
        std::vector<size_t> indices;
        for (size_t i = 0; i < cards_.size(); ++i)
            if (cards_[i]->canPlayOn(callCard)) indices.push_back(i);
        return indices;
    }

    bool Hand::hasPlayableCard(const Card& callCard) const
    {
        return !getPlayableCardIndices(callCard).empty();
    }

    std::vector<const Card*> Hand::getCardsBySuit(Suit suit) const
    {
        std::vector<const Card*> out;
        for (const auto& c : cards_)
            if (c->matchesSuit(suit)) out.push_back(c.get());
        return out;
    }

    std::vector<const Card*> Hand::getCardsByValue(CardValue value) const
    {
        std::vector<const Card*> out;
        for (const auto& c : cards_)
            if (c->matchesValue(value)) out.push_back(c.get());
        return out;
    }

    std::vector<const Card*> Hand::getWhotCards() const
    {
        std::vector<const Card*> out;
        for (const auto& c : cards_)
            if (c->isWhotCard()) out.push_back(c.get());
        return out;
    }

    int Hand::calculateTotalScore() const
    {
        int score = 0;
        for (const auto& card : cards_) score += card->getScoreValue();
        return score;
    }

    std::string Hand::toJson() const
    {
        using json = nlohmann::json;
        json j = json::array();
        for (const auto& card : cards_)
            if (card) j.push_back(json::parse(card->toJson()));
        return j.dump();
    }

    Hand Hand::fromJson(const std::string& jsonStr)
    {
        using json = nlohmann::json;
        Hand h;
        json j = json::parse(jsonStr);
        if (!j.is_array()) return h;
        for (const auto& item : j) {
            std::string cardStr = item.dump();
            auto card = Card::fromJson(cardStr);
            if (card) h.addCard(std::move(card));
        }
        return h;
    }

    auto Hand::begin() { return cards_.begin(); }
    auto Hand::end() { return cards_.end(); }
    auto Hand::begin() const { return cards_.cbegin(); }
    auto Hand::end() const { return cards_.cend(); }

} // namespace whot::core