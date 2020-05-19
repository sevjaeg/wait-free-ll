#include <atomic>
#include <iostream>

#include "marked_pointer.h"

using namespace std;

template <class T>
struct LockFreeNode {
    T item;
    atomic<LockFreeNode*> next;
    LockFreeNode(T val): item(val), next(nullptr){}
};

template <class T>
struct LockFreeWindow {
    LockFreeNode<T>* pred;
    LockFreeNode<T>* curr;
};

template <class T>
class LockFreeList {
    private:
    T first_sentinel_value;
    T last_sentinel_value;
    public:
    atomic<LockFreeNode<T>*> head;

    LockFreeList(T first_sentinel_value, T last_sentinel_value) {
        this->first_sentinel_value = first_sentinel_value;
        this->last_sentinel_value = last_sentinel_value;
        head = new LockFreeNode<T>(first_sentinel_value);
        LockFreeNode<T>* tmp = head;
        tmp->next = new LockFreeNode<T>(last_sentinel_value);
    }

    bool contains(T item) {
        LockFreeNode<T>* n = head;
        while (n->item < item) {
            n = (LockFreeNode<T>*) getPointer((n->next));
            if(n == nullptr) {
                return false;
            }
        }
        return n->item == item && !(getFlag(n->next));
    }

    LockFreeWindow<T> find (T item, volatile int* cas_misses) {
        retry: while(true) {
            LockFreeNode<T>* pred = head;
            LockFreeNode<T>* curr = (LockFreeNode<T>*)getPointer(pred->next);
           
            while(true) {
                //link out marked item
                LockFreeNode<T>* succ = (LockFreeNode<T>*)getPointer(curr->next);
                while(getFlag(curr->next)) {
                    //printf("next element\n");
                    resetFlag((void**)&curr);
                    resetFlag((void**)&succ);;
                    
                    if(!pred->next.compare_exchange_weak(curr, succ)) {
                        printf("find CAS failed\n");
                        (*cas_misses) ++;
                        goto retry;
                    }
                    curr = succ;
                    succ = (LockFreeNode<T>*)getPointer(succ->next);
                }

                if((curr->item) >= item) {
                    LockFreeWindow<T> w;
                    w.curr = curr;
                    w.pred = pred;
                    return w;
                }
                pred = curr;
                curr = (LockFreeNode<T>*)getPointer(curr->next);
            }
        }
    }

    int add (T item) {
        LockFreeWindow<T> w;
        LockFreeNode<T>* n = new LockFreeNode<T>(item);
        volatile int cas_misses = 0;
        while (true) {
            w = find(item, &cas_misses);
            LockFreeNode<T>* pred = w.pred;
            LockFreeNode<T>* curr = w.curr;

            if(curr-> item == item) {
                delete(n);
                return -1;
            }

            n->next = curr;

            resetFlag((void **) &n->next);
            resetFlag((void **) &curr);

            if(pred->next.compare_exchange_weak(curr, n)) {
                return cas_misses;
            }
            printf("add CAS failed\n");
            cas_misses++;
        }
    }

    int remove (T item) {
        volatile int cas_misses = 0;
        if(item == INT32_MAX || item == INT32_MIN) {
            return -1; //dont remove sentinels
        }
        LockFreeWindow<T> w;
        while(true){
            w = find(item, &cas_misses);
            if(item != w.curr->item) {
                return -1; //item not in list
            }

            LockFreeNode<T>* succ = w.curr->next;
            LockFreeNode<T>* marked_succ = succ;
            setFlag((void **)&marked_succ);
            resetFlag((void **)&succ);
            
            if(!w.curr->next.compare_exchange_weak(succ, marked_succ)) {
                printf("del CAS1 failed\n");
                cas_misses++;
                continue;
            }
            //printf("del CAS1 success\n");

            w.pred->next.compare_exchange_weak(w.curr, succ);
            return cas_misses;
        }
    }

    void print() {
        LockFreeNode<T>* ptr = head;
        printf("List:[");
        while (ptr != nullptr) {
            printf("%d,", (int) ptr->item);
            ptr = ptr->next;
        }
        printf("]\n");
    }
};