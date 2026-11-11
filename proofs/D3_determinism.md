# D3 — Deterministic simulation and correct processing order

**Owner: P3 (Simulation Architect)**

This file is written for a beginner. Every technical word is explained the first time it appears.

---

## 0. The words you need

| Word | Meaning in easy English |
|---|---|
| **Event** | One thing that will happen at one moment, e.g. "packet 7 arrives at time 120". |
| **Timestamp** `t(e)` | The time at which event `e` happens. |
| **Sequence number** `s(e)` | A number stamped on each event when it is *created*: first event gets 0, next gets 1, next gets 2, and so on. |
| **Priority queue / event set** | A box where we put events. We can always ask it: "give me the event with the smallest time". |
| **Deterministic** | Same input → same output, every single time, on every computer. |
| **Tie** | Two events with the same timestamp. |
| **Total order** | A rule that can compare ANY two different things and always says clearly which one is first. |
| **Invariant** | A statement that stays true all the time while the program runs. |

## 1. The problem: ties

Suppose two events are both at time 50:

* `A` = "ACK arrives for packet 3"
* `B` = "timer fires for packet 3"

If the order is not fixed, a heap might give `A` first and an AVL tree might give `B` first.
If `A` goes first, the timer is cancelled and nothing else happens.
If `B` goes first, the packet is sent again (new events, new random numbers…).
From that moment the two simulations are **different worlds**. So we cannot compare heap and AVL fairly.

**Fix:** compare `(time, sequence number)`:

> `e < f`  ⇔  `t(e) < t(f)`, **or** `t(e) = t(f)` and `s(e) < s(f)`.

In the code: `operator<` in `include/event.hpp`, and the counter `nextSeq++` in `Simulator::schedule` (the only place where events are created).

## 2. Lemma 1 — every event has a different sequence number

**Claim.** If `e ≠ f` are two events, then `s(e) ≠ s(f)`.

**Proof.** The counter starts at 0. Creating an event does "give the event the counter value, then add 1 to the counter". So the 1st event created gets 0, the 2nd gets 1, …, the k-th gets k−1. The counter only goes up, so no value is ever given twice. ∎

Also: **an event created later always has a bigger sequence number** than an event created earlier. (We use this in Theorem 2.)

## 3. Lemma 2 — `(time, seq)` is a total order, so the smallest event is unique

**Claim.** For any two different events `e, f` exactly one of `e < f` or `f < e` is true. The relation is also transitive (`e<f` and `f<g` ⇒ `e<g`).

**Proof.**
* *Comparable:* if `t(e) ≠ t(f)`, the smaller time decides. If `t(e) = t(f)`, then by Lemma 1 `s(e) ≠ s(f)`, so the smaller sequence number decides. Only one of the two cases can happen at once, so exactly one of `e<f`, `f<e` holds.
* *Transitive:* this is the ordinary "dictionary order" on pairs of numbers. If `e<f` and `f<g`: look at the times. If `t(e) < t(g)` we are done. Times can never go backwards along the chain (`t(e) ≤ t(f) ≤ t(g)`), so the only other possibility is `t(e) = t(f) = t(g)`, and then `s(e) < s(f) < s(g)`, so `e<g`. ∎

**Consequence.** Every non-empty set of events has **exactly one** smallest event. A correct "delete-min" has no choice at all: heap, AVL tree and sorted array must all return that same event.

## 4. Theorem 1 — the simulation is deterministic

**Claim.** For a fixed configuration (including the random seed), the sequence of processed events is the same in every run, and the same for every correct event-set structure (heap, AVL, sorted array).

**Proof (by induction on the step number k).** The *state* of the simulation is: the set of live events, the counter, the state of our random generator, and the per-packet records.

* *Start:* the state before step 1 is fixed by the configuration.
* *Step:* assume the state before step k is the same in two runs (A and B). 
  1. The event removed is the smallest live event. By Lemma 2 it is unique, so both runs remove the same event `e_k`. 
  2. The handler for `e_k` uses only (a) `e_k`, (b) the state, (c) integer arithmetic, (d) our own random generator `splitmix64` (`include/rng.hpp`), which is a fixed formula. So it creates the same new events, with the same times, in the same order. The counter therefore gives them the same sequence numbers. It also cancels the same events. 
  3. So the state after step k is the same in both runs.

By induction all steps are identical. ∎

(We wrote our own random generator because `std::uniform_int_distribution` may give different numbers on different compilers; that would break step 2.)

## 5. Theorem 2 — no event is handled before an event with a smaller timestamp

