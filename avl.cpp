#include<iostream>
#include <string>
#include <algorithm>
#include <cstdlib>
using namespace std;

long long leftRotations = 0;
long long rightRotations = 0;

struct Event {
    long long time;
    long long sequence;
    long long id;
    string type;
};


struct Node { 
    Event event;
    int h ; 
    Node * left , * right ; };



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

Node * successor(Node * x,long long t) {
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


int main() {
     // normal code checking
    Node* root = nullptr;

    Event e1 = {30, 1, 101, "Timeout"};
    Event e2 = {20, 2, 102, "ACK"};
    Event e3 = {40, 3, 103, "Timeout"};
    Event e4 = {10, 4, 104, "ACK"};
    Event e5 = {25, 5, 105, "Timeout"};

    root = insert(root, e1);
    root = insert(root, e2);
    root = insert(root, e3);
    root = insert(root, e4);
    root = insert(root, e5);

    if (validateAvl(root)) {
        cout << "PASS: Insertion test\n";
    }
    else {
        cout << "FAIL: Insertion test\n";
    }

    // checking ll
    root = nullptr;

root = insert(root, {30, 1, 101, "Timeout"});
root = insert(root, {20, 2, 102, "Timeout"});
root = insert(root, {10, 3, 103, "Timeout"});

if (validateAvl(root)) {
        cout << "PASS: ll\n";
    }
    else {
        cout << "FAIL: ll\n";
    }

    //checking rr
    root = nullptr;

root = insert(root, {10, 1, 101, "Timeout"});
root = insert(root, {20, 2, 102, "Timeout"});
root = insert(root, {30, 3, 103, "Timeout"});

if (validateAvl(root)) {
        cout << "PASS: rr\n";
    }
    else {
        cout << "FAIL: rr\n";
    }


// checking lr
    root = nullptr;

root = insert(root, {30, 1, 101, "Timeout"});
root = insert(root, {20, 2, 102, "Timeout"});
root = insert(root, {25, 3, 103, "Timeout"});

if (validateAvl(root)) {
        cout << "PASS: lr\n";
    }
    else {
        cout << "FAIL: lr\n";
    }


// checking rl
     root = nullptr;

root = insert(root, {10, 1, 101, "Timeout"});
root = insert(root, {20, 2, 102, "Timeout"});
root = insert(root, {15, 3, 103, "Timeout"});

if (validateAvl(root)) {
        cout << "PASS: rl\n";
    }
    else {
        cout << "FAIL: rl\n";
    }
        



    root = nullptr;

root = insert(root, {30, 1, 101, "Timeout"});
root = insert(root, {10, 2, 102, "ACK"});
root = insert(root, {20, 3, 103, "Timeout"});

root = insert(root, {12, 3, 103, "Timeout"});

Event removed;

if (deleteMin(root, removed) &&
    removed.time == 10 &&
    validateAvl(root) ) {
     cout<<getmin(root)->event.time<<endl;
    cout << "PASS: deleteMin\n";
}
else {
    cout << "FAIL: deleteMin\n";
}


Node* ans = successor(root, 15);

if (ans != nullptr && ans->event.time == 20) {
    cout << "PASS: Successor\n";
}
else {
    cout << "FAIL: Successor\n";
}


root = nullptr;
e1 = {100, 1, 101, "Timeout"};
 e2 = {200, 2, 102, "Timeout"};
e3 = {300, 3, 103, "Timeout"};

root = insert(root, e1);
root = insert(root, e2);
root = insert(root, e3);

root = cancel(root, e2);

if (validateAvl(root)) {
        cout << "PASS: cancel\n";
    }
    else {
        cout << "FAIL: cancel\n";
    }

    return 0;
}