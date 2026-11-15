# P6: Event-set benchmarking module

Prepared from the six-person plan supplied in the conversation. The original assignment sheet was not supplied. The subsequently uploaded avl.cpp and minheap.cpp are now integrated, with corrections documented in docs/INTEGRATION_CHANGES.md. Original uploads are preserved byte-for-byte in team_originals/.

## Start here

1. Read `docs/EXPLANATION.md` for the project, logic and worked example.
2. Read `docs/D6_COST_MODEL.md` for the pre-experiment prediction.
3. Compile and run the demo below.
4. Read `docs/INTEGRATION_CHANGES.md` to understand the fixes and adapters.
5. Run the full suite on one machine and analyze the output.

## Build (Ubuntu, or Windows with a C++17 compiler)

Direct GCC build, from this directory:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic src/benchmark.cpp -o p6_bench
./p6_bench --self-test
```

Windows MinGW: name the output `p6_bench.exe`, then use `./p6_bench.exe` in PowerShell.
CMake alternative (provided, but not exercised in the authoring environment):

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## One runnable example

`results` already exists. Output prefix controls two CSV filenames. Do not reuse it for results you need to keep.

```sh
./p6_bench demo 1000 100000 10000 0.70 exponential 123456789 10 results/demo
python -m pip install -r requirements.txt
python analyze.py results
```

Arguments: mode, live size N, measured cycles, warm-up cycles, cancellation fraction, distribution, base seed, repetitions, output prefix. Each cycle is TWO event-set API operations: removal + insertion. Throughput is `2 * cycles / elapsed seconds`.

`demo` uses `std::set` and `std::map` to exercise the harness. It is not a binary heap, not an AVL implementation and not a valid substitute for either team's code. It is explicitly labelled `reference_demo` in every result. `team` now runs the integrated uploaded heap and AVL implementations.

## Team experiment (already integrated)

```sh
python run_suite.py --exe ./p6_bench --mode team --out results/team_run --sizes 100,1000,10000 --steps 100000 --warmup 10000 --repeats 10 --build-description "GCC version here; -std=c++17 -O2; team commit here"
python analyze.py results/team_run
```

The default grid has 3 sizes × 12 cancellation fractions × 4 distributions × 10 trials per backend. At rate zero it is the basic hold model. Increase steps until each timed trial lasts at least about 0.2–1 seconds. Increase warm-up relative to N and check sensitivity by doubling it. Defaults are starting settings, not proof of steady state. Use an empty output directory for a new suite. Keep the machine plugged in, avoid concurrent workloads, record CPU and compiler details in the build description.

## Included files

| File | Responsibility |
|---|---|
| `src/event_set.hpp` | Shared event, interface and metric definitions |
| `src/benchmark.cpp` | Distributions, trace generation, validation, timing and CSV |
| `src/reference.hpp` | Demonstration backend and generation oracle |
| `src/team_adapters.hpp` | Working adapters for the uploaded P1/P2 code |
| `src/team/` | Corrected heap and AVL cores plus a custom AVL ID lookup map |
| `team_originals/` | Original uploaded files |
| `run_suite.py` | Experiment grid, paired seeds and environment record |
| `analyze.py` | Summary statistics, bootstrap intervals, paired speedup and plots |
| `docs/D6_COST_MODEL.md` | Predictions and assumptions made before comparison |
| `docs/EXPLANATION.md` | Beginner-friendly explanation and review of the plan |
| `docs/INTEGRATION.md` | Adapter example and metric contract |
| `tests/fault_detection.cpp` | Deliberately incorrect backend rejection test |
| `results/example/` | Actual short reference-only smoke-run outputs |

## Deliverable status

Implemented and tested: uploaded heap/AVL integration, regression fixes, framework, four distributions, exact trace replay, sampled structural checks, warm-up, repeated trials, CSV, analysis and plots. Team exploratory runs are in results/team_exploratory/. Read docs/RESULTS.md for scope and limitations. Unknown or inapplicable metrics remain -1. Memory is accounted structure storage, not process RSS or allocator-exact peak memory. Timing trials are short exploratory measurements; increase duration before making final performance claims.

This is a P6 module, not the P3 network simulator or P4's independent sorted-array oracle. The use of standard containers here is confined to P6's untimed trace builder and demonstration backend. If the original assignment bans standard containers throughout the project, replace the reference backend with P4's oracle before submission.

## Included team results

- `results/team_exploratory`: all 144 configurations (3 sizes × 12 rates × 4 distributions), five paired seeds each. These short trials provide coverage and preliminary plots.
- `results/team_long`: focused longer runs at N=1000 with exponential increments, rates 0 and 0.95, five paired seeds, 3,000,000 measured cycles and 10,000 warm-up cycles. See `docs/RESULTS.md`.

To reproduce one focused case:

```sh
./p6_bench team 1000 3000000 10000 0.95 exponential 123456789 5 results/long_cancel95
```

This pre-generates a large trace in memory. If memory is limited, reduce steps, but retain enough measured duration. Report the chosen duration and repeat count. For your final submission, use more seeds and repeat on the team's target machine.
