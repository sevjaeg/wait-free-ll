# pragma once

#include <atomic>
#include "markable_reference.cpp"

template <class T>
struct Node {
    T item;
    MarkableReference<Node<T>> next;
};

template <class T>
struct Window {
    MarkableReference<Node<T>> pred;
    MarkableReference<Node<T>> curr;
};

//TODO implement sentinel
template <class T>
class LockFreeList {
    public:
    MarkableReference<Node<T>> head;

    LockFreeList() {
        head = *new MarkableReference<Node<T>>(nullptr);
    }

    bool contains(int item) {
        Node<T>* n = head.getPointer();
        if(n == nullptr) {
            return false;
        }

        while (n->item < item )
            n = (n->next).getPointer();
        
        return n->item == item && !((n->next).getFlag());
    }


    // TODO handle empty list
    Window<T> find (T item) {
        retry: while (true) {
            MarkableReference<Node<T>> pred = head;
            MarkableReference<Node<T>> curr = (pred.getAll())->next;
            while(true) {
                MarkableReference<Node<T>> succ = (curr.getAll())->next.getPointer();
                while((curr.getAll())->next.getFlag()) {
                    curr.resetFlag();
                    succ.resetFlag();
                    
                    std::atomic<Node<T>*> aptr;
                    
                    aptr.store((((pred.getAll())->next).getAll()));

                    if(!aptr.compare_exchange_weak(&(curr.getAll()), succ.getAll())) {
                        goto retry;
                    }
                    curr = succ;
                    succ = ((succ.getAll())->next).getPointer();
                    
                    if((curr.getPointer())->item >= item) {
                        Window<T> w;
                        w.curr = curr;
                        w.pred = pred;
                        return w;
                    }
                    pred = curr;
                    curr = (curr.getAll())->next;
                }
            }
        }
    }

};