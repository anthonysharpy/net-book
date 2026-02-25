// #include "gtest/gtest.h"
// #include "concurrency/spscringbuffer.hpp"
// #include <vector>

// using nanofill::concurrency::SPSCRingBuffer;

// TEST(Concurrency, SPSCRingBuffer) {
//     auto buffer = SPSCRingBuffer<int, 128>();

//     int item;
//     int items[128]{};

//     // Nothing to pop.
//     ASSERT_FALSE(buffer.pop(item));

//     // Nothing to pop.
//     ASSERT_FALSE(buffer.pop_many(items, 5000));

//     // Push then pop one by one.
//     ASSERT_TRUE(buffer.push(1));
//     ASSERT_TRUE(buffer.push(2));
//     ASSERT_TRUE(buffer.push(3));
//     ASSERT_TRUE(buffer.pop(item));
//     ASSERT_EQ(1, item);
//     ASSERT_TRUE(buffer.pop(item));
//     ASSERT_EQ(2, item);
//     ASSERT_TRUE(buffer.pop(item));
//     ASSERT_EQ(3, item);
//     ASSERT_FALSE(buffer.pop(item));
//     ASSERT_EQ(3, item);

//     // Push then pop lots.
//     ASSERT_TRUE(buffer.push(1));
//     ASSERT_TRUE(buffer.push(2));
//     ASSERT_TRUE(buffer.push(3));
//     ASSERT_EQ(3U, buffer.pop_many(items, 1000));
//     ASSERT_EQ(1, items[0]);
//     ASSERT_EQ(2, items[1]);
//     ASSERT_EQ(3, items[2]);
//     ASSERT_EQ(0, items[3]);
//     ASSERT_EQ(0U, buffer.pop_many(items, 1000));
//     ASSERT_EQ(1, items[0]);
//     ASSERT_EQ(2, items[1]);
//     ASSERT_EQ(3, items[2]);
//     ASSERT_EQ(0, items[3]);

//     // Make it full.
//     ASSERT_FALSE(buffer.pop(item));
//     for (int i = 0; i < 127; ++i) {
//         ASSERT_TRUE(buffer.push(i));
//     }
//     ASSERT_FALSE(buffer.push(999));

//     // Pop them, checking order.
//     for (int i = 0; i < 127; ++i) {
//         ASSERT_TRUE(buffer.pop(item));
//         ASSERT_EQ(i, item);
//     }

//     // Make it full.
//     for (int i = 0; i < 127; ++i) {
//         ASSERT_TRUE(buffer.push(i));
//     }
//     ASSERT_FALSE(buffer.push(999));

//     // Pop them all at once, checking order.
//     ASSERT_EQ(127U, buffer.pop_many(items, 9999));
//     ASSERT_FALSE(buffer.pop(item));
//     for (int i = 0; i < 127; ++i) {
//         ASSERT_EQ(i, items[i]);
//     }
// }

// TEST(Concurrency, SPSCRingBufferConcurrencyStressTest) {
//     auto buffer = SPSCRingBuffer<int, 64>();
//     int read_count = 0;

//     std::thread writer([&] {
//         for (int i = 0; i < 10000;) {
//             if (buffer.push(i)) {
//                 ++i;
//             }
//         }
//     });

//     std::thread reader([&] {
//         int value;

//         for (int i = 0; i < 10000;) {
//             if (buffer.pop(value)) {
//                 if (value == read_count) {
//                     read_count++;
//                 }

//                 ++i;
//             }
//         }
//     });

//     writer.join();
//     reader.join();

//     ASSERT_EQ(10000, read_count);
// }

// TEST(Concurrency, SPSCRingBufferConcurrencyStressTestPopMany) {
//     auto buffer = SPSCRingBuffer<int, 64>();
//     int read_count = 0;

//     std::thread writer([&] {
//         for (int i = 0; i < 10000;) {
//             if (buffer.push(i)) {
//                 ++i;
//             }
//         }
//     });

//     std::thread reader([&] {
//         int values[10];
//         int amount_popped = 0;

//         for (int i = 0; i < 10000;) {
//             amount_popped = buffer.pop_many(values, 10);
            
//             if (amount_popped > 0) {
//                 for (int x = 0; x < amount_popped; ++x) {
//                     if (values[x] == read_count) {
//                         ++read_count;
//                     }

//                     ++i;
//                 }
//             }
//         }
//     });

//     writer.join();
//     reader.join();

//     ASSERT_EQ(10000, read_count);
// }