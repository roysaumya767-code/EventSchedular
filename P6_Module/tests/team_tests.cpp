#include "../src/team_adapters.hpp"
#include <iostream>
#include <stdexcept>
using namespace p6;
void require(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
int main() try {
    for(auto b:team_backends()) {
        auto q=b.make();
        q->insert({1,0});q->insert({1,1});q->insert({2,2});
        require(q->cancel(2),"first cancel");require(!q->cancel(2),"repeated cancel");
        require(!q->cancel(999),"absent cancel");require(q->delete_min().id==0,"tie order first");
        require(q->delete_min().id==1,"tie order second");require(q->size()==0,"empty size");
        require(q->validate_structure(),"empty invariant");
        bool threw=false;try{q->delete_min();}catch(const std::runtime_error&){threw=true;}
        require(threw,"empty minimum must throw");
        q->insert({0,3});threw=false;try{q->insert({2,3});}catch(const std::runtime_error&){threw=true;}
        require(threw && q->size()==1,"duplicate ID changes size");
        auto other=b.make();other->insert({1,0});require(other->size()==1 && q->size()==1,"instance isolation");
        require(q->cancel(3) && q->validate_structure(),"cancel last live");
        if(b.name=="heap")require(q->metrics().physical==0,"heap stale entries at zero live");
    }
    HeapAdapter heap;
    for(uint64_t i=0;i<6;++i)heap.insert({double(i),i});
    for(uint64_t i=3;i<6;++i)require(heap.cancel(i),"heap setup cancel");
    require(heap.metrics().purges==0,"premature purge at equality");
    require(heap.delete_min().id==0,"heap minimum before post-delete purge");
    require(heap.metrics().purges==1 && heap.metrics().physical==2,"missing purge after live delete");
    require(heap.validate_structure(),"post-purge invariant");
    for(auto pattern:std::vector<std::vector<double>>{{3,2,1},{1,2,3},{3,1,2},{1,3,2}}) {
        AVLAdapter avl;for(size_t i=0;i<3;++i)avl.insert({pattern[i],i});
        long long expected=pattern==std::vector<double>{3,2,1} || pattern==std::vector<double>{1,2,3} ? 1:2;
        require(avl.metrics().rotations==expected,"rotation count");require(avl.validate_structure(),"rotation invariant");
        require(avl.cancel(1) && avl.validate_structure(),"index after deletion");
    }
    std::cout<<"PASS: both contracts, repeated/missing cancellation, duplicate IDs, empty queues, instance isolation, purge boundary, four rotation counters\n";
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
