// #include "fileio/fileio.hpp"
// #include "fileio/csv.hpp"
// #include "events/event.hpp"
// #include "orderbook/orderbook.hpp"
// #include "concurrency/spscringbuffer.hpp"
// #include "tradingengine/tradingengine.hpp"
// #include "threads/threads.hpp"
// #include "graphics/renderer.hpp"
// #include "consts/consts.hpp"
// #include <iostream>
// #include <chrono>
// #include <thread>

// using nanofill::events::Event;
// using nanofill::tradingengine::TradingEngine;
// using nanofill::orderbook::OrderBook;
// using nanofill::concurrency::SPSCRingBuffer;

// void initialise() {
//     std::ios_base::sync_with_stdio(false);
//     std::cin.tie(nullptr);
// }

// std::vector<Event> parse_events(const std::vector<nanofill::consts::TradingDataCSVFormat>& csv_data) {
//     std::cout << "Parsing " << csv_data.size() << " events..." << std::endl;
//     auto clock_start = std::chrono::steady_clock::now();
//     auto events = nanofill::events::events_from_csv_data(csv_data);
//     auto clock_end = std::chrono::steady_clock::now();
//     std::int64_t elapsed = std::chrono::duration_cast<std::chrono::microseconds>(clock_end - clock_start).count();
//     double elapsed_seconds = elapsed / 1000000.0;
//     std::cout << "Done in " << elapsed_seconds << " seconds" << std::endl;

//     return events;
// }

// std::vector<unsigned int>
// process_events(const std::vector<Event>& events, TradingEngine& trading_engine, OrderBook& order_book) {
//     std::vector<unsigned int> performance_data;
//     performance_data.resize(events.size());
//     SPSCRingBuffer<Event, 1024> buffer;
    
//     std::cout << "Processing " << events.size() << " events..." << std::endl;

//     std::thread event_producer_thread(nanofill::threads::event_producer<1024>, std::ref(buffer), std::ref(events));
//     std::thread event_consumer_thread(
//         nanofill::threads::event_consumer<1024>,
//         std::ref(buffer), std::ref(order_book),
//         std::ref(trading_engine),
//         std::ref(performance_data)
//     );
//     event_producer_thread.join();
//     event_consumer_thread.join();
    
//     std::cout << "Done!" << std::endl;

//     return performance_data;
// }

// int main() {
//     std::cout << "Initialising..." << std::endl;
//     initialise();

//     OrderBook order_book;
//     TradingEngine trading_engine(10000);

//     std::cout << "Opening data file..." << std::endl;
//     std::vector<std::string> file_data = nanofill::fileio::open_text_file("./data/MSFT_2012-06-21_34200000_57600000_message_10.csv");

//     std::cout << "Parsing CSV data..." << std::endl;
//     auto csv_data = nanofill::fileio::parse_csv_data<nanofill::consts::TradingDataCSVFormat>(file_data);
//     std::cout << "Done!" << std::endl;

//     // ===== FROM HERE is where we care about performance ===== //
//     // ===== ここから性能が大事だ ===== //

//     auto events = parse_events(csv_data);
//     auto performance_data = process_events(events, trading_engine, order_book);    

//     // ===== Don't care about performance after this ===== //
//     // ===== ここから性能がどうでもいい ===== //

//     nanofill::graphics::render_latency_chart(performance_data);

//     return 0;
// }