#include <iostream>
#include <csignal>
#include <cstring>
#include <thread>
#include "dpdk/dpdk.hpp"
#include "mocking/mock_data.hpp"
#include "processing/message_processor.hpp"
#include "globals/globals.hpp"
#include "globals/constants.hpp"
#include "helpers/time_helpers.hpp"
#include "concurrency/concurrency.hpp"

void initialise() {
    if (!netbook::dpdk::initialise()) {
        std::cerr << "Initialisation failed, exiting...\n";
        exit(1);
    }

    std::cout << "Registering network receivers...\n";

    netbook::dpdk::register_receiver(netbook::processing::process_message);

    std::cout << "Initialisation succeeded\n";
}

void poll() {
    std::cout << "Beginning DPDK poll loop...\n";
    std::cout << "Sending " << netbook::globals::packet_limit << " packets over " << netbook::globals::dpdk_queue_count << " queues...\n";

    std::vector<std::jthread> poll_read_threads;
    std::vector<std::jthread> mock_data_threads;

    poll_read_threads.reserve(netbook::globals::dpdk_queue_count);
    mock_data_threads.reserve(netbook::globals::dpdk_queue_count);

    auto start_time = netbook::helpers::get_benchmark_timestamp_nanoseconds();

    for (unsigned int i = 0; i < netbook::globals::dpdk_queue_count; ++i) {
        poll_read_threads.emplace_back(netbook::dpdk::poll_read, i, netbook::globals::packets_per_queue);
        mock_data_threads.emplace_back(netbook::mocking::mock_data_pusher, i, netbook::globals::packets_per_queue);
    }

    for (unsigned int i = 0; i < netbook::globals::dpdk_queue_count; ++i) {
        mock_data_threads[i].join();
        poll_read_threads[i].join();
    }

    auto end_time = netbook::helpers::get_benchmark_timestamp_nanoseconds();

    std::cout << "Complete!\n"
        << "Total time: " << end_time - start_time << "ns\n";
}

int main() {
    initialise();
    poll();
    netbook::dpdk::cleanup();

    return EXIT_SUCCESS;
}