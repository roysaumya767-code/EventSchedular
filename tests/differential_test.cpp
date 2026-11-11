// differential_test.cpp -- P3: random operations on ALL structures, compared step by step.
// g++ -std=c++17 -O2 -Iinclude tests/differential_test.cpp src/simulator.cpp -o diff_test
#include <cstdio>
#include <vector>
#include <memory>
#include "simulator.hpp"
#include "simple_sets.hpp"
#include "heap.hpp"
#include "avl.hpp"

int main() {
    Rng rng(2026);
    int bad = 0; long long ops = 0;
    for (int round = 0; round < 200; round++) {
        SortedVectorSet ref; StdSetEventSet st; HeapEventSet hp; AvlEventSet av;
        EventSet* all[3] = { &st, &hp, &av };
        std::vector<Event> live; std::uint64_t seq = 0;
        long long spread = (round % 4 == 0) ? 3 : (round % 4 == 1) ? 50 : 1000;   // small spread = MANY ties
        for (int step = 0; step < 3000; step++) {
            int r = (int)rng.range(0, 99); ops++;
            if (r < 45 || live.empty()) {                       // insert
                Event e; e.time = rng.range(0, spread); e.seq = seq++; e.type = (int)rng.range(0, 2); e.packet = (int)rng.range(0, 9);
                ref.insert(e); for (EventSet* s : all) s->insert(e); live.push_back(e);
            } else if (r < 75) {                                // cancel a random live event
                size_t i = (size_t)rng.range(0, (long long)live.size() - 1);
                Event e = live[i]; live.erase(live.begin() + i);
                bool a = ref.cancel(e);
                for (EventSet* s : all) if (s->cancel(e) != a) { std::printf("cancel mismatch %s\n", s->name()); bad++; }
                // cancelling it a second time must fail everywhere
                bool a2 = ref.cancel(e);
                for (EventSet* s : all) if (s->cancel(e) != a2) { std::printf("2nd cancel mismatch %s\n", s->name()); bad++; }
            } else {                                            // deleteMin
                Event x{}, y{}; bool okr = ref.deleteMin(x);
                for (EventSet* s : all) {
                    bool ok = s->deleteMin(y);
                    if (ok != okr || (ok && (x.time != y.time || x.seq != y.seq || x.type != y.type || x.packet != y.packet))) {
                        std::printf("deleteMin mismatch %s\n", s->name()); bad++; }
                }
                for (size_t i = 0; i < live.size(); i++) if (live[i].seq == x.seq) { live.erase(live.begin() + i); break; }
            }
            for (EventSet* s : all) if (s->size() != ref.size()) { std::printf("size mismatch %s\n", s->name()); bad++; }
            if (!av.validate()) { std::printf("AVL INVALID at round %d step %d\n", round, step); bad++; }
            if (bad > 10) return 1;
        }
    }
    std::printf("%lld random operations, mismatches: %d -> %s\n", ops, bad, bad ? "FAILED" : "ALL AGREE");
    return bad ? 1 : 0;
}
