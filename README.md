# CS 253 / MA 253 Lab Project 9: Event Scheduler — Heap Against Balanced Tree

A rigorous, evidence-based C++17 comparative study of priority queue architectures for discrete-event network simulation under cancellation-heavy workloads.

---

## 1. Project Purpose & Architecture

This repository evaluates two core event scheduling structures:
- **Structure A (`src/structA/`):** A contiguous binary min-heap keyed by `(time, sequence number)` with lazy cancellation recorded in a from-scratch `CustomHashSet` (open addressing, linear probing, backward-shift deletion) and an amortized $O(1)$ purge policy triggering when cancelled entries strictly outnumber live entries ($C > L$).
- **Structure B (`src/structB/`):** An AVL balanced search tree keyed by `(time, sequence number)` featuring true node deletion on `cancel()`, explicit rotation tracking, and a unified `successor(t)` definition ($\text{time} \ge t$).
- **Application (`app/`):** A network sender simulator with retransmission timers and probabilistic packet acknowledgments, runnable for $\ge 1,000,000$ events with identical outputs across structures under the same configuration and seed.
- **Oracle & Testing (`oracle/`):** A sorted-array oracle with binary-search insertion, an automatic test-case shrinker (Delta Debugging `ddmin`), and a 1,000,000-operation differential fuzzing suite.
- **Adversary (`adversary/`):** Generators for worst-case AVL rotation stress (1.000 rotations/insertion) and heap purge prevention ($2.00\times$ physical storage bloat).
- **Benchmarks (`bench/`):** Empirical hold model across 4 increment distributions (Exponential, Uniform, Bimodal, Triangular) and cancellation rates from 0% to 95%, with 5 repetitions per point, spread, and hardware cycle profiling (`__rdtsc`).
- **Derivations (`docs/derivations/`):** Individually attributed mathematical proofs for D1 through D6 in PDF format.

---

## 2. Repository Layout

```
.
├── app/                        # Network sender simulation application
│   └── main.cpp                # CLI entry point for simulator
├── src/
│   ├── structA/                # Structure A: Binary Min-Heap + Scratch Hash Set
│   │   ├── custom_hash_set.hpp # From-scratch open-addressing hash set
│   │   └── heap.hpp            # Binary min-heap and HeapEventSet wrapper
│   ├── structB/                # Structure B: AVL Balanced Search Tree
│   │   └── avl.hpp             # AVL tree with true cancellation and successor(t)
│   ├── simulator.cpp           # Discrete-event network simulation engine
│   └── main.cpp                # Legacy simulator bridge
├── include/                    # Public API headers
│   ├── event.hpp               # Event structure & total ordering (time, seq)
│   ├── event_set.hpp           # Abstract EventSet interface
│   ├── heap.hpp                # Bridge header forwarding to structA::HeapEventSet
│   ├── avl.hpp                 # Bridge header forwarding to structB::AvlEventSet
│   ├── simulator.hpp           # Simulator declarations
│   └── rng.hpp                 # SplitMix64 deterministic PRNG
├── oracle/                     # Ground-truth verification & fuzzing
│   ├── oracle.hpp              # SortedArrayOracle with binary-search insertion
│   ├── shrinker.hpp            # Delta Debugging (ddmin) automatic test shrinker
│   └── differential_test.cpp   # >= 1,000,000 operations differential test
├── adversary/                  # Adversarial input generators
│   ├── avl_adversary.cpp       # Monotonic & zigzag streams forcing rotations
│   └── heap_adversary.cpp      # Purge-prevention stream causing storage bloat
├── bench/                      # Performance benchmarking & profiling
│   └── hold_bench.cpp          # Hold model & cancellation sweeps with __rdtsc
├── tests/                      # Unit and regression test suites
│   ├── test_suite.cpp          # Focused unit tests (ties, cancels, successor)
│   ├── d3_tests.cpp            # Deterministic ordering tests
│   └── differential_test.cpp   # Differential test bridge
├── docs/
│   └── derivations/            # Formal individually authored mathematical proofs
│       ├── D1.pdf              # Heap purge amortized bound & hash set dynamics
│       ├── D2.pdf              # AVL height upper bound via Fibonacci trees
│       ├── D3.pdf              # Determinism & timestamp-order preservation
│       ├── D4.pdf              # AVL invariant checker soundness & completeness
│       ├── D5.pdf              # Insertion rotation bound & adversarial stream
│       └── D6.pdf              # Hold-model predicted cost model vs measurements
├── results/
│   ├── raw/                    # Raw CSV benchmark logs
│   │   ├── hold_model.csv      # Hold model throughput across 4 distributions
│   │   ├── cancellation_sweep.csv # Throughput across 0% to 95% cancellations
│   │   ├── avl_adversary.csv   # Rotations vs insertions
│   │   ├── heap_adversary.csv  # Physical storage vs inserted events
│   │   └── cache_profile.csv   # Footprints and CPU cycles per operation
│   └── figures/                # Publication-quality vector SVG charts
│       ├── fig1_hold_throughput.svg
│       ├── fig2_cancellation_sweep.svg
│       ├── fig3_avl_rotations.svg
│       ├── fig4_heap_storage_bloat.svg
│       ├── fig5_cache_cycles.svg
│       └── generate_figures.py # Standalone SVG chart generator
├── scripts/                    # Build, execution, and document authoring scripts
│   ├── run_all.sh              # Master one-command execution script (Bash)
│   ├── run_all.ps1             # Master one-command execution script (PowerShell)
│   ├── make_pdf.py             # Pure Python PDF generation library
│   ├── generate_derivations.py # Compiles D1.pdf through D6.pdf
│   ├── generate_slides.py      # Compiles docs/slides/slides.pdf
│   └── generate_report_pdf.py  # Compiles REPORT.pdf
├── INTERFACE.md                # Shared interface and structural invariant contracts
├── REPORT.md                   # Comprehensive technical report
├── REPORT.pdf                  # Compiled PDF report
├── TOOLS.md                    # AI and development tools disclosure
└── README.md                   # This project guide
```

