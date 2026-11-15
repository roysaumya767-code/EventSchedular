# Integration is complete

`src/team_adapters.hpp` registers two working backends named heap and avl. Compile src/benchmark.cpp; it includes the adapted headers directly. Do not additionally compile the originals, which contain standalone main functions.

Run from the package directory:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic src/benchmark.cpp -o p6_bench
./p6_bench --self-test
./p6_bench team 1000 100000 10000 0.70 exponential 123456789 10 results/team
python analyze.py results
```

Read INTEGRATION_CHANGES.md before submission. It lists every substantive adaptation. The original uploads are in team_originals/.

The common EventSet interface supplies insert, delete_min, cancel, size, validate_structure and metrics. Event ID also serves as the unique sequence number. Cancellation must return false for absent or already cancelled IDs. size means live size. The heap returns complete live events; the AVL maps IDs to full keys through a custom hash map.

Metrics fields: physical = stored entries; bytes = accounted structure storage; purges = lifetime rebuild count; purged_entries = lifetime physical inputs scanned by rebuilds; rotations = lifetime primitive rotations; height = current AVL height with leaf=0. A value of -1 means not applicable/unavailable. Counters include initialization and warm-up; diagnostic_summary.csv subtracts the warm-up boundary.

To use integer simulation timestamps later, explicitly choose a time-unit/rounding policy and apply it in the trace generator before generating expected outcomes. Silently rounding only in an adapter breaks fair comparison. This version preserves fractional generated timestamps.

Run tests/team_tests.cpp for targeted regressions. Timing builds should be optimized; sanitizer builds are for correctness only. Increase steps and warm-up for final report measurements, and preserve compiler flags, code versions and raw data.
