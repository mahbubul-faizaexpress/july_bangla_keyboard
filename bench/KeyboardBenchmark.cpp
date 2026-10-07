// keyboard_benchmark: measures the engine hot path (Composer::pressKey / backspace).
//
//   keyboard_benchmark [report.txt]
//
// Feeds a realistic Bijoy keystroke stream (words with conjuncts, kars, reph, phala,
// spaces, a few backspaces) for 10^4, 10^5 and 10^6 events. Latency is measured per batch
// of kBatch events with QueryPerformanceCounter and divided by the batch size, because a
// single event is shorter than the timer resolution; percentiles are over batches.
// Heap allocations are counted by replacing global operator new for this executable.
// This measures the engine only, not TSF or the application (that is Phase 10, ETW).

#include <windows.h>
#include <psapi.h>

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <vector>

#include "july/engine/Composer.h"
#include "july/engine/Version.h"

namespace {

std::atomic<std::size_t> g_allocations{0};

struct Key {
    std::uint16_t scan;
    bool shift;
    bool backspace;
};

// Bijoy keystrokes for: "আমি বাংলায় গান গাই ক্ষমা কর্ম কার্য ব্যাংক প্রকৃতি কৌশল" plus
// a correction (two backspaces). Labels: lowercase = key, uppercase = Shift + key.
std::vector<Key> buildStream() {
    static constexpr const char* kWords[] = {
        "gfdm", "hfQVfW", "ofb", "ofgd", "jgNmf", "jmA", "jfwA", "hZfQj", "rzjadk", "cjXMV",
    };
    static constexpr char kRows[3][11] = {"qwertyuiop", "asdfghjkl", "zxcvbnm"};
    static constexpr std::uint16_t kRowStart[3] = {0x10, 0x1E, 0x2C};
    auto scanOf = [](char lower) -> std::uint16_t {
        for (int row = 0; row < 3; ++row) {
            for (int i = 0; kRows[row][i] != '\0'; ++i)
                if (kRows[row][i] == lower) return static_cast<std::uint16_t>(kRowStart[row] + i);
        }
        return 0;
    };
    std::vector<Key> keys;
    for (const char* word : kWords) {
        for (const char* p = word; *p != '\0'; ++p) {
            const bool shift = *p >= 'A' && *p <= 'Z';
            keys.push_back({scanOf(shift ? static_cast<char>(*p - 'A' + 'a') : *p), shift, false});
        }
        keys.push_back({0x39, false, false});  // space
    }
    keys.push_back({0, false, true});
    keys.push_back({0, false, true});
    return keys;
}

double ticksToNs(LONGLONG ticks, LONGLONG frequency) {
    return static_cast<double>(ticks) * 1e9 / static_cast<double>(frequency);
}

double cpuSeconds() {
    FILETIME creation, exit, kernel, user;
    GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user);
    auto toSeconds = [](const FILETIME& ft) {
        ULARGE_INTEGER v;
        v.LowPart = ft.dwLowDateTime;
        v.HighPart = ft.dwHighDateTime;
        return static_cast<double>(v.QuadPart) / 1e7;
    };
    return toSeconds(kernel) + toSeconds(user);
}

void report(FILE* out, const char* fmt, double a = 0, double b = 0, double c = 0) {
    std::fprintf(stdout, fmt, a, b, c);
    if (out) std::fprintf(out, fmt, a, b, c);
}

} // namespace