**Assumption (causality).** A handler only creates events whose time is **not earlier than the current time**: `t(new) ≥ t(current)`. (In our simulator every delay is at least 1 tick, so this holds.)

**Claim.** If `e_1, e_2, e_3, …` are the events in the order they are processed, then `e_k < e_{k+1}` for every `k`. In particular `t(e_k) ≤ t(e_{k+1})`, so times never go backwards.

**Proof.** Look at the moment right after `e_k` was handled. The event `e_{k+1}` is the smallest live event at that moment. It is one of two kinds:

1. *It was already in the set when `e_k` was removed.* `e_k` was the smallest of all live events then, and `e_{k+1} ≠ e_k`, so `e_k < e_{k+1}`.
2. *It was created while handling `e_k`.* By the assumption `t(e_{k+1}) ≥ t(e_k)`. It was created later than `e_k`, so `s(e_{k+1}) > s(e_k)` (Lemma 1). If the time is bigger, `e_k < e_{k+1}` by time. If the times are equal, `e_k < e_{k+1}` by sequence number.

In both cases `e_k < e_{k+1}`. Chaining the inequalities gives `t(e_j) ≤ t(e_k)` for all `j < k`: nothing is ever handled before something with a smaller time. ∎

**Cancellation does not break this:** a cancelled event simply never appears in the list `e_1, e_2, …`. The proof only talks about live events.
(P1's lazy-deletion heap must therefore skip cancelled entries when it returns the minimum.)

## 6. How the code checks the proof while running

* `Simulator::run` tests, for **every** processed event, that `(time, seq)` is strictly bigger than the previous one. If not it prints `ORDER VIOLATION` and stops. This is Theorem 2 checked at runtime.
* It also builds a 64-bit **fingerprint** (`traceHash`) of the whole processed order. Equal fingerprints ⇒ equal order (Theorem 1 in practice).

Results (1,000,000 events, seed 12345):

```
processed=337910 scheduled=1000036 cancelled=662126 retransmissions=31314 maxLive=248 lastTime=284441
cancelled%=66.2
traceHash=15d0491cecb33bd5
```
Sorted vector, `std::set`, **P1's heap and P2's AVL tree** all give **exactly the same output** (`diff` shows no difference). A random test with 600,000 operations (many ties, many cancels) also showed no difference between the structures, and P2's AVL checker passed after every operation.

## 7. Worked example (do it by hand)

Events created in this order:

| created # | name | time |
|---|---|---|
| 0 | A | 30 |
| 1 | B | 10 |
| 2 | C | 30 |
| 3 | D | 10 |

Pairs `(time, seq)`: A(30,0) B(10,1) C(30,2) D(10,3).
Sorted: B(10,1) < D(10,3) < A(30,0) < C(30,2). Processing order: **B, D, A, C**.
Check: times 10,10,30,30 never go backwards; the ties (B,D) and (A,C) are broken by creation order.

## 8. Practice problems (with solutions)

**Q1.** Sort these `(time, seq)` pairs: (7,4) (5,9) (7,1) (5,2) (9,0).
**A1.** (5,2) (5,9) (7,1) (7,4) (9,0). First by time; ties by seq.

**Q2.** Is it possible that two different events have the same `(time, seq)`? Why?
**A2.** No. Sequence numbers are all different (Lemma 1), so the pairs are all different.

**Q3.** The counter is reset to 0 after each packet. What breaks?
**A3.** Two events from different packets could get the same `seq`. With equal times they would tie completely, Lemma 1 and Lemma 2 fail, and the heap and AVL could return them in different orders → results differ.

**Q4.** A handler creates a new event with time = now − 5. Which theorem fails and why?
**A4.** Theorem 2. The new event is smaller than the one just handled, so it would be handled *after* an event with a bigger time. The causality assumption is violated.

**Q5.** A handler creates a new event at the *same* time as the current event. Is it processed after the current one?
**A5.** Yes. Same time, but its seq is bigger (it is created later), so it is bigger in `(time, seq)`.

**Q6.** Events are created in this order: X(time 4), Y(time 4), Z(time 3). In which order are they processed?
**A6.** Z (3,2), then X (4,0), then Y (4,1).

**Q7 (think).** Why can we not simply say "ties can go in any order, it does not matter"?
**A7.** Because the order changes what the handlers do (e.g. ACK before timer cancels the timer; timer before ACK causes a retransmission). Different order ⇒ different future ⇒ heap and AVL runs cannot be compared.
