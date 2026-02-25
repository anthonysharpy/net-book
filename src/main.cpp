#include <iostream>
#include "dpdk/dpdk.hpp"
#include <csignal>
#include <cstring>
#include <thread>

static volatile sig_atomic_t got_stop_signal = 0;

// Called when we have been told to terminate.
void terminate_handler(int) 
{
    got_stop_signal = 1;
}

void initialise() {
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = terminate_handler;
    sigaction(SIGTERM, &action, nullptr);
    sigaction(SIGINT, &action, nullptr);

    if (!netbook::dpdk::initialise()) {
        std::cerr << "Initialisation failed, exiting" << std::endl;
        exit(1);
    }

    std::cout << "Initialisation succeeded" << std::endl;
}

void poll() {
    std::cout << "Beginning DPDK poll loop..." << std::endl;
    std::jthread poll_thread(netbook::dpdk::poll);

    while (got_stop_signal == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Got stop signal, stopping..." << std::endl;

    poll_thread.request_stop();
    poll_thread.join();
}

int main() {
    initialise();

    poll();

    netbook::dpdk::cleanup();
}