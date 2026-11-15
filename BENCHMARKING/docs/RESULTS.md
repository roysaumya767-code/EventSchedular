# Results from the integrated uploaded implementations

## Focused longer measurements

These are actual runs of the corrected, integrated uploaded heap and AVL, not the reference demo. N=1000, exponential increments with mean 1, 3,000,000 measured cycles, 10,000 warm-up cycles and five paired seeds per setting. Each timed cycle includes one removal and one insertion. Compiler: GCC 14.2.0 with -O2, in the shared Linux authoring environment.

| Cancellation fraction | Mean heap time (s) | Mean AVL time (s) | Mean paired heap speedup | Approx. 95% bootstrap interval |
|---|---:|---:|---:|---|
| 0% | 0.407 | 1.663 | 4.13x | 3.78–4.48x |
| 95% | 0.978 | 2.154 | 2.23x | 1.94–2.53x |

The heap was faster in both focused settings. Speedup is the mean of paired AVL-time/heap-time ratios, so it need not equal the ratio of the displayed mean times. Five trials provide limited uncertainty estimation; the percentile bootstrap interval is descriptive and does not account for shared-machine bias. A wider and repeated experiment is needed for a general recommendation or crossover claim.

All 20 timed trials lasted between 0.341 and 2.559 seconds. All 10 paired checksums matched, and every trial passed full event-output validation and sampled structural checks before timing.

## Full exploratory grid

The exploratory grid spans N=100,1000,10000; all 12 specified cancellation rates; four distributions; five paired seeds. It contains 720 pairs / 1,440 timed trials. Each has 5,000 measured cycles and 2,000 warm-up cycles. All pairs matched. All trials were shorter than 0.2 seconds, and some overlapped correctness tooling, so use them for coverage and preliminary patterns only. Do not report their exact speed ratios as established performance results.

The maximum sampled heap H/L was 1.999, consistent with the post-operation bound of 2. This is a sampled observation, supported by structural and purge-boundary tests, not a proof covering all possible executions.

## Interpreting memory and work

The fixed hash bucket arrays make heap accounted memory much larger at small N. The fixed arrays also impose clear-all-buckets work at every purge. These are implementation choices, not inherent requirements of a binary heap. AVL uses a newly added custom growing ID index and preserves the original string-bearing event layout. All of these costs are included in the comparison.

Diagnostic summaries subtract warm-up counter values for purge and rotation counts. Memory is accounted storage at sampled points; it excludes allocator overhead and temporary purge peaks. Separate diagnostic graphs show memory, purge count, rotations and height. Long-run occupancy plots show the first seed; summaries cover all five.

## What to say in the report

“We tested our corrected heap and AVL implementations using identical pre-generated traces. At N=1000 with exponential increments, the heap was faster at both 0% and 95% cancellation in five paired longer runs. The broader grid was exploratory because its timers were short. The result is specific to our implementations and includes their different ID-index and storage designs.”

For final submission, repeat on the team machine with at least ten paired seeds, longer warm-up checks, sufficient duration at every configuration, recorded compiler/flags and no concurrent workload. Do not infer an exact crossover from these two focused rates.
