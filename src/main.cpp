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
#include "output/output.hpp"

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
        std::cerr << "Initialisation failed, exiting...\n";
        exit(1);
    }

    std::cout << "Registering network receivers...\n";

    netbook::dpdk::register_receiver(netbook::processing::process_message);

    std::cout << "Initialisation succeeded\n";
}

void poll() {
    std::cout << "Beginning DPDK poll loop...\n";

    std::vector<std::jthread> poll_read_threads;
    std::vector<std::jthread> mock_data_threads;

    for (int i = 0; i < netbook::globals::dpdk_queue_count; ++i) {
        std::cout << "Creating queue loop " << i << "...\n";

        poll_read_threads.emplace_back(netbook::dpdk::poll_read, i);
        mock_data_threads.emplace_back(netbook::mocking::push_mock_data, i);
    }
   
    std::jthread print_thread(netbook::output::print_stats_thread);

    netbook::globals::simulation_start_time_ns.store(netbook::helpers::get_benchmark_timestamp_nanoseconds());

    while (got_stop_signal == 0) {
        auto current_time = netbook::helpers::get_benchmark_timestamp_nanoseconds();
        auto start_time = netbook::globals::simulation_start_time_ns.load();
        auto runtime_seconds = static_cast<double>(current_time - start_time) / 1000000000.0;

        if (netbook::globals::program_runtime_limit_seconds.has_value()
            && runtime_seconds >= netbook::globals::program_runtime_limit_seconds) {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    print_thread.request_stop();
    print_thread.join();

    std::cout << "Got stop signal, stopping...\n";

    for (int i = 0; i < netbook::globals::dpdk_queue_count; ++i) {
        std::cout << "Waiting for queue loop " << i << " to stop...\n";

        std::cout << "Waiting for mock data thread to stop...\n";
        mock_data_threads[i].request_stop();
        mock_data_threads[i].join();

        std::cout << "Waiting for dpdk read thread to stop...\n";
        poll_read_threads[i].request_stop();
        poll_read_threads[i].join();
    }
}

void parse_args(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg.starts_with("--runtime=")) {
            netbook::globals::program_runtime_limit_seconds = std::stoi(arg.substr(10));
        }
    }
}

int main(int argc, char* argv[]) {
    parse_args(argc, argv);

    initialise();
    poll();
    netbook::dpdk::cleanup();
}