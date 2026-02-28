#include <pthread.h>
#include "concurrency.hpp"
#include <cstdint>

namespace netbook::concurrency {

// Pins the current thread to the given core ID.
void pin_thread_to_core(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

// Makes the current thread run on a unique core.
void use_unique_core_for_thread() {
    static unsigned int last_used_core = 0;

    pin_thread_to_core(last_used_core);

    ++last_used_core;
}

}