#ifndef WHOT_UTILS_RANDOM_HPP
#define WHOT_UTILS_RANDOM_HPP

#include <random>
#include <string>

namespace whot::utils {

class Random {
public:
    static Random& getInstance();
    
    // Integer generation
    int nextInt(int min, int max);
    
    // Float generation
    double nextDouble(double min = 0.0, double max = 1.0);
    
    // Boolean
    bool nextBool(double probability = 0.5);
    
    // String generation
    std::string generateId(size_t length = 16);
    /// Uppercase alphanumeric code for game join (e.g. 6 chars).
    std::string generateGameCode(size_t length = 6);
    std::string generateUUID();
    
    // Seeding
    void seed(unsigned int seed);
    void randomSeed();
    
private:
    Random();
    Random(const Random&) = delete;
    Random& operator=(const Random&) = delete;
    
    std::mt19937 generator_;
};

} // namespace whot::utils

#endif // WHOT_UTILS_RANDOM_HPP
