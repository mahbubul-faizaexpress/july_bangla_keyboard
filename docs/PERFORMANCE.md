# Performance

Measured results only. Re-run `keyboard_benchmark` after any engine change.

## Engine hot path (Phase 3)

**What is measured:** `Composer::pressKey` / `backspace`, which run for every key in
Bangla mode. TSF edit sessions and application rendering are not included; end-to-end
latency will be measured with ETW in Phase 10.

**Method:**

- A realistic Bijoy keystroke stream (words with conjuncts, pre-base kars, reph, phala,
  spaces and backspaces) is replayed for 10⁴, 10⁵ and 10⁶ events.
- A single event is shorter than the 100 ns QueryPerformanceCounter resolution, so latency
  is timed per batch of 256 events and divided by 256.
- Percentiles are taken over batches.
- Heap allocations are counted by replacing global `operator new`.

**Results:** Release x64, MSVC 19.51, 2026-10-06, one developer machine. Two runs are shown
to give the run-to-run spread.

| Events | Mean (ns/event) | p50 | p95 | p99 | Max (batch avg) | Heap allocations |
|---|---|---|---|---|---|---|
| 10,000 | 58.9 – 64.5 | 58.2 – 59.8 | 59.0 – 87.9 | 59.0 – 118.0 | 81.6 – 163.3 | 0 |
| 100,000 | 59.3 – 64.4 | 58.2 – 59.8 | 58.6 – 91.0 | 84.8 – 102.3 | 160.2 – 256.6 | 0 |
| 1,000,000 | 59.1 – 83.3 | 58.2 – 84.4 | 59.4 – 111.7 | 85.5 – 196.1 | 158.2 – 768.4 | 0 |

**CPU time:**

- 1,000,000 events took 62.5 – 78.1 ms of CPU, which is 62.5 – 78.1 µs per 1,000 events.
- Shorter runs are below the ~15.6 ms resolution of `GetProcessTimes`, so they are not
  reported.

**Memory:**

- Engine state is `sizeof(Composer)` = 176 bytes per TSF thread. `EditResult` is 408 bytes
  and lives on the stack.
- The benchmark process (harness included) used 592 – 632 KB private bytes, with a peak
  working set of 3.5 – 4.0 MB. This is not the companion or TIP footprint; those are
  measured in Phases 7 and 10.

**Interpretation:** about 0.06–0.08 µs per key, with no allocation per key. The worst
batch averages (up to ~0.77 µs) come from OS scheduling noise and are still four orders of
magnitude below a keystroke interval.

## Classic (SutonnyMJ) output (Phase 4)

Same benchmark and stream, with the Composer set to Classic output. Release x64,
2026-10-06, single run.

| Events | Mean (ns/event) | p50 | p95 | p99 | Max (batch avg) | Heap allocations |
|---|---|---|---|---|---|---|
| 10,000 | 159.7 | 159.0 | 168.8 | 171.1 | 186.3 | 0 |
| 100,000 | 160.7 | 158.6 | 166.4 | 217.6 | 308.6 | 0 |
| 1,000,000 | 168.4 | 164.5 | 185.5 | 292.6 | 509.0 | 0 |

**CPU:** 1,000,000 events took 171.9 ms, which is 171.9 µs per 1,000 events.

Classic is about 3× the Unicode cost, because each syllable is re-rendered with
longest-match binary searches over the 222-row table. It is still about 0.17 µs per key,
with no allocation. In the same run, Unicode output measured a mean of 55.0 – 56.4 ns per
event.

## Robustness runs (not performance, recorded here for completeness)

- `july_fuzz` ran 10,000,000 events × 2 in 1.42 s (Release) with all invariants holding and
  a deterministic output hash.
- Under AddressSanitizer (Debug), the unit tests and `july_fuzz` (3 seeds × 1,000,000
  events × 2) produced no reports.
