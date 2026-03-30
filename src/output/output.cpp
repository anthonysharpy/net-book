#include "output.hpp"
#include "concurrency/concurrency.hpp"
#include "helpers/time_helpers.hpp"
#include "globals/globals.hpp"
#include "globals/constants.hpp"
#include <iostream>

namespace netbook::output {

void print_stats() {
    auto current_time = netbook::helpers::get_benchmark_timestamp_nanoseconds();
    auto time_elapsed = current_time - netbook::globals::simulation_start_time_ns.load();

    auto packets_written_to_dpdk = netbook::globals::packets_written_to_dpdk.load();
    auto packets_read_from_dpdk = netbook::globals::packets_read_from_dpdk.load();

    auto packets_written_to_dpdk_per_second = static_cast<double>(packets_written_to_dpdk) / (static_cast<double>(time_elapsed) / 1000000000.0);
    auto packets_read_from_dpdk_per_second = static_cast<double>(packets_read_from_dpdk) / (static_cast<double>(time_elapsed) / 1000000000.0);

    std::cout << "Packets given to DPDK: " <<  packets_written_to_dpdk << " (" << static_cast<std::uint64_t>(packets_written_to_dpdk_per_second) << " packets/s)\n";
    std::cout << "Packets taken from DPDK: " <<  packets_read_from_dpdk << " (" << static_cast<std::uint64_t>(packets_read_from_dpdk_per_second) << " packets/s)\n";
    std::cout << "Total throughput: " << packets_read_from_dpdk << "\n";
    std::cout.flush();
}

// Prints stats to the console.
void print_stats_thread(std::stop_token stop) {
    netbook::concurrency::use_unique_core_for_thread();

    while (!stop.stop_requested()) {
        print_stats();

        std::this_thread::sleep_for(std::chrono::milliseconds(netbook::globals::print_delay_ms));

        // Wipe the lines we printed above.
        std::cout << "\033[" << 3 << "F";
    }

    // Print freshest stats on exit.
    print_stats();
}

}