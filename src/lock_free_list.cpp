# pragma once

#include <atomic>
#include <iostream>

#include "marked_pointer.h"

template <class T>
struct Node {
    T item;
    Node* next;
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
    Node<T>* head;

    LockFreeList(T first_sentinel_value, T last_sentinel_value) {
        this->first_sentinel_value = first_sentinel_value;
        this->last_sentinel_value = last_sentinel_value;
        head = new Node<T>(first_sentinel_value);
        head->next = new Node<T>(last_sentinel_value);
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


    Window<T> find (T item) {
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
                    resetFlag((void**)&succ);

                    std::atomic<Node<T>*> aptr;
                    aptr.store(pred->next);
                    
                    if(!aptr.compare_exchange_strong(curr, succ)) {
                        pred->next = aptr.load();
                        //printf("find CAS success\n");
                        goto retry;
                    }
                    //printf("find CAS failed\n");
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

    bool add (T item) {
        //printf("adding\n");
        Window<T> w;
        Node<T>* n = new Node<T>(item) ;
        while (true) {
            w = find(item);
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

            std::atomic<Node<T>*> aptr;
            aptr.store(pred->next);
            if(aptr.compare_exchange_weak(curr, n)) {
                pred->next = aptr.load();
                //printf("add CAS success\n");
                return true;
            }
            //printf("add CAS failed\n");
        }
    }

    bool remove (T item) {
        if(item == INT32_MAX || item == INT32_MIN) {
            return false; //dont remove sentinels
        }
        Window<T> w;
        while(true){
            w = find(item);
            if(item != w.curr->item) {
                return false; //item not in list
            }

            Node<T>* succ = w.curr->next;
            Node<T>* marked_succ = succ;
            setFlag((void **)&marked_succ);
            resetFlag((void **)&succ);

            std::atomic<Node<T>*> aptr;
            aptr.store(w.curr->next);
            if(!aptr.compare_exchange_weak(succ, marked_succ)) {
                w.curr->next = aptr.load();
                //printf("del CAS1 failed\n");
                continue;
            }
            //printf("del CAS1 success\n");

            aptr.store(w.pred->next);
            aptr.compare_exchange_weak(w.curr, succ);
            w.pred->next = aptr.load();
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