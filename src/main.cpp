#include <iostream>
#include <csignal>
#include <cstring>
#include <thread>
#include "dpdk/dpdk.hpp"
#include "mocking/mock_data.hpp"
#include "processing/message_processor.hpp"
#include "globals/globals.hpp"
#include "helpers/time_helpers.hpp"

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

    std::cout << "Registering network receivers..." << std::endl;

    netbook::dpdk::register_receiver(netbook::processing::process_message);

    std::cout << "Initialisation succeeded" << std::endl;
}

// Prints stats to the console.
void print_stats(std::stop_token stop) {
    while (!stop.stop_requested()) {
        auto current_time = netbook::helpers::get_benchmark_timestamp_nanoseconds();
        auto time_elapsed = current_time - netbook::globals::simulation_start_time_ns.load();
        auto packets_sent = netbook::globals::packets_sent.load();
        auto packets_processed = netbook::globals::packets_processed.load();
        auto packets_sent_per_second = static_cast<double>(packets_sent) / (static_cast<double>(time_elapsed) / 1000000000.0);
        auto packets_processed_per_second = static_cast<double>(packets_processed) / (static_cast<double>(time_elapsed) / 1000000000.0);

        std::cout << "Packets sent: " <<  packets_sent << "(" << static_cast<uint64_t>(packets_sent_per_second) << " packets/s)" << std::endl;
        std::cout << "Packets processed: " <<  packets_processed << "(" << static_cast<uint64_t>(packets_processed_per_second) << " packets/s)" << std::endl;
        std::cout.flush();

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Wipe the lines we printed above.
        std::cout << "\033[" << 2 << "F";
    }
}

void poll() {
    std::cout << "Beginning DPDK poll loop..." << std::endl;
    std::jthread poll_write_thread(netbook::dpdk::poll_write);
    std::jthread poll_read_thread(netbook::dpdk::poll_read);
    std::jthread poll_read_buffer_thread(netbook::dpdk::poll_read_buffer);
    std::jthread mock_data_thread(netbook::mocking::push_mock_data);
    std::jthread print_thread(print_stats);

    netbook::globals::simulation_start_time_ns.store(netbook::helpers::get_benchmark_timestamp_nanoseconds());

    while (got_stop_signal == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Got stop signal, stopping..." << std::endl;

    mock_data_thread.request_stop();
    poll_write_thread.request_stop();
    poll_read_thread.request_stop();
    poll_read_buffer_thread.request_stop();
    print_thread.request_stop();
    mock_data_thread.join();
    poll_write_thread.join();
    poll_read_thread.join();
    poll_read_buffer_thread.join();
    print_thread.join();
}

int main() {
    initialise();

    poll();

    netbook::dpdk::cleanup();
}