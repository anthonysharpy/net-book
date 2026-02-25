#include <iostream>
#include "dpdk/dpdk.hpp"

void initialise() {
    if (!netbook::dpdk::initialise()) {
        std::cerr << "Initialisation failed, exiting" << std::endl;
        exit(1);
    }
}

int main() {
    initialise();

    netbook::dpdk::cleanup();
}