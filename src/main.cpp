#include <iostream>
#include <csignal>
#include <cstring>
#include <thread>
#include "dpdk/dpdk.hpp"
#include "mocking/mock_data.hpp"
#include "processing/message_processor.hpp"
#include "constants/constants.hpp"
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
    std::cout << "Sending " << netbook::constants::PACKET_LIMIT << " packets over " << netbook::constants::DPDK_QUEUE_COUNT << " queues...\n";

    std::vector<std::thread> poll_read_threads;
    std::vector<std::thread> mock_data_threads;

    poll_read_threads.reserve(netbook::constants::DPDK_QUEUE_COUNT);
    mock_data_threads.reserve(netbook::constants::DPDK_QUEUE_COUNT);

    auto start_time = netbook::helpers::get_benchmark_timestamp_nanoseconds();

    for (unsigned int i = 0; i < netbook::constants::DPDK_QUEUE_COUNT; ++i) {
        poll_read_threads.emplace_back(netbook::dpdk::poll_read, i, netbook::constants::PACKETS_PER_QUEUE);
        mock_data_threads.emplace_back(netbook::mocking::mock_data_pusher, i, netbook::constants::PACKETS_PER_QUEUE);
    }

    for (unsigned int i = 0; i < netbook::constants::DPDK_QUEUE_COUNT; ++i) {
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