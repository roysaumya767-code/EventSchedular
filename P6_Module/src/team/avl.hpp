#pragma once
#include <algorithm>
#include <cstdlib>
#include <string>
namespace team_avl {
using std::string; using std::max; using std::abs;
struct Event {
    double time;
    long long sequence;
    long long id;
    string type;
};


struct Node { 
    Event event;
    int h ; 
    Node * left , * right ; };




class Core {
public:
long long leftRotations=0, rightRotations=0;
int ht ( Node * x ) { return x ? x ->h : -1; }

void upd ( Node * x) { x ->h = 1 + max ( ht (x -> left ) ,
ht (x -> right )); }


Node * rotR ( Node * y) {
Node * x = y -> left ;
y -> left = x -> right ; 
x -> right = y;
upd (y ); upd ( x ); 
rightRotations++;
return x; 
}


Node * rotL ( Node * x) { 
Node * y = x -> right ;
x -> right = y -> left ;
y -> left = x;
upd (x ); upd ( y );
leftRotations++;
return y;
}



int bf ( Node * x) { return ht (x -> left ) - ht (x -> right ); }


Node * rebalance ( Node * z) {
upd (z );
if ( bf (z) > 1) { // left too tall
if ( bf (z -> left ) < 0) // LR: make it LL
z -> left = rotL (z -> left );
return rotR (z ); // LL
}
if ( bf (z) < -1) { // right too tall
if ( bf (z -> right ) > 0) // RL: make it RR
z -> right = rotR (z -> right );
return rotL (z ); // RR
}
return z; // already fine
}

bool comparator(Event a, Event b) {
    if(a.time != b.time) return a.time<b.time;
    return a.sequence < b.sequence;

}




Node * insert ( Node * x , Event k) {
if (! x) return new Node {k , 0, nullptr , nullptr };
if (comparator(k,x->event) ) x -> left = insert (x -> left , k );
else if (comparator(x->event,k)) x -> right = insert (x -> right , k );
else return x ; // no duplicates
return rebalance (x ); // repair on the way up
}
Node* erase(Node* x, Event k) {
if (x == nullptr) return nullptr; // not found
if (comparator(k,x->event)) x->left = erase(x->left, k);
else if (comparator(x->event,k)) x->right = erase(x->right, k);
else {
if (x->left == nullptr) { // cases 1, 2
Node* r = x->right; delete x; return r;
}
if (x->right == nullptr) { // case 2
Node* l = x->left; delete x; return l;
}
Node* s = x->right; // case 3:
while (s->left) s = s->left; // successor
x->event = s->event; // copy it up,
x->right = erase(x->right, s->event); // delete below
}
return rebalance(x);
}


Node * getmin(Node *x) {
    if (! x) return x;
    if(x->left) {
         return getmin(x->left);
    }
    else {
    return x;
    }
}


bool deleteMin(Node *&x, Event& result) {
      if (x == nullptr)
        return false;
    Node *y = getmin(x);
    result = y->event;
     x = erase(x,y->event);
    return true;
}

Node * successor(Node * x,double t) {
    Node * best = nullptr;
    while(x != nullptr) {
        if(x->event.time >= t) {
            best = x; 
            x=x->left;
        }
        else {
            x = x->right;
        }
    }
    return best;
}

Node * cancel(Node *x, Event k) {
    return erase(x,k);
    
}


bool validation(Node * x, Event *low, Event *high ) {
    if(!x) return true;

    //to check bst ordering
    if(low != nullptr && !(comparator(*low,x->event))) {
        return false;
    }

    if(high != nullptr && !(comparator(x->event,*high))) {
        return false;
    }

    //to check height balanced
    int m = bf(x);
    int k = abs(m);
    if(k>1) return false;


    // check height of tree 

    int htm = 1 + max(ht(x->left),ht(x->right));
    if(x->h != htm) {
        return false;

    }

    return validation(x->left,low,&x->event) && validation(x->right,&x->event,high);

}

bool validateAvl(Node *x) {
    return validation(x,nullptr,nullptr);

}
void clearTree(Node* x) {
    if (x == nullptr)
        return;

    clearTree(x->left);
    clearTree(x->right);
    delete x;
}



};
}
