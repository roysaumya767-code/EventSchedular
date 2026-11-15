#include "reference.hpp"
#include "team_adapters.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
using namespace p6;
struct Step { bool cancel; Event removed, added; };
struct Trace { std::vector<Event> initial; std::vector<Step> steps; size_t cancellations=0; };
class Random {
    std::mt19937_64 rng;
public:
    explicit Random(uint64_t seed):rng(seed) {}
    double unit() { return double(rng() >> 11)/9007199254740992.0; }
    size_t index(size_t n) {
        uint64_t bound=n, threshold=(-bound)%bound, x;
        do { x=rng(); } while(x<threshold);
        return x%bound;
    }
    double increment(const std::string& d) {
        double u=unit();
        if(d=="exponential") return -std::log1p(-u);
        if(d=="uniform") return 2*u;
        if(d=="triangular") return u<0.5 ? std::sqrt(2*u) : 2-std::sqrt(2*(1-u));
        if(d=="bimodal") return (u<0.5 ? 0.1 : 1.7)+0.2*unit();
        throw std::runtime_error("unknown distribution");
    }
};
Trace generate(size_t n,size_t count,size_t warm,double rate,uint64_t seed,const std::string& d) {
    Random rng(seed); Trace t; Reference oracle;
    std::vector<Event> live; std::map<uint64_t,size_t> positions;
    uint64_t next=0; double now=0;
    auto add=[&](Event e) { oracle.insert(e); positions[e.id]=live.size(); live.push_back(e); };
    for(size_t i=0;i<n;++i) { Event e{rng.increment(d),next++}; t.initial.push_back(e); add(e); }
    for(size_t i=0;i<warm+count;++i) {
        bool cancel=rng.unit()<rate;
        Event removed;
        if(cancel) { removed=live[rng.index(live.size())]; oracle.cancel(removed.id); }
        else { removed=oracle.delete_min(); now=removed.time; }
        size_t p=positions.at(removed.id); live[p]=live.back(); positions[live[p].id]=p;
        live.pop_back(); positions.erase(removed.id);
        Event added{now+rng.increment(d),next++};
        add(added); t.steps.push_back({cancel,removed,added});
        if(i>=warm && cancel) ++t.cancellations;
    }
    return t;
}
uint64_t apply(EventSet& q,const Step& s,bool check) {
    uint64_t value;
    if(s.cancel) {
        bool ok=q.cancel(s.removed.id);
        if(check && !ok) throw std::runtime_error("valid cancellation failed");
        value=ok ? s.removed.id : UINT64_MAX;
    } else {
        Event got=q.delete_min();
        if(check && !(got==s.removed)) throw std::runtime_error("minimum differs from oracle");
        value=got.id;
    }
    q.insert(s.added); return value;
}
void validate(const Backend& b,const Trace& t) {
    auto q=b.make(); for(auto e:t.initial) q->insert(e);
    size_t checked_steps=0;
    for(const auto& s:t.steps) {
        apply(*q,s,true);
        if((t.initial.size()<=100 || ++checked_steps%1024==0) && !q->validate_structure())
            throw std::runtime_error("structural invariant failed");
        if(q->size()!=t.initial.size()) throw std::runtime_error("live-size mismatch");
    }
    if(!q->validate_structure()) throw std::runtime_error("final structural invariant failed");
    Reference expected; for(auto e:t.initial) expected.insert(e);
    for(auto s:t.steps) apply(expected,s,true);
    while(expected.size()) if(!(expected.delete_min()==q->delete_min())) throw std::runtime_error("final drain differs");
    if(q->size()) throw std::runtime_error("queue not empty after drain");
}
void self_test() {
    Backend ref{"reference_demo",[]{return std::make_unique<Reference>();}};
    for(auto d:{"exponential","uniform","bimodal","triangular"}) {
        Random r(42); double sum=0;
        for(int i=0;i<100000;++i) {double x=r.increment(d); if(!std::isfinite(x)||x<0) throw std::runtime_error("bad increment"); sum+=x;}
        if(std::abs(sum/100000-1)>0.02) throw std::runtime_error("distribution mean test");
        for(double rate:{0.0,0.5,0.95,1.0}) for(size_t n:{1u,10u,100u}) {
            Trace t=generate(n,1000,100,rate,42,d); validate(ref,t);
            for(const auto& backend:team_backends()) validate(backend,t);
            Trace u=generate(n,1000,100,rate,42,d);
            for(size_t i=0;i<t.steps.size();++i)
                if(!(t.steps[i].added==u.steps[i].added) || !(t.steps[i].removed==u.steps[i].removed) || t.steps[i].cancel!=u.steps[i].cancel)
                    throw std::runtime_error("nonrepeatable trace");
        }
    }
    Reference q; q.insert({1,2});q.insert({1,1});
    if(q.delete_min().id!=1 || !q.cancel(2) || q.cancel(2) || q.cancel(99) || q.size()) throw std::runtime_error("contract test");
    std::cout<<"PASS: distributions, 48 traces across reference/heap/AVL, reproducibility, tie ordering and cancellation contract\n";
}
int main(int argc,char** argv) try {
    if(argc==2 && std::string(argv[1])=="--self-test") { self_test(); return 0; }
    if(argc!=10) {
        std::cerr<<"Usage: p6_bench demo|team N steps warmup rate distribution seed repeats output_prefix\n"; return 2;
    }
    std::string mode=argv[1],dist=argv[6],prefix=argv[9];
    auto positive=[](const char* s) { std::string v=s; if(v.empty()||v.find_first_not_of("0123456789")!=std::string::npos) throw std::runtime_error("integer expected"); return std::stoull(v); };
    size_t n=positive(argv[2]),count=positive(argv[3]),warm=positive(argv[4]);
    size_t pos=0; double rate=std::stod(argv[5],&pos); uint64_t seed=positive(argv[7]); size_t reps=positive(argv[8]);
    if(pos!=std::string(argv[5]).size() || !n||!count||!reps||!std::isfinite(rate)||rate<0||rate>1 || warm>SIZE_MAX-count) throw std::runtime_error("invalid configuration");
    std::vector<Backend> backends;
    if(mode=="demo") backends.push_back({"reference_demo",[]{return std::make_unique<Reference>();}});
    else if(mode=="team") backends=team_backends();
    else throw std::runtime_error("mode must be demo or team");
    if(backends.empty()) throw std::runtime_error("Team adapters missing: connect P1/P2 in src/team_adapters.hpp");
    std::ofstream raw(prefix+"_raw.csv"),diag(prefix+"_diagnostics.csv");
    if(!raw||!diag) throw std::runtime_error("cannot open output; create parent directory first");
    raw<<"backend,n,steps,warmup,rate,distribution,seed,repeat,actual_rate,seconds,operations_per_second,checksum\n";
    diag<<"backend,n,rate,distribution,seed,step,live,physical,bytes,purges,purged_entries,rotations,height\n";
    raw<<std::setprecision(17); diag<<std::setprecision(17);
    for(size_t rep=0;rep<reps;++rep) {
        Trace t=generate(n,count,warm,rate,seed+rep,dist);
        // Rotate order across paired trials to reduce systematic order bias.
        for(size_t k=0;k<backends.size();++k) {
            const auto& b=backends[(k+rep)%backends.size()]; validate(b,t);
            auto q=b.make(); for(auto e:t.initial) q->insert(e);
            for(size_t i=0;i<warm;++i) apply(*q,t.steps[i],false);
            uint64_t checksum=0;
            auto start=std::chrono::steady_clock::now();
            for(size_t i=warm;i<t.steps.size();++i) checksum=checksum*1099511628211ULL+apply(*q,t.steps[i],false);
            double secs=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
            raw<<b.name<<','<<n<<','<<count<<','<<warm<<','<<rate<<','<<dist<<','<<seed+rep<<','<<rep<<','<<double(t.cancellations)/count<<','<<secs<<','<<2.0*count/secs<<','<<checksum<<'\n';
            q.reset(); q=b.make(); for(auto e:t.initial) q->insert(e);
            // Diagnostics are a separate replay, never part of the reported timer.
            for(size_t i=0;i<t.steps.size();++i) {
                apply(*q,t.steps[i],false);
                if(i+1>=warm && ((i+1-warm)%std::max<size_t>(1,count/100)==0 || i+1==t.steps.size())) {
                    auto m=q->metrics(); diag<<b.name<<','<<n<<','<<rate<<','<<dist<<','<<seed+rep<<','<<i+1-warm<<','<<q->size()<<','<<m.physical<<','<<m.bytes<<','<<m.purges<<','<<m.purged_entries<<','<<m.rotations<<','<<m.height<<'\n';
                }
            }
        }
    }
    if(!raw||!diag) throw std::runtime_error("output write failed");
    return 0;
} catch(const std::exception& e) { std::cerr<<"Error: "<<e.what()<<'\n'; return 1; }
