# Event Scheduler: heap vs AVL tree — P3 (Simulation Architect) part

## What is in this folder
| Path | What it is |
|---|---|
| `include/event.hpp` | The `Event` and the ordering rule `(time, seq)` |
| `include/event_set.hpp` | The interface (socket) that P1 (heap), P2 (AVL), P4 (oracle) must plug into |
| `include/simulator.hpp`, `src/simulator.cpp` | The network simulator (sender + retransmission timers + cancellations) |
| `include/rng.hpp` | Our own random number generator (same numbers on every computer) |
| `include/heap.hpp`, `include/avl.hpp` | P1 and P2 code connected to the interface |
| `include/simple_sets.hpp` | TEMPORARY stand-ins for testing until the real structures arrive |
| `src/main.cpp` | The program `./sim --structure ...` |
| `tests/d3_tests.cpp` | Tests for D3 (sorted vector, std::set, heap, AVL, and the retry limit) |
| `tests/differential_test.cpp` | Random test comparing all structures |
| `proofs/D3_determinism.md` | The proof, with examples and practice problems |

## Build and run (Mac Terminal)
```
g++ -std=c++17 -O2 -Iinclude src/main.cpp src/simulator.cpp -o sim
g++ -std=c++17 -O2 -Iinclude tests/d3_tests.cpp src/simulator.cpp -o d3_tests
./d3_tests
g++ -std=c++17 -O2 -Iinclude tests/differential_test.cpp src/simulator.cpp -o diff_test
./diff_test
./sim --structure heap --events 1000000
./sim --structure avl  --events 1000000
```
Options: `--events N --seed S --ack 0..1000 --timers K --rto T --maxdelay D --gap G --retries R`.
Bad or missing values stop the program with an error message. Use `--timers 40` to reach about 95% cancelled events (default 4 gives about 66%).

Every structure (sortedvec, stdset, heap, avl) must print the same three lines (same `traceHash`).

## How P1 and P2 are connected (done)
* `include/heap.hpp` = P1's heap + lazy deletion (from `minheap.cpp`) wrapped as `HeapEventSet`.
* `include/avl.hpp`  = P2's AVL tree (from `avl.cpp`) wrapped as `AvlEventSet`.
* Their original files stay untouched in the repo. The small changes I made are marked `P3 change` in the comments.
* `./sim --structure heap` and `./sim --structure avl` now work.

## Results (1,000,000 scheduled events, seed 12345)
All four structures print exactly the same three lines (same `traceHash`).
`tests/differential_test.cpp`: 600,000 random operations (many ties, many cancels, double cancels) on stdset, heap and AVL
compared with the sorted-vector oracle after every step, and `validateAvl` after every step: **0 mismatches**.

Run time on the 1M-event simulation: sortedvec 0.14 s, stdset 0.18 s, AVL 0.19 s, **heap 1.08 s**.

## Things the team should look at (found while connecting)
1. **Mac build:** the original `minheap.cpp` uses `#include <bits/stdc++.h>`, which does not exist on Mac (clang). Use normal includes.
2. **P1 purge cost:** `id_set::clear()` loops over all 100,000 buckets, twice per purge. This run does 3,797 purges with only ~250 live events, so purges cost far more than the live size. This is why the heap is ~5x slower. D1 says "amortised O(log n)"; with this fixed 100,000 term that is not true for small n. Fix idea: clear only buckets that were used, or size the table to the live count. P6 must know this before writing the cost model (D6).
3. **P2 rotation counter:** `leftRotations` was never increased (fixed in `avl.hpp`). P5/P6 rotation data from the original file is wrong.
4. **P2 `successor(t)`** returns the first event with time `>= t`. The project text says "the next event after a given time" (maybe strictly `>`). Please agree on one meaning and write it in the report.
5. **P2 Event** stored a `string` type and the comparator copied events by value, which is slow (fixed in `avl.hpp`).
6. **P1 `cancel_event`** returned `true` when cancelling an already-cancelled event (fixed in `heap.hpp`).

## Put it on GitHub (first time)
1. Make a free account at github.com and create a new empty repository (ask your team for its link; if the team already has one, use that).
2. In Terminal: `cd` into this folder, then
```
git init
git add .
git commit -m "P3: simulator, event ordering, D3 proof"
git branch -M main
git remote add origin <PASTE-THE-REPO-LINK-HERE>
git push -u origin main
```
(If the team repo already exists: `git clone <link>`, copy these files in, then `git add .`, `git commit -m "P3 work"`, `git push`.)
