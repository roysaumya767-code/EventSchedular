# What P6 has done, and why

## 1. Understand the whole project

The project compares two ways to store future events. A min-heap is good at finding the earliest event. An AVL tree keeps events in sorted order and supports deletion and successor queries. A network simulator produces events such as packet transmission, ACK arrival and timeout. When an ACK arrives, its timeout can be cancelled.

P1 implements the heap and lazy cancellation. P2 implements the AVL tree. P3 builds the simulator. P4 verifies correctness. P5 produces difficult workloads. P6 designs fair experiments and explains their results. P6 must not report numbers from incorrect implementations or change the workload to favour one implementation.

This module handles P6's synthetic experiments. It does not simulate packets. A later simulator-based experiment is complementary: application behaviour may produce a different cancellation pattern from uniformly selected live events.

## 2. Corrections and clarifications to the supplied plan

- A hold cycle removes exactly one event and inserts exactly one event. Live size is exactly N at cycle boundaries and N-1 between those two calls, not merely approximately constant.
- “Cancellation percentage” is ambiguous. Here r is the probability that a removal is a cancellation instead of delete-min. Every removal succeeds and is followed by an insertion. At r=0.70, approximately 70% of removals are cancellations, but only approximately 35% of all API calls are cancellations. CSV records the realised fraction too.
- The heap bound H<=2L applies after an operation's purge policy has been enforced. Cancellation briefly may make C>L. Deleting live entries can also trigger that condition, so checking only cancel() is insufficient. At L=0 all physical entries must be cleared. H/L is undefined when L=0.
- Bounding physical entries does not bound allocated memory unless vector capacity, hash-table buckets, tombstones and indexes are managed too. A vector may retain a large capacity after a purge.
- A lazy-cancellation hash lookup is expected O(1) under suitable hashing and load management; it is not automatically worst-case O(1). The amortized heap bound is not a per-call latency guarantee.
- Cancelling an AVL event by ID requires locating its key. Provide an ID-to-key index or a safe handle strategy; otherwise a linear search destroys the claimed O(log N) cancellation bound. Count the index's memory and runtime.
- For the Fibonacci AVL formula, state the convention: empty height -1 and leaf height 0 gives minimum nodes N(h)=F(h+3)-1. Changing the height convention shifts the formula.
- A checker must enforce BST bounds for the entire left/right subtree, not only immediate children. Height/balance correctness assumes a finite, acyclic, non-shared tree; robust checking also detects cycles or shared nodes.
- Define successor: first key strictly greater than (time,sequence), or first timestamp strictly greater than t? These are different with ties. P6's current benchmarks do not call successor.
- A global sequence counter provides total ordering for finite valid times and non-reused IDs. Full simulator determinism additionally requires deterministic generation, cancellation and random-number use. Reject NaN timestamps and prevent counter overflow.
- One AVL insertion has at most one rebalance location; a double rotation contains two primitive rotations. Deletion can rebalance at multiple ancestors. Record primitive rotations consistently.
- Both structures being O(log N) does not predict a universal winner or guarantee a crossover. Allocation, cache locality, stale-root cleanup and workload shape matter.

## 3. The central idea: generate once, replay identically

Imagine giving two students different exam papers and comparing their speed. That is not a fair test. The same problem occurs if heap and AVL independently generate random workloads.

`generate()` builds a complete operation trace outside the timer. Each step records the event to remove and the event to insert. Every backend receives that exact trace. A reference ordered set determines which event is truly minimum. A vector stores currently live events, while an ID-to-position map allows removal by swapping with the last element. A uniformly sampled vector position chooses a valid cancellation target without retries over dead IDs.

The generator's use of maps, random numbers and memory is outside the event-set timer. Its cost still affects how long the overall experiment takes and its peak process memory, so process RSS is not reported as structure memory.

## 4. A small worked example

Start with events (2,0), (5,1), (8,2), where the second number is the unique sequence ID. N=3 and simulation time now=0.

Cycle 1: delete-min returns (2,0). Set now=2. Suppose increment=1.5. Insert (3.5,3). Live size returns to three.

Cycle 2: cancel ID 2, which removes (8,2). Cancellation does not process that future event, so now remains 2. Suppose increment=0.4. Insert (2.4,4). Again there are three live events.

Cycle 3: delete-min must now return (2.4,4), not (3.5,3). Both implementations must return the same event.

For a cancellation step the inserted time is now+increment, not cancelled_event.time+increment. This choice makes the mixed cancellation experiment an explicitly defined extension. At r=0 it reduces to the usual hold model.

## 5. Why use four distributions?

The increment says how far into the future to schedule a new event. All four choices here have theoretical mean 1, so comparison changes distribution shape without changing its mean.

