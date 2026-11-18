#include "random.hpp"
#include <chrono>

static std::mt19937 rng;

void set_seed(unsigned int seed) { rng.seed(seed); }

int random_int(int lo, int hi) {
    std::uniform_int_distribution<int> d(lo, hi);
    return d(rng);
}

double random_double(double lo, double hi) {
    std::uniform_real_distribution<double> d(lo, hi);
    return d(rng);
}

unsigned int random_uint() {
    return rng();
}