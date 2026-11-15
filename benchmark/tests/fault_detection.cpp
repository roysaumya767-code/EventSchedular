#define main p6_benchmark_main
#include "../src/benchmark.cpp"
#undef main
class Broken : public p6::EventSet {
    p6::Reference q;
public:
    void insert(p6::Event e) override { q.insert(e); }
    p6::Event delete_min() override { auto e=q.delete_min();e.id+=1;return e; }
    bool cancel(uint64_t id) override { return q.cancel(id); }
    size_t size() const override { return q.size(); }
};
int main() {
    p6::Backend b{"broken",[]{return std::make_unique<Broken>();}};
    try { validate(b,generate(10,100,10,0,42,"uniform")); }
    catch(const std::runtime_error&) {std::cout<<"PASS: deliberately corrupted minimum rejected\n";return 0;}
    std::cerr<<"FAIL: corruption was accepted\n";return 1;
}
