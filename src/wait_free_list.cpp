#include <atomic>
#include <iostream>

#include "stamped_pointer.h"

using namespace std;

enum OperationType{
    ADD,
    SEARCH_REMOVE,
    EXECUTE_REMOVE,
    SUCCESS,
    FAILURE,
    DETERMINE_REMOVE
};

template <class T>
struct WaitFreeNode {
    T item;
    atomic<bool> success_bit;
    atomic<WaitFreeNode*> next;
    WaitFreeNode(T val): item(val), next(nullptr), success_bit(false){}
};

template <class T>
struct WaitFreeWindow {
    WaitFreeNode<T>* pred;
    WaitFreeNode<T>* curr;
};

template <class T>
struct OperationDescription {
    long phase;
    OperationType type;
    WaitFreeNode<T> node;
    WaitFreeWindow<T> search_result;
    OperationDescription(long ph, OperationType t, WaitFreeNode<T> n, WaitFreeWindow<T> res): 
        phase(ph), type(t), node(n), search_result(res) {}
};



template <class T>
class WaitFreeList {
    private:
    atomic<WaitFreeNode<T>*> head, tail;
    atomic<OperationDescription<T>*> state[];
    atomic<long> max_phase;

    /**
     * Not the strictly wait-free implementation from the paper, but the basic
     * version from the reference implementation
     */
    public: bool contains(T item) {
        WaitFreeNode<T>* n = head;
        while (n->item < item) {
            n = (WaitFreeNode<T>*) getPointer((n->next));
            if(n == nullptr) {
                return false;
            }
        }
        return n->item == item && !(getFlag(n->next));
    }

    public: bool add(int tid, T value) {

    }

    public : bool remove(int tid, T value) {

    }

    public : WaitFreeWindow<T> search(int tid, T value, long phase) {

    }

    private: void help(long phase) {

    }

    private: void helpInsert(int tid, long phase) {

    }

    private: void helpDelete(int tid, long phase) {

    }

    private: long maxPhase() {}

    private: bool isSearchStillPending(int tid, long phase)  {

    }
};
