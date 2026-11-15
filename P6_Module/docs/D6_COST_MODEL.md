# D6: pre-experiment cost model

This is a prediction and reasoning document, not a claim about measured team performance.

Let N be the live event count at cycle boundaries, M the measured cycles, r the fraction of removals that cancel, H the physical heap entries and C=H-L stale entries. There are M insertions, approximately (1-r)M delete-min calls and rM valid cancellations. Use log(N+1) to include N=1.

## Heap model

A useful accounting model for total time is:

T_heap ≈ M*a_i*log(N+1) + (1-r)*M*a_d*log(N+1)
         + r*M*a_c + K*a_s*log(N+1) + a_p*S + lookup/allocation overhead.

K counts stale entries individually popped at the root. S is the sum of physical entries scanned by all purges. Constants are empirical, depend on the implementation and machine, and need not stay identical across distributions. Do not count stale entries removed by purge in K as well.

The model assumes expected constant-time hash operations, including membership checks and appropriate resizing. Each stale entry can be individually removed at most once, so K is bounded by prior successful cancellations, including warm-up debt when measuring only a suffix. If C>L at a purge, H=L+C<2C. A linear filter plus bottom-up heapify costs O(H)=O(C), which can be charged to the C cancelled entries being discarded. Those entries are not charged again at another purge. Across the complete run, purge work is therefore linear in successful cancellations, plus initialization. Heap rebuilding by repeated insertion would cost more and does not meet this linear-rebuild assumption.

After enforcement, C<=L, hence H=L+C<=2L. In a fixed-size cycle, L is N or N-1; array growth is bounded in terms of N only if capacity management is also appropriate. Combined with amortized accounting for stale-root pops, the total expected amortized operation cost remains O(log(N+1)). Individual delete-min or purge calls can be much slower than this average. Hash adversaries or unreclaimed auxiliary tables invalidate stronger guarantees.

## AVL model

T_avl ≈ M*b_i*log(N+1) + (1-r)*M*b_d*log(N+1)
        + r*M*b_c*log(N+1) + b_rot*R + allocation/index overhead.

R counts primitive rotations, including two for a double rotation. Balance bounds height by O(log N), so insertion, deletion and key successor are O(log N). Cancellation needs an efficient ID-to-key lookup. A custom expected-O(1) hash index plus O(log N) deletion preserves the expected bound. A logarithmic ID index also preserves the bound. Scanning all nodes does not.

A single insertion may repair one imbalance with one or two primitive rotations. AVL deletion may repair multiple ancestors. Rotation count alone does not measure all work: comparisons, height updates, pointer reads and allocation matter too.

## Predictions to test

1. Both structures should generally require more work as N increases, but cache boundaries can create irregular trends.
2. The heap may benefit from array locality. AVL may benefit when many cancellations require immediate removal. Neither advantage guarantees a winner.
3. Increasing cancellation changes both the operation mix and the heap's deferred cleanup. Throughput need not be monotonic in r.
4. Equal-mean distributions can change insertion positions and stale-root exposure. The growth order may remain the same while constants differ.
5. Heap physical occupancy should respect the configured post-operation bound. A violation indicates an implementation/policy problem, not merely measurement noise.
6. A crossover is an empirical outcome, not a required result. Report “no clear crossover in the tested range” when appropriate.

## How to use the data

Measure paired throughput with uncertainty, then inspect purge-work and rotation counters for a plausible explanation. Correlation alone is not causation: isolate a mechanism with additional controlled tests before claiming it caused a crossover. Do not fit many constants to a small dataset and call that proof. Keep raw rows, compiler/flags, configuration and code revision with the report.

Memory: require adapter-owned allocated-byte accounting including event storage and auxiliary indexes. Live element counts are not bytes. Whole-process peak RSS includes the trace, oracle and allocator retention; it cannot be labelled “heap memory” or “AVL memory” in this harness.


## Qualification for the uploaded heap now integrated

The supplied hash sets each have B=100,000 fixed buckets, with no resizing. A purge loops over both full bucket arrays in clear(), so a more accurate purge model for this implementation is O(B+H), plus collision-chain lookup work, not strictly O(H). Total purge work includes O(P*B+S), where P is purge count. At small N, scanning these buckets can dominate. At large N or adversarial colliding IDs, chains grow; expected constant lookup is not uniform over unbounded queue sizes.

The generic linear-purge amortized argument above is therefore conditional and cannot be cited unchanged as a uniform bound for this exact implementation. On the fixed-B implementation it has a potentially large B-dependent constant; when treating B as a design parameter, retain it explicitly. A follow-up optimized variant could clear only occupied buckets and resize hash tables, but that is not the version measured here.

The integrated AVL uses a new custom chained ID-to-event hash map with dynamic bucket growth, needed because the upload cancels by full key rather than ID. Include this map's work and backing storage in all comparisons. The measured implementations use different index designs; results compare complete implementations, not isolated abstract tree/heap algorithms.
