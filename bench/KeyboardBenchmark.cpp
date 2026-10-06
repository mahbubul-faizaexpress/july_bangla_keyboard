// keyboard_benchmark: Phase 1 skeleton. Engine latency/allocation measurements are added
// in Phase 3 (once there is an engine to measure) and reported in Phase 10.

#include <cstdio>

#include "july/engine/Version.h"

int main() {
    std::printf("keyboard_benchmark (engine %s, layout %s)\n", july::kEngineVersion,
                july::kLayoutVersion);
    std::printf("No benchmarks implemented yet.\n");
    return 0;
}
