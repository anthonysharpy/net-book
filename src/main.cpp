#include <iostream>
#include <csignal>
#include <cstring>
#include <thread>
#include "dpdk/dpdk.hpp"
#include "mocking/mock_data.hpp"

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
    std::jthread poll_write_thread(netbook::dpdk::poll_write);
    std::jthread poll_read_thread(netbook::dpdk::poll_read);
    std::jthread mock_data_thread(netbook::mocking::push_mock_data);

    while (got_stop_signal == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Got stop signal, stopping..." << std::endl;

    mock_data_thread.request_stop();
    poll_write_thread.request_stop();
    poll_read_thread.request_stop();
    mock_data_thread.join();
    poll_write_thread.join();
    poll_read_thread.join();
}

int main() {
    initialise();

    poll();

    netbook::dpdk::cleanup();
}