# Integration of the uploaded heap and AVL

The supplied files are preserved in team_originals/. Production headers are derived from those files; this is not a substitution with library priority queues or a different AVL algorithm.

## Changes to avl.cpp

1. Added the missing `leftRotations++` inside rotL. Without this, RR and parts of double rotations were invisible in the measurements.
2. Moved functions and rotation counters into an instance-owned Core object inside team_avl. Each benchmark trial has independent counters.
3. Removed the demonstration main from the production header. The original main repeatedly sets root=nullptr without clearing the prior tree, leaking those demonstration trees. The adapter owns the root and calls clearTree on destruction.
4. Changed time from long long to double to preserve fractional increments produced by the benchmark. Ordering still uses (time,sequence). The harness supplies its unique ID as sequence.
5. Added a custom dynamically growing chained ID-to-event hash map in the adapter. The uploaded cancel takes a complete Event key, while the benchmark cancels by ID. The index finds that key without a linear tree scan. It stores keys, not node pointers, so copying successor data during two-child deletion does not invalidate index handles.
6. Exposed height, primitive rotation count and accounted storage. A double rotation counts twice. Existing BST/height/balance validation is reused and the index is checked against tree nodes.
7. Retained successor's existing `time >= t` semantics. This is a timestamp lower-bound, not a strict successor. The benchmark does not call it; decide with P3/P4 whether the assignment requires `>` instead.

The original AVL example inserts the same ID and sequence with two different timestamps. The BST allows these as different keys, but an event scheduler needs unique active IDs. The adapter rejects duplicate IDs before changing the tree.

## Changes to minheap.cpp

1. Changed timestamp to double for the same reason as AVL.
2. Changed the production next_live_event return value from optional ID to optional full event. This allows exact timestamp/order validation without adding another ID lookup just to recover the timestamp.
3. Repeated cancellation now returns false if an ID is already in the cancelled set. The original returned true until that event was physically removed.
4. Added a purge check before returning a live minimum. Example: H=6,C=3,L=3 satisfies C<=L. Removing a live minimum makes H=5,C=3,L=2, so a purge is now required. The original only checked during cancellation.
5. Added live-size, physical-size, purge count, total purge input entries, accounted memory and structural-check methods. Count/lookup queries are const-correct. The heap's get_events returns a const reference to let validation inspect without copying; purge still takes its own snapshot before rebuilding.
6. Replaced nonstandard bits/stdc++.h with explicit standard headers and put the implementation in team_heap to avoid naming conflicts. Removed production main. Reject negative IDs at scheduler entry points; the adapter rejects IDs outside signed 64-bit range and invalid times.

Retained: custom binary heap, sift-up/down, bottom-up heap rebuild, custom chained hash sets, fixed 100,000 buckets per set, and retained vector capacities. These details affect results. Two bucket arrays alone account for roughly 4.8 MB with 24-byte vectors in this environment, even for tiny queues. clear() visits every bucket on every purge, and clear() retains individual bucket capacities.

## What the measurements mean

Memory counts object/node storage and vector backing capacities. It excludes allocator metadata, allocator size rounding and temporary purge peak allocations. AVL type strings are empty in this workload; the measured library stores them inline. No process-RSS or exact peak-memory claim is made. bytes are measured outside the timer.

The benchmark's complete AVL event includes the original type string; the original heap event does not. This difference in payload/node layout is preserved and can affect memory/cache results. Both receive the same timestamp, sequence and ID.

Metrics count physical rotations including those during deletion. Purge work counts physical entries scanned, separately from fixed bucket-clear overhead. There is no heap max-occupancy counter: sampled peaks may miss transient maxima. The occupancy bound is additionally checked during validation and in targeted regression tests.

Do not cite these results as an intrinsic proof that all heaps or all AVL trees are faster. This experiment compares these corrected implementations and their indexes on the supplied workload.
