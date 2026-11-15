# Integrated validation record — 10 November 2026

Environment: Linux, GCC 14.2.0. Optimized build: -std=c++17 -O2 -Wall -Wextra -Wpedantic, no compiler diagnostics.

## Passed

- Framework distribution tests and 48 trace configurations, each replayed on reference, uploaded/adapted heap and uploaded/adapted AVL (144 backend validations). N=1,10,100; cancellation r=0,0.5,0.95,1; four distributions. Same-seed trace repeatability checked.
- Structure and AVL index checks during validation, plus exact final drains.
- Targeted team contracts: repeated/missing cancellation, duplicate ID rejection, empty delete-min, tie order and independent instances.
- Heap regression: C=L before deleting a live minimum; purge triggers immediately after deletion makes C>L. Cancelling the last live event leaves no physical entries.
- Rotation regressions: LL and RR count one primitive rotation; LR and RL count two. Trees and indexes remain valid after deletion.
- Deliberately corrupted-minimum backend rejected by differential validation.
- Integrated self-tests and targeted regressions passed AddressSanitizer + UndefinedBehaviorSanitizer with ASAN_OPTIONS=detect_leaks=0. LeakSanitizer is disabled because it failed under this environment's ptrace restriction in the earlier run; no leak-check claim is made.
- Python scripts compile and analysis runs end to end.
- Full exploratory grid: 1,440 timed trials / 720 pairs. All paired checksums match; maximum sampled heap H/L=1.999. Every trial passed validation before timing.
- Original uploads preserved byte-for-byte in team_originals; source digests recorded in SOURCE_HASHES.json.

- Longer focused runs: 20 timed trials, 10 matched pairs; each timer 0.341–2.559 seconds. All pre-timing validations passed. Results and uncertainty are recorded in RESULTS.md.

## Limits

Exploratory trials are short (all below 0.2 seconds) and some ran alongside correctness tool activity. They verify coverage and show preliminary patterns, not report-quality speed rankings. Longer focused results are described separately in RESULTS.md. Shared-machine timings remain environment-specific. Metrics are sampled outside timers. Accounted memory excludes allocator overhead and temporary purge peaks. The supplied CMake path remains untested because CMake is unavailable here; direct GCC commands were executed.
