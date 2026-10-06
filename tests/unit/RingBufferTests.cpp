#include "JulyTest.h"
#include "july/engine/RingBuffer.h"

using july::RingBuffer;

TEST_CASE("RingBuffer push/popBack is LIFO from the newest end") {
    RingBuffer<int, 4> rb;
    CHECK(rb.empty());
    CHECK(!rb.popBack().has_value());
    rb.push(1);
    rb.push(2);
    rb.push(3);
    CHECK(rb.size() == 3);
    CHECK(*rb.back() == 3);
    CHECK(rb.popBack().value() == 3);
    CHECK(rb.popBack().value() == 2);
    CHECK(rb.size() == 1);
    CHECK(rb[0] == 1);
}

TEST_CASE("RingBuffer overwrites the oldest element when full") {
    RingBuffer<int, 3> rb;
    for (int i = 1; i <= 5; ++i) rb.push(i);
    CHECK(rb.size() == 3);
    CHECK(rb[0] == 3);
    CHECK(rb[1] == 4);
    CHECK(rb[2] == 5);
    CHECK(rb.popBack().value() == 5);
    rb.push(6);
    CHECK(rb[0] == 3);
    CHECK(rb[2] == 6);
}

TEST_CASE("RingBuffer clear resets state") {
    RingBuffer<int, 2> rb;
    rb.push(7);
    rb.push(8);
    rb.push(9);
    rb.clear();
    CHECK(rb.empty());
    CHECK(rb.back() == nullptr);
    rb.push(10);
    CHECK(rb[0] == 10);
}

static_assert([] {
    RingBuffer<int, 2> rb;
    rb.push(1);
    rb.push(2);
    rb.push(3);
    return rb[0] == 2 && rb.size() == 2;
}(), "RingBuffer must be usable in constant expressions");
