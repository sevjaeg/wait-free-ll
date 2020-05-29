#include <atomic>
#include <iostream>

#include "stamped_marked_pointer.h"

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
    WaitFreeNode<T>* node;
    WaitFreeWindow<T> search_result;
    OperationDescription(long ph, OperationType t, WaitFreeNode<T>* n, WaitFreeWindow<T> res): 
        phase(ph), type(t), node(n), search_result(res) {}
};

template <class T>
class WaitFreeList {
    private:
    atomic<WaitFreeNode<T>*> head, tail;
    atomic<OperationDescription<T>*> state[];
    atomic<long> current_max_phase;

    T first_sentinel_value;
    T last_sentinel_value;

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
        long phase = maxPhase();
        WaitFreeNode<T> newNode(value);
        OperationDescription<T>* op = new OperationDescription<T>(phase, OperationType::ADD, newNode, nullptr);
        state[tid] = op;
        help(phase);
        return state[tid] == OperationType::SUCCESS;
    }

    public : bool remove(int tid, T value) {
        if(value == first_sentinel_value || value == last_sentinel_value) {
            return false; //dont remove sentinels
        }
        long phase = maxPhase();
        WaitFreeNode<T> newNode(value);
        OperationDescription<T>* op = new OperationDescription<T>(phase, OperationType::SEARCH_REMOVE, newNode, nullptr);
        state[tid] = op;
        help(phase);
        op = state[tid];
        if(op->type == OperationType::DETERMINE_REMOVE) {
            return op->search_result.curr->success_bit.compare_exchange_strong(false, true);
        }
        return false;
    }

    public : WaitFreeWindow<T> search(int tid, T value, long phase) {
        WaitFreeNode<T>* pred = nullptr, curr = nullptr, succ = nullptr;
        //bool[] marked = {false};
        bool snip;
    }

    private: void help(long phase) {
        for(int i = 0; i < state.size(); i++) {
            OperationDescription<T>* desc = state[i];
            if(desc->phase <= phase) { // help all pending operations
                if(desc->type == OperationType::ADD) {
                    helpAdd(i, desc->phase);
                } else if(desc->type == OperationType::SEARCH_REMOVE || desc->type == OperationType::EXECUTE_REMOVE) {
                    helpRemove(i, desc->phase);
                }
            }
        }
    }

    private: void helpAdd(int tid, long phase) {
        while(true) {
            OperationDescription<T>* op = state[tid];
            if(op->type == OperationType::ADD || op->phase == phase) {
                return;
            }
            WaitFreeNode<T>* node = op->node;
            WaitFreeNode<T>* next_node = getPointer(node->next);
            WaitFreeWindow<T> window = search(node->item, tid, phase);
            if(window == nullptr) {
                return;
            }
            if(window.curr->item == node->item) {
                if((window.curr == *node) || getFlag(node->next)) {
                    // node already inserted
                    OperationDescription<T>* success = new OperationDescription<T>(phase, OperationType::SUCCESS,
                        node, nullptr);
                    // TODO cas for this case
                    if(state[tid].compare_exchange_strong()) {
                        return;
                    }
                } else {
                    OperationDescription<T>* failure = new OperationDescription<T>(phase, OperationType::FAILURE,
                        node, nullptr);
                    // TODO cas
                    if(state) {
                        return;
                    }
                } 
            } else {
                if(getFlag(node->next)) {
                    OperationDescription<T>* success = new OperationDescription<T>(phase, OperationType::SUCCESS,
                        node, nullptr);
                    // TODO cas
                    if(state) {
                        return;
                    }
                }
                int version = getStamp(window.pred->next);
                
            }
        }
    }

    private: void helpRemove(int tid, long phase) {

    }

    private: long maxPhase() {
        long result = current_max_phase;
        // the max phase has to be increased before the thread proceeds
        current_max_phase.compare_exchange_strong(result, result + 1);
        return result;
    }

    private: bool isSearchStillPending(int tid, long ph)  {
        OperationDescription<T> curr = state[tid];
        return curr.phase == ph && (curr.type == OperationType::ADD || curr.type == OperationType::SEARCH_REMOVE
                || curr.type == OperationType::EXECUTE_REMOVE);
    }

    public: void print() {
        WaitFreeNode<T>* ptr = head;
        printf("List:[");
        while (ptr != nullptr) {
            printf("%d,", (int) ptr->item);
            ptr = ptr->next;
        }
        printf("]\n");
    }
};
