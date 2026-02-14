#include "../../include/Utils/Random.hpp"
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace whot::utils {

Random& Random::getInstance() {
    static Random instance;
    return instance;
}

Random::Random() : generator_(static_cast<unsigned>(std::random_device{}())) {}

int Random::nextInt(int min, int max) {
    if (min >= max) return min;
    std::uniform_int_distribution<int> dist(min, max);
    return dist(generator_);
}

double Random::nextDouble(double min, double max) {
    if (min >= max) return min;
    std::uniform_real_distribution<double> dist(min, max);
    return dist(generator_);
}

bool Random::nextBool(double probability) {
    return nextDouble(0.0, 1.0) < probability;
}

std::string Random::generateId(size_t length) {
    static const char chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string out;
    out.reserve(length);
    for (size_t i = 0; i < length; ++i)
        out += chars[nextInt(0, static_cast<int>(sizeof(chars) - 2))];
    return out;
}

std::string Random::generateGameCode(size_t length) {
    static const char chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string out;
    out.reserve(length);
    for (size_t i = 0; i < length; ++i)
        out += chars[nextInt(0, static_cast<int>(sizeof(chars) - 2))];
    return out;
}

std::string Random::generateUUID() {
    std::ostringstream oss;
    oss << std::hex;
    std::uniform_int_distribution<int> nibble(0, 15);
    auto putHex = [&](int digits) {
        for (int i = 0; i < digits; ++i)
            oss << nibble(generator_);
    };
    putHex(8);  oss << '-';
    putHex(4);  oss << '-';
    putHex(4);  oss << '-';
    putHex(4);  oss << '-';
    putHex(12);
    return oss.str();
}

void Random::seed(unsigned int seed) {
    generator_.seed(seed);
}

void Random::randomSeed() {
    generator_.seed(static_cast<unsigned>(std::random_device{}()));
}

} // namespace whot::utils