| Distribution | Parameters | Generator logic | Interpretation |
|---|---|---|---|
| Exponential | rate lambda=1 | -ln(1-U) | Many short waits, occasional long waits |
| Uniform | [0,2) | 2U | Equal density over a bounded interval |
| Triangular | low 0, mode 1, high 2 | sqrt(2U) below U=0.5; otherwise 2-sqrt(2(1-U)) | Middle waits are more frequent |
| Bimodal | equal mixture of U[0.1,0.3) and U[1.7,1.9) | Choose a band, then sample inside it | Two separate groups of waits |

`Random` uses a seeded mt19937_64 generator. `unit()` maps 53 random bits to [0,1), preventing log(0) at U=1. The index sampler uses rejection sampling to avoid modulo bias. Floating-point library implementations may differ across platforms, so same-seed replay is guaranteed within the experiment environment, not claimed bit-identical across all operating systems.

## 6. Checking before measuring

`validate()` creates a fresh backend, replays every step, checks every returned minimum and cancellation result, and checks live size after each cycle. It then drains and compares all remaining events. This catches leftover-event corruption that a prefix of successful delete-mins might miss.

A second fresh backend is used for timing. Initialization and warm-up happen first. The timed loop does removal plus insertion and a small rolling checksum. The checksum makes returned IDs observable. Checks, CSV writing, random generation, oracle work and plotting are excluded from timing. Interface dispatch, trace reads and checksum overhead remain; both implementations face the same overhead.

Validation is not a substitute for P4's structural AVL checker. A tree can return correct results on one trace and still violate an invariant or have a latent bug.

## 7. Warm-up, trials and order

Warm-up lets the initial queue evolve before measurement. It is a practical control, not proof that the queue has reached steady state. Try a longer warm-up and verify that conclusions do not change.

Each repetition uses seed+repeat. Both structures use the same seed within a pair. Run order alternates for two backends so one backend is not always measured first. They run sequentially, never simultaneously.

The first reference-only sample is a smoke test. Its short durations should not be treated as stable performance estimates. For submission use at least 10 paired seeds and longer timed loops, then repeat batches to assess machine noise.

## 8. What the outputs mean

`*_raw.csv` contains one row per backend/trial: configuration, actual cancellation rate, seconds, throughput and checksum. `summary.csv` reports mean, median, sample standard deviation and an approximate 95% percentile-bootstrap interval for the mean throughput (2,000 resamples). Intervals are omitted for fewer than five trials; ten or more are recommended. Resampling assumes independent trials. It does not remove systematic machine bias.

`paired_speedup.csv` uses the same seed's AVL time divided by heap time. Above 1 means heap was faster; below 1 means AVL was faster. A confidence interval containing 1 means these trials do not clearly establish a winner at that setting. Compare adjacent measured rates when discussing a crossover; do not invent an exact threshold between unmeasured points.

Throughput plots keep distribution and either cancellation fraction or N fixed. Shading shows uncertainty where available. Diagnostic occupancy plots use the first seed and show sampled H/L. They do not measure every transient maximum.

The uploaded implementations are now connected. See INTEGRATION_CHANGES.md for timestamp changes, counter fixes and the AVL ID index. Structural checks run every cycle for N<=100, every 1,024 cycles for larger N, and before the final drain.

A separate untimed diagnostic replay reads physical entries, allocated bytes, purge counters, purged entries, rotations and height. Unknown values remain -1. Counters are cumulative and include warm-up; subtract a warm-up-boundary sample to isolate the measured interval. `purged_entries` is the total number of physical entries scanned during purges, so its delta divided by purge-count delta gives mean purge input size.

## 9. How to explain your contribution in a viva

“I designed a controlled experiment for the heap and AVL event sets. I generate one reproducible workload and replay it on both. I first verify correctness against an oracle. Then I measure removal-plus-insertion time after warm-up, using repeated paired seeds. I vary queue size, cancellation rate and increment distribution. I collect diagnostics separately so they do not distort the main timer. Finally, I report uncertainty and compare the observations with a theoretical cost model.”

Why not time once? A single run can reflect background processes or an unusual random workload.

Why not include random generation? We want event-set performance, not differences in random-number generation.

Why could a heap cancellation be fast but the run still slow? Lazy cancellation defers cleanup to later root removals or purges; deferred work still has a cost.

Why not announce a universal winner? The integrated implementations have only exploratory timings here. Short trials, machine effects and implementation details prevent a general conclusion. See RESULTS.md.

## References and scope

Douglas W. Jones, “An Empirical Comparison of Priority-Queue and Event-Set Implementations,” Communications of the ACM 29(4), 300–311, 1986. Paper: https://www.cs.odu.edu/~cmo/classes/old/cs475sp05/papers/jones86.pdf
Author's algorithm collection: https://homepage.cs.uiowa.edu/~dwjones/event/

The paper motivates constant-size hold-model measurement. This module makes its own initialization, distribution parameter, warm-up, allocation and cancellation choices. In particular, it initializes N sampled events and then warms up; it does not reproduce Jones's mixed enqueue/dequeue queue-construction method. Treat it as a modern project experiment, not a replication of his numerical results.
