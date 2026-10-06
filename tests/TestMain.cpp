#include "JulyTest.h"

namespace july::test {

namespace {
int g_failures = 0;
}

TestCase*& registry() noexcept {
    static TestCase* head = nullptr;
    return head;
}

void reportFailure(const char* file, int line, const char* expr) noexcept {
    ++g_failures;
    std::fprintf(stderr, "%s(%d): CHECK failed: %s\n", file, line, expr);
}

} // namespace july::test

int main() {
    using namespace july::test;
    int ran = 0;
    for (TestCase* t = registry(); t != nullptr; t = t->next) {
        const int before = g_failures;
        t->fn();
        ++ran;
        if (g_failures != before) std::fprintf(stderr, "FAILED: %s\n", t->name);
    }
    std::printf("%d test cases, %d failed checks\n", ran, g_failures);
    return g_failures == 0 ? 0 : 1;
}
