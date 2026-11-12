#include <bits/stdc++.h>
#define main heap_main_unused
#include "../minheap.cpp"
#undef main
#define main avl_main_unused
#include "../avl.cpp"
#undef main
using namespace std;

int totalChecks = 0, failed = 0;

void check(bool ok, string msg) {
    totalChecks++;
    if (!ok) {
        failed++;
        if (failed <= 20) cout << "FAILED: " << msg << endl;
    }
}


struct OEvent { long long time, seq, id; };

class Oracle {
    vector<OEvent> arr;

    
    int lowerBound(long long t, long long s) {
        int lo = 0, hi = arr.size();
        while (lo < hi) {
            int mid = (lo + hi) / 2;
            if (arr[mid].time < t || (arr[mid].time == t && arr[mid].seq < s)) lo = mid + 1;
            else hi = mid;
        }
        return lo;
    }

public:
    int size() { return arr.size(); }
    OEvent get(int i) { return arr[i]; }

    void insert(OEvent e) { arr.insert(arr.begin() + lowerBound(e.time, e.seq), e); }

    bool popMin(OEvent &out) {
        if (arr.empty()) return false;
        out = arr[0];
        arr.erase(arr.begin());
        return true;
    }

    bool cancel(long long t, long long s) {    
        int pos = lowerBound(t, s);
        if (pos < (int)arr.size() && arr[pos].time == t && arr[pos].seq == s) {
            arr.erase(arr.begin() + pos);
            return true;
        }
        return false;
    }

    bool successor(long long t, OEvent &out){
        int pos = lowerBound(t, LLONG_MIN);
        if (pos == (int)arr.size()) return false;
        out = arr[pos];
        return true;
    }
};


const int ERR = -100;   
string errorMsg;

bool lessThan(Event a, Event b) {
    if (a.time != b.time) return a.time < b.time;
    return a.sequence < b.sequence;
}


int checkNode(Node *x, Event *low, Event *high) {
    if (x == NULL) return -1;
    if ((low && !lessThan(*low, x->event)) || (high && !lessThan(x->event, *high))) {
        errorMsg = "BST order broken";
        return ERR;
    }
    int leftH = checkNode(x->left, low, &x->event);
    if (leftH == ERR) return ERR;
    int rightH = checkNode(x->right, &x->event, high);
    if (rightH == ERR) return ERR;

    int realH = 1 + max(leftH, rightH);
    if (x->h != realH) { errorMsg = "stored height is wrong"; return ERR; }
    if (abs(leftH - rightH) > 1) { errorMsg = "not balanced"; return ERR; }
    return realH;
}

bool myCheck(Node *root) {
    errorMsg = "";
    return checkNode(root, NULL, NULL) != ERR;
}
-
Event makeEvent(long long time, long long seq, long long id) {
    Event e;
    e.time = time; e.sequence = seq; e.id = id; e.type = "X";
    return e;
}

int countNodes(Node *x) {
    return x == NULL ? 0 : 1 + countNodes(x->left) + countNodes(x->right);
}

void collectNodes(Node *x, vector<Node *> &v) {
    if (x == NULL) return;
    v.push_back(x);
    collectNodes(x->left, v);
    collectNodes(x->right, v);
}

int realHeight(Node *x) {
    return x == NULL ? -1 : 1 + max(realHeight(x->left), realHeight(x->right));
}

bool allBalanced(Node *x) {
    if (x == NULL) return true;
    if (abs(realHeight(x->left) - realHeight(x->right)) > 1) return false;
    return allBalanced(x->left) && allBalanced(x->right);
}

Node *plainInsert(Node *x, Event k) {
    if (x == NULL) return new Node{k, 0, NULL, NULL};
    if (lessThan(k, x->event)) x->left = plainInsert(x->left, k);
    else x->right = plainInsert(x->right, k);
    upd(x);
    return x;
}

void testMinHeap() {
    srand(1);
    for (int round = 0; round < 100; round++) {
        minheap h;
        vector<event> all;
        int n = rand() % 200;
        for (int i = 0; i < n; i++) {
            event e = {rand() % 20, i, i};    
            h.insert(e);
            all.push_back(e);
        }
        vector<event> v = h.get_events();     
        for (int k = 1; k < (int)v.size(); k++)
            check(!earlier(v[k], v[(k - 1) / 2]), "heap property");
        sort(all.begin(), all.end(), earlier);
        for (int i = 0; i < n; i++)
            check(h.deleteMin().event_id == all[i].event_id, "deleteMin order");
    }
    cout << "Test 1 (min heap) done" << endl;
}


void testAllOrders() {
    int trees = 0, disagree = 0;
    for (int n = 0; n <= 7; n++) {
        vector<int> order(n);
        for (int i = 0; i < n; i++) order[i] = i;
        do {
            Node *plain = NULL, *avl = NULL;
            for (int i = 0; i < n; i++) {
                Event e = makeEvent(order[i], 0, order[i]);
                plain = plainInsert(plain, e);
                avl = insert(avl, e);
            }
            check(myCheck(plain) == allBalanced(plain), "checker wrong on plain BST");
            if (validateAvl(plain) != myCheck(plain)) disagree++;
            check(myCheck(avl) && countNodes(avl) == n, "AVL insert made a bad tree");
            trees++;
            clearTree(plain);
            clearTree(avl);
        } while (next_permutation(order.begin(), order.end()));
    }
    cout << "Test 2 (all insertion orders): " << trees << " trees, validateAvl disagreed "
         << disagree << " times" << endl;
    check(disagree == 0, "P2 validateAvl disagrees with my checker");
}