// Count every heap allocation made by this process (engine + benchmark harness).
#pragma warning(suppress : 28251)  // the replacement new keeps the standard contract
void* operator new(std::size_t size) {
    ++g_allocations;
    if (void* p = std::malloc(size ? size : 1)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

int main(int argc, char** argv) {
    FILE* out = nullptr;
    if (argc > 1 && fopen_s(&out, argv[1], "w") != 0) out = nullptr;

    constexpr std::size_t kBatch = 256;
    const std::vector<Key> stream = buildStream();
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    std::printf("keyboard_benchmark  engine %s, layout %s\n", july::kEngineVersion, july::kLayoutVersion);
    if (out) std::fprintf(out, "keyboard_benchmark  engine %s, layout %s\n", july::kEngineVersion, july::kLayoutVersion);
    report(out, "timer resolution: %.0f ns; latency = per-event average within %.0f-event batches\n\n",
           1e9 / static_cast<double>(frequency.QuadPart), static_cast<double>(kBatch));

    for (const auto encoding : {july::OutputEncoding::Unicode, july::OutputEncoding::Classic}) {
    const char* encodingName = encoding == july::OutputEncoding::Classic ? "Classic (SutonnyMJ)" : "Unicode";
    std::printf("== %s output ==\n", encodingName);
    if (out) std::fprintf(out, "== %s output ==\n", encodingName);
    for (const std::size_t total : {std::size_t{10'000}, std::size_t{100'000}, std::size_t{1'000'000}}) {
        july::Composer composer(july::ComposerOptions{july::NuktaForm::Precomposed, encoding});
        std::vector<double> batchNs;
        batchNs.reserve(total / kBatch + 1);
        std::size_t sink = 0;  // keeps results observable so the optimizer cannot drop work

        const std::size_t allocationsBefore = g_allocations.load();
        const double cpuBefore = cpuSeconds();
        LARGE_INTEGER runStart, runEnd;
        QueryPerformanceCounter(&runStart);

        std::size_t done = 0;
        std::size_t index = 0;
        while (done < total) {
            const std::size_t n = std::min(kBatch, total - done);
            LARGE_INTEGER t0, t1;
            QueryPerformanceCounter(&t0);
            for (std::size_t i = 0; i < n; ++i) {
                const Key& k = stream[index];
                index = index + 1 == stream.size() ? 0 : index + 1;
                const july::EditResult r = k.backspace ? composer.backspace() : composer.pressKey(k.scan, k.shift);
                sink += r.commit.size() + r.composition.size();
            }
            QueryPerformanceCounter(&t1);
            batchNs.push_back(ticksToNs(t1.QuadPart - t0.QuadPart, frequency.QuadPart) / static_cast<double>(n));
            done += n;
        }

        QueryPerformanceCounter(&runEnd);
        const double cpu = cpuSeconds() - cpuBefore;
        const std::size_t allocations = g_allocations.load() - allocationsBefore;

        std::vector<double> sorted = batchNs;
        std::sort(sorted.begin(), sorted.end());
        auto pct = [&](double p) { return sorted[static_cast<std::size_t>(p * static_cast<double>(sorted.size() - 1))]; };
        double sum = 0;
        for (double v : batchNs) sum += v;

        report(out, "events: %.0f   wall: %.3f ms\n", static_cast<double>(total),
               ticksToNs(runEnd.QuadPart - runStart.QuadPart, frequency.QuadPart) / 1e6);
        report(out, "  per-event latency (ns): mean %.1f   p50 %.1f   p95 %.1f\n", sum / static_cast<double>(batchNs.size()),
               pct(0.50), pct(0.95));
        report(out, "                          p99 %.1f   max %.1f\n", pct(0.99), sorted.back());
        report(out, "  heap allocations in engine loop: %.0f\n", static_cast<double>(allocations));
        // GetProcessTimes advances in scheduler ticks (~15.6 ms), so short runs read 0.
        if (cpu > 0) {
            report(out, "  CPU: %.3f ms   CPU per 1,000 events: %.3f us\n", cpu * 1e3, cpu * 1e9 / static_cast<double>(total));
        } else {
            report(out, "  CPU: below GetProcessTimes resolution (~15.6 ms) for this run length\n");
        }
        report(out, "  (checksum %.0f)\n\n", static_cast<double>(sink));
    }
    }

    PROCESS_MEMORY_COUNTERS_EX mem{};
    GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&mem), sizeof mem);
    report(out, "process memory: private %.0f KB   peak working set %.0f KB\n",
           static_cast<double>(mem.PrivateUsage) / 1024.0, static_cast<double>(mem.PeakWorkingSetSize) / 1024.0);
    report(out, "note: memory includes the benchmark harness (latency vectors), not only the engine.\n");
    report(out, "sizeof(Composer) = %.0f bytes, sizeof(EditResult) = %.0f bytes\n",
           static_cast<double>(sizeof(july::Composer)), static_cast<double>(sizeof(july::EditResult)));

    if (out) std::fclose(out);
    return 0;
}