---

## 3. Quick Start: One Command Execution

To compile everything, execute the unit tests, run the 1,000,000-operation differential test and shrinker, run the network application on both structures, run adversarial generators, collect benchmark logs, and render all figures and PDFs:

### Linux / macOS / MSYS2 (Bash):
```bash
bash scripts/run_all.sh
```

### Windows (PowerShell):
```powershell
powershell -ExecutionPolicy Bypass -File scripts/run_all.ps1
```

---

## 4. Manual Build and Run Instructions

### 1. Build the Network Simulator:
```bash
g++ -std=c++17 -O3 -Iinclude app/main.cpp src/simulator.cpp -o sim
```

### 2. Run the 1,000,000-Event Simulation:
```bash
# Run with Structure A (Heap)
./sim --structure heap --events 1000000

# Run with Structure B (AVL)
./sim --structure avl --events 1000000
```

#### Expected Output (Seed 12345, default parameters):
Both structures will print **exactly identical output**:
```
processed=337910 scheduled=1000036 cancelled=662126 retransmissions=31314 maxLive=248 lastTime=284441
cancelled%=66.2
traceHash=15d0491cecb33bd5
```

### 3. Run the Unit & Regression Test Suite:
```bash
g++ -std=c++17 -O3 -Iinclude tests/test_suite.cpp src/simulator.cpp -o test_suite
./test_suite
```
*Expected: 83 / 83 tests passed (covers ties, cancellations, repeated cancellations, empty structures, purge threshold, and successor semantics).*

### 4. Run the 1,000,000 Operations Differential Test & Shrinker:
```bash
g++ -std=c++17 -O3 -Iinclude oracle/differential_test.cpp -o diff_test
./diff_test 1000000
```
*Expected: Automatic Shrinker verification passes (reducing 104 noisy ops to 4 minimal ops), followed by 1,000,000 operations with 0 mismatches across Oracle, Heap, and AVL.*

### 5. Run Adversarial Generators:
```bash
g++ -std=c++17 -O3 -Iinclude adversary/avl_adversary.cpp -o avl_adv
./avl_adv

g++ -std=c++17 -O3 -Iinclude adversary/heap_adversary.cpp -o heap_adv
./heap_adv
```

### 6. Run Hold Model & Cancellation Benchmarks:
```bash
g++ -std=c++17 -O3 -Iinclude bench/hold_bench.cpp -o hold_bench
./hold_bench
```

### 7. Render Charts & Generate PDFs:
```bash
python results/figures/generate_figures.py
python scripts/generate_derivations.py
python scripts/generate_slides.py
python scripts/generate_report_pdf.py
```

---

## 5. Summary of Results

1. **Equivalence:** Structure A, Structure B, and SortedArrayOracle produce identical traces under all identical seeds and configurations (`traceHash = 15d0491cecb33bd5`).
2. **Heap Optimization:** Replacing the legacy fixed 100,000-bucket clear loop with `CustomHashSet` maintaining dynamic index tracking improved 1M-event simulation speed by **$5.8\times$** (from 1.08s down to 0.18s), matching the theoretical $O(1)$ amortized cancellation bound (D1).
3. **Hold Model Throughput:** For queue sizes $M \le 10,000$, Structure A achieves **$1.8\times$ to $2.4\times$ higher throughput** than Structure B across all four distributions due to flat array cache locality (150–350 CPU cycles vs 380–750 CPU cycles for AVL).
4. **Crossover:** At $M = 100,000$ (2.4 MB working set, exceeding CPU L2 cache), the Heap advantage drops to $1.13\times$ as sift-down incurs L3 misses. At 25%–50% cancellation rates, AVL slightly outperforms Heap because true deletion immediately frees nodes.
5. **Adversarial Stress:**
   - AVL Monotonic stream forced 499,981 rotations for 500,000 insertions (**1.000 rotations/insertion**), exactly matching $R(N) = N - h(N)$ (D5).
   - Heap purge-prevention stream forced **0 purges** over 250,000 operations, resulting in a **$2.00\times$ physical storage bloat** ($P = 2L$).

---

- Paste the official team repository URL into the Git configuration before final submission.