void testBrokenTrees() {
    srand(5);
    int caught = 0, total = 0, p2missed = 0;
    for (int round = 0; round < 200; round++) {
        Node *root = NULL;
        int n = 5 + rand() % 100;
        for (int i = 0; i < n; i++) root = insert(root, makeEvent(rand() % 1000, i, i));
        check(myCheck(root), "starting tree should be valid");
        vector<Node *> nodes;
        collectNodes(root, nodes);


        Node *a = nodes[rand() % nodes.size()], *b = nodes[rand() % nodes.size()];
        if (a->event.time != b->event.time) {
            swap(a->event, b->event);
            total++;
            if (!myCheck(root)) caught++;
            if (validateAvl(root)) p2missed++;
            swap(a->event, b->event);
        }

        Node *c = nodes[rand() % nodes.size()];
        c->h++;
        total++;
        if (!myCheck(root)) caught++;
        if (validateAvl(root)) p2missed++;
        c->h--;

        check(myCheck(root), "tree should be valid again after repair");
        clearTree(root);

        Node *chain = NULL;
        for (int i = 0; i < 3 + rand() % 20; i++) chain = plainInsert(chain, makeEvent(i, i, i));
        total++;
        if (!myCheck(chain) && errorMsg == "not balanced") caught++;
        if (validateAvl(chain)) p2missed++;
        clearTree(chain);
    }
    cout << "Test 3 (broken trees): caught " << caught << " of " << total
         << ", P2 validateAvl missed " << p2missed << endl;
    check(caught == total, "some broken tree was not caught");
    check(p2missed == 0, "P2 validateAvl accepted a broken tree");
}


void randomTest(int seed, int steps, int tmax, bool checkEveryStep) {
    srand(seed);
    scheduler sch;
    Node *root = NULL;
    Oracle oracle;
    vector<Event> live, fired;
    long long idCounter = 0, now = 0, lastTime = -1;

    for (int step = 0; step < steps; step++) {
        int r = rand() % 100;

        if (r < 45) {                                   
            idCounter++;
            long long t = now + rand() % tmax;
            Event e = makeEvent(t, idCounter, idCounter);
            check(sch.add_event({t, idCounter, idCounter}), "heap add_event failed");
            root = insert(root, e);
            oracle.insert({t, idCounter, idCounter});
            live.push_back(e);
        }
        else if (r < 70) {                              
            if (!live.empty() && rand() % 4 != 0) {
                int i = rand() % live.size();
                Event e = live[i];
                live.erase(live.begin() + i);
                check(sch.cancel_event(e.id), "heap cancel failed");
                root = cancel(root, e);
                check(oracle.cancel(e.time, e.sequence), "oracle cancel failed");
            }
            else if (!fired.empty()) {                  
                Event e = fired[rand() % fired.size()];
                int before = countNodes(root);
                check(!sch.cancel_event(e.id), "heap should return false for old event");
                root = cancel(root, e);
                check(countNodes(root) == before, "AVL changed on missing cancel");
            }
        }
        else if (r < 90) {                              
            optional<long long> fromHeap = sch.next_live_event();
            Event fromAvl;
            bool avlOk = deleteMin(root, fromAvl);
            OEvent fromOracle;
            bool oracleOk = oracle.popMin(fromOracle);
            check(fromHeap.has_value() == oracleOk && avlOk == oracleOk, "emptiness differs");
            if (oracleOk) {
                check(fromHeap.value() == fromOracle.id, "heap removed a different event");
                check(fromAvl.id == fromOracle.id, "AVL removed a different event");
                check(fromOracle.time >= lastTime, "time went backwards");
                lastTime = now = fromOracle.time;
                for (int i = 0; i < (int)live.size(); i++)
                    if (live[i].id == fromOracle.id) { live.erase(live.begin() + i); break; }
                fired.push_back(fromAvl);
            }
        }
        else {                                          
            long long t = now + rand() % (tmax + 2) - 1;
            Node *s = successor(root, t);
            OEvent o;
            bool has = oracle.successor(t, o);
            check((s != NULL) == has, "successor exists or not");
            if (s != NULL && has) check(s->event.id == o.id, "successor is a different event");
        }

        if (checkEveryStep) {
            check(validateAvl(root), "P2 validateAvl failed");
            check(myCheck(root), "my checker failed: " + errorMsg);
            check(countNodes(root) == oracle.size(), "AVL size != oracle size");
        }
    }

    
    while (true) {
        optional<long long> h = sch.next_live_event();
        Event a;
        bool avlOk = deleteMin(root, a);
        OEvent o;
        bool oracleOk = oracle.popMin(o);
        check(h.has_value() == oracleOk && avlOk == oracleOk, "final emptying: emptiness differs");
        if (!oracleOk) break;
        check(h.value() == o.id && a.id == o.id, "final emptying: order differs");
    }
    cout << "Test 4 (random) seed=" << seed << " tmax=" << tmax << " inserts=" << idCounter << endl;
}

int main() {
    testMinHeap();
    testAllOrders();
    testBrokenTrees();
    randomTest(1, 30000, 1, true);       
    randomTest(2, 30000, 3, true);
    randomTest(3, 30000, 20, true);
    randomTest(4, 200000, 1000, false);  

    cout << endl << "checks: " << totalChecks << "   failed: " << failed << endl;
    cout << (failed == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED") << endl;
    return failed == 0 ? 0 : 1;
}
