#include <random>
#include "number_helpers.hpp"

namespace netbook::helpers {

int get_random_number() {
    static thread_local std::uniform_int_distribution<> random_distribution(0, std::numeric_limits<int>::max());
    static thread_local std::random_device rd;
    static thread_local std::mt19937 generator(rd());

    return random_distribution(generator);
}

}