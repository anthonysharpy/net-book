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

        auto packets_created = netbook::globals::packets_created.load();
        auto packets_written_to_write_buffer = netbook::globals::packets_written_to_write_buffer.load();
        auto packets_written_to_dpdk = netbook::globals::packets_written_to_dpdk.load();
        auto packets_read_from_dpdk = netbook::globals::packets_read_from_dpdk.load();
        auto packets_processed = netbook::globals::packets_processed.load();

        auto packets_created_per_second = static_cast<double>(packets_created) / (static_cast<double>(time_elapsed) / 1000000000.0);
        auto packets_written_to_write_buffer_per_second = static_cast<double>(packets_written_to_write_buffer) / (static_cast<double>(time_elapsed) / 1000000000.0);
        auto packets_written_to_dpdk_per_second = static_cast<double>(packets_written_to_dpdk) / (static_cast<double>(time_elapsed) / 1000000000.0);
        auto packets_read_from_dpdk_per_second = static_cast<double>(packets_read_from_dpdk) / (static_cast<double>(time_elapsed) / 1000000000.0);
        auto packets_processed_per_second = static_cast<double>(packets_processed) / (static_cast<double>(time_elapsed) / 1000000000.0);

        std::cout << "Packets created: " <<  packets_created << " (" << static_cast<std::uint64_t>(packets_created_per_second) << " packets/s)" << std::endl;
        std::cout << "Packets written to write buffer: " <<  packets_written_to_write_buffer << " (" << static_cast<std::uint64_t>(packets_written_to_write_buffer_per_second) << " packets/s)" << std::endl;
        std::cout << "Packets given to DPDK: " <<  packets_written_to_dpdk << " (" << static_cast<std::uint64_t>(packets_written_to_dpdk) << " packets/s)" << std::endl;
        std::cout << "Packets taken from DPDK: " <<  packets_read_from_dpdk << " (" << static_cast<std::uint64_t>(packets_read_from_dpdk_per_second) << " packets/s)" << std::endl;
        std::cout << "Packets processed: " <<  packets_processed << " (" << static_cast<std::uint64_t>(packets_processed_per_second) << " packets/s)" << std::endl;
        std::cout.flush();

        std::this_thread::sleep_for(std::chrono::milliseconds(netbook::globals::print_delay_ms));

        // Wipe the lines we printed above.
        std::cout << "\033[" << 5 << "F";
    }
}

void poll() {
    std::cout << "Beginning DPDK poll loop..." << std::endl;
    std::jthread poll_write_thread(netbook::dpdk::poll_write);
    std::jthread poll_read_thread(netbook::dpdk::poll_read);
    std::jthread poll_process_thread(netbook::dpdk::poll_process_buffer);
    std::jthread mock_data_thread(netbook::mocking::push_mock_data);
    std::jthread print_thread(print_stats);

    netbook::globals::simulation_start_time_ns.store(netbook::helpers::get_benchmark_timestamp_nanoseconds());

    while (got_stop_signal == 0) {
        auto current_time = netbook::helpers::get_benchmark_timestamp_nanoseconds();
        auto start_time = netbook::globals::simulation_start_time_ns.load();
        auto runtime_seconds = static_cast<double>(current_time - start_time) / 1000000000.0;

        if (netbook::globals::program_runtime_limit_seconds != 0
            && runtime_seconds >= netbook::globals::program_runtime_limit_seconds) {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    print_thread.request_stop();
    print_thread.join();

    std::cout << "Got stop signal, stopping..." << std::endl;

    std::cout << "Waiting for mock data thread to stop..." << std::endl;
    mock_data_thread.request_stop();
    mock_data_thread.join();

    std::cout << "Waiting for dpdk write thread to stop..." << std::endl;
    poll_write_thread.request_stop();
    poll_write_thread.join();

    std::cout << "Waiting for dpdk read thread to stop..." << std::endl;
    poll_read_thread.request_stop();
    poll_read_thread.join();

    std::cout << "Waiting for dpdk process thread to stop..." << std::endl;
    poll_process_thread.request_stop();
    poll_process_thread.join();
}

int main() {
    initialise();

    poll();

    netbook::dpdk::cleanup();
}