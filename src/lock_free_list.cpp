#include <atomic>
#include <iostream>

#include "marked_pointer.h"

using namespace std;

template <class T>
struct Node {
    T item;
    atomic<Node*> next;
    Node(T val): item(val), next(nullptr){}
};

template <class T>
struct Window {
    Node<T>* pred;
    Node<T>* curr;
};

template <class T>
class LockFreeList {
    private:
    T first_sentinel_value;
    T last_sentinel_value;
    public:
    atomic<Node<T>*> head;

    LockFreeList(T first_sentinel_value, T last_sentinel_value) {
        this->first_sentinel_value = first_sentinel_value;
        this->last_sentinel_value = last_sentinel_value;
        head = new Node<T>(first_sentinel_value);
        Node<T>* tmp = head;
        tmp->next = new Node<T>(last_sentinel_value);
    }

    bool contains(T item) {
        Node<T>* n = head;
        while (n->item < item) {
            n = (Node<T>*) getPointer((n->next));
            if(n == nullptr) {
                return false;
            }
        }
        return n->item == item && !(getFlag(n->next));
    }

    Window<T> find (T item, int* cas_misses) {
        retry: while(true) {
            Node<T>* pred = head;
            Node<T>* curr = (Node<T>*)getPointer(pred->next);
            
            //std::cout << pred->next <<"\n";
            //std::cout << "pred:" << pred << " val:" << pred->item << "\n" <<"curr:" <<curr << " val:" << curr->item << "\n";

            while(true) {
                //link out marked item
                Node<T>* succ = (Node<T>*)getPointer(curr->next);
                while(getFlag(curr->next)) {
                    //printf("next element\n");
                    resetFlag((void**)&curr);
                    resetFlag((void**)&succ);;
                    
                    if(!pred->next.compare_exchange_weak(curr, succ)) {
                        printf("find CAS failed\n");
                        (*cas_misses)++;
                        goto retry;
                    }
                    curr = succ;
                    succ = (Node<T>*)getPointer(succ->next);
                }

                if((curr->item) >= item) {
                    Window<T> w;
                    w.curr = curr;
                    w.pred = pred;
                    //printf("returning hit: curr: %p pred: %p\n", curr, pred);
                    return w;
                }
                pred = curr;
                curr = (Node<T>*)getPointer(curr->next);
            }
        }
    }

    bool add (T item, int* cas_misses, int* cas_misses_find) {
        //printf("adding\n");
        Window<T> w;
        Node<T>* n = new Node<T>(item) ;
        while (true) {
            w = find(item, cas_misses_find);
            Node<T>* pred = w.pred;
            Node<T>* curr = w.curr;

            if(curr-> item == item) {
                //printf("duplicate\n");
                delete(n);
                return false;
            }

            n->next = curr;

            resetFlag((void **) &n->next);
            resetFlag((void **) &curr);

            if(pred->next.compare_exchange_weak(curr, n)) {
                return true;
            }
            printf("add CAS failed\n");
            (*cas_misses)++;
        }
    }

    bool remove (T item, int* cas_misses, int* cas_misses_find) {
        if(item == INT32_MAX || item == INT32_MIN) {
            return false; //dont remove sentinels
        }
        Window<T> w;
        while(true){
            w = find(item, cas_misses_find);
            if(item != w.curr->item) {
                return false; //item not in list
            }

            Node<T>* succ = w.curr->next;
            Node<T>* marked_succ = succ;
            setFlag((void **)&marked_succ);
            resetFlag((void **)&succ);
            
            if(!w.curr->next.compare_exchange_weak(succ, marked_succ)) {
                printf("del CAS1 failed\n");
                (*cas_misses)++;
                continue;
            }
            //printf("del CAS1 success\n");

            w.pred->next.compare_exchange_weak(w.curr, succ);
            return true;
        }
    }

    void print() {
        Node<T>* ptr = head;
        printf("List:[");
        while (ptr != nullptr) {
            printf("%d,", (int) ptr->item);
            ptr = ptr->next;
        }
        printf("]\n");
    }
};