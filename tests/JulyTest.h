#pragma once

// Minimal self-registering test harness (no third-party dependency).

#include <cstdio>

namespace july::test {

using TestFn = void (*)();

struct TestCase {
    const char* name;
    TestFn fn;
    TestCase* next;
};

TestCase*& registry() noexcept;
void reportFailure(const char* file, int line, const char* expr) noexcept;

struct Registrar {
    TestCase node;
    Registrar(const char* name, TestFn fn) noexcept : node{name, fn, registry()} {
        registry() = &node;
    }
};

} // namespace july::test

#define JULY_TEST_CONCAT_(a, b) a##b
#define JULY_TEST_CONCAT(a, b) JULY_TEST_CONCAT_(a, b)

#define TEST_CASE(name)                                                                 \
    static void JULY_TEST_CONCAT(julyTest_, __LINE__)();                                \
    static ::july::test::Registrar JULY_TEST_CONCAT(julyReg_, __LINE__){               \
        name, &JULY_TEST_CONCAT(julyTest_, __LINE__)};                                  \
    static void JULY_TEST_CONCAT(julyTest_, __LINE__)()

#define CHECK(expr)                                                                     \
    do {                                                                                \
        if (!(expr)) ::july::test::reportFailure(__FILE__, __LINE__, #expr);            \
    } while (false)
