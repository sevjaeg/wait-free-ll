#include <atomic>
#include <iostream>
#include <vector>
#include <memory>

#include "stamped_marked_pointer.h"

#define DEBUG 0

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
    atomic<WaitFreeNode*> next;
    atomic<bool> success_bit;
    WaitFreeNode(T val): item(val), next(nullptr), success_bit(false){}
};

template <class T>
struct WaitFreeWindow {
    WaitFreeNode<T>* pred, *curr;
    WaitFreeWindow(WaitFreeNode<T>* p, WaitFreeNode<T>* c): pred(p), curr(c){}
};

template <class T>
struct OperationDescriptor {
    volatile long phase;
    volatile OperationType type;
    WaitFreeNode<T>* node;
    WaitFreeWindow<T>* search_result;
    OperationDescriptor(long ph, OperationType t, WaitFreeNode<T>* n, WaitFreeWindow<T>* res): 
        phase(ph), type(t), node(n), search_result(res) {}
};

template <class T>
class WaitFreeList {
    private:
    WaitFreeNode<T> * head, * tail;
    std::vector<unique_ptr<atomic<OperationDescriptor<T>*>>> state;
    volatile int state_size;
    atomic<long> current_max_phase;

    T first_sentinel_value;
    T last_sentinel_value;
    
    /**
     * Constructor
     * Creates sentinels, Initialises state array 
     */
    public : WaitFreeList(T first_sent, T last_sent, int nthreads) {
        #if DEBUG
        cout << "creating wait-free list" << "\n";
        #endif

        first_sentinel_value = first_sent;
        last_sentinel_value = last_sent;
        current_max_phase = 0;
        tail = new WaitFreeNode<T>(last_sentinel_value);
        head = new WaitFreeNode<T>(first_sentinel_value);
        head->next = tail;
        state.resize(nthreads);
        state_size = nthreads;
        for(int i = 0; i < nthreads; i++) {
            OperationDescriptor<T>* op = new OperationDescriptor<T>(0, OperationType::SUCCESS, nullptr, nullptr);
            state.at(i) = make_unique<atomic<OperationDescriptor<T>*>>(op);
        }
    }

    //TODO free list
    private : ~ WaitFreeList() {

    }

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

    public: bool add(int tid, T value, volatile int* cas_misses, volatile int* cas_misses_find) {
        #if DEBUG
        cout << "addding " << value << "\n";
        #endif

        long phase = maxPhase();
        WaitFreeNode<T>* newNode = new WaitFreeNode<T>(value);
        OperationDescriptor<T>* op = new OperationDescriptor<T>(phase, OperationType::ADD, newNode, nullptr);
        *state.at(tid) = op;
        help(phase, cas_misses, cas_misses_find);
        return (**state.at(tid)).type == OperationType::SUCCESS;
    }

    public : bool remove(int tid, T value, volatile int* cas_misses, volatile int* cas_misses_find) {
        #if DEBUG
        cout << "removing " << value << "\n";
        #endif

        if(value == first_sentinel_value || value == last_sentinel_value) {
            return false; //dont remove sentinels
        }
        long phase = maxPhase();
        WaitFreeNode<T>* newNode = new WaitFreeNode<T>(value);
        OperationDescriptor<T>* op = new OperationDescriptor<T>(phase, OperationType::SEARCH_REMOVE, newNode, nullptr);
        *state.at(tid) = op;
        help(phase, cas_misses, cas_misses_find);
        op = *state.at(tid);
        if(op->type == OperationType::DETERMINE_REMOVE) {
            bool a = false; // workaround
            return op->search_result->curr->success_bit.compare_exchange_strong(a, true);
        }
        return false;
    }

    public : WaitFreeWindow<T>* search(int tid, T value, long phase, volatile int * cas_misses) {
        WaitFreeNode<T> *pred = nullptr, *curr = nullptr, *succ = nullptr;
        bool marked = false;
        bool snip;

        retry: while(true) {
            pred = head;
            curr = (WaitFreeNode<T>*)getPointer(pred->next);
            while(true) {
                succ = (WaitFreeNode<T>*)getPointer(curr->next);
                marked = getFlag(curr->next);
                while(marked) {
                    //node logically deleted, remove it physically
                    snip = compareAndSet(pred->next, curr, succ, false, false);

                    if(!isSearchStillPending(tid, phase)) {
                        return nullptr;
                    }
                    if (!snip) {
                        #if DEBUG
                        printf("find CAS failed\n");
                        #endif
                        
                        (*cas_misses)++;
                        goto retry;
                    }
                    #if DEBUG
                    cout << "Search removed node physically\n";
                    #endif

                    curr = succ;
                    succ = (WaitFreeNode<T>*)getPointer(curr->next);
                    marked = getFlag(curr->next);
                }
                if(curr->item >= value) {
                    #if DEBUG
                    cout << "searched " << value << ", found window " << pred->item << "," << curr->item <<"\n";
                    #endif

                    return new WaitFreeWindow<T>(pred, curr);
                }
                pred = curr;
                curr = succ;
            }
        }
    }

    private: void help(long phase, volatile int* cas_misses, volatile int* cas_misses_find) {
        for(int i = 0; i < state_size; i++) {
            OperationDescriptor<T>* desc = *state.at(i);
            if(desc->phase <= phase) { // help all pending operations
                if(desc->type == OperationType::ADD) {
                    #if DEBUG
                    cout << "helping add\n";
                    #endif

                    helpAdd(i, desc->phase, cas_misses, cas_misses_find);
                } else if(desc->type == OperationType::SEARCH_REMOVE || desc->type == OperationType::EXECUTE_REMOVE) {
                    #if DEBUG
                    cout << "helping remove\n";
                    #endif

                    helpRemove(i, desc->phase, cas_misses, cas_misses_find);
                }
            }
        }
    }

    private: void helpAdd(int tid, long phase, volatile int* cas_misses, volatile int* cas_misses_find) {
        while(true) {
            OperationDescriptor<T>* op = *state.at(tid);
            if(!(op->type == OperationType::ADD && op->phase == phase)) {
                #if DEBUG
                cout << "add no longer relevant\n";
                #endif

                return;
            }
            WaitFreeNode<T>* node = op->node;
            WaitFreeNode<T>* next_node = (WaitFreeNode<T>*)getPointer(node->next);
            WaitFreeWindow<T>* window = search(tid, node->item, phase, cas_misses_find);
            if(window == nullptr) {
                #if DEBUG
                cout << "window == nullptr\n";
                #endif

                return;
            }
            if(window->curr->item == node->item) {
                if((window->curr == node) || getFlag(node->next)) {
                    // node already inserted
                    #if DEBUG
                    cout << "node already inserted before: success\n";
                    #endif

                    OperationDescriptor<T>* success = new OperationDescriptor<T>(phase, OperationType::SUCCESS,
                        node, nullptr);
                    if((*state[tid]).compare_exchange_strong(op, success)) {
                        return;
                    }
                } else {
                    #if DEBUG
                    cout << "node not yet inserted: failure\n";
                    #endif
                    
                    OperationDescriptor<T>* failure = new OperationDescriptor<T>(phase, OperationType::FAILURE,
                        node, nullptr);
                    if((*state[tid]).compare_exchange_strong(op, failure)) {
                        return;
                    }
                } 
            } else {
                if(getFlag(node->next)) {
                    #if DEBUG
                    cout << "node already inserted and deleted: success\n";
                    #endif

                    OperationDescriptor<T>* success = new OperationDescriptor<T>(phase, OperationType::SUCCESS,
                        node, nullptr);
                    if((*state[tid]).compare_exchange_strong(op, success)) {
                        return;
                    }
                }

                int version = getStamp(window->pred->next);
                OperationDescriptor<T> *newOp = new OperationDescriptor<T>(phase, OperationType::ADD, node, nullptr);
                if(!(*state[tid]).compare_exchange_strong(op, newOp)) {
                    continue;
                }
                
                compareAndSet(node->next, next_node, (WaitFreeNode<T>*)getPointer(window->curr), false, false);
                if(compareAndSetVersion(version, window->pred->next, (WaitFreeNode<T>*)getPointer(node->next), (WaitFreeNode<T>*)getPointer(node), false, false)) {
                    OperationDescriptor<T> *success = new OperationDescriptor<T>(phase, OperationType::SUCCESS, node, nullptr);
                    if((*state[tid]).compare_exchange_strong(newOp, success)) {
                        return;
                    } 
                } else {
                    #if DEBUG
                    printf("add CAS failed\n");
                    #endif

                    (*cas_misses)++;
                }
            }
        }
    }

    private: void helpRemove(int tid, long phase, volatile int* cas_misses, volatile int* cas_misses_find) {
        while(true) {
            OperationDescriptor<T> *op = *state.at(tid);
            if(!((op->type == OperationType::SEARCH_REMOVE || op->type == OperationType::EXECUTE_REMOVE)
                && op->phase == phase)) {
                    #if DEBUG
                    cout << "remove no longer relevant\n";
                    #endif

                    return;
            }
            WaitFreeNode<T> *node = op->node;
            if(op->type == OperationType::SEARCH_REMOVE) {
                WaitFreeWindow<T> *window = search(tid, node->item, phase, cas_misses_find);
                if(window == nullptr) {
                    #if DEBUG
                    cout << "window == nullptr\n";
                    #endif

                    continue;
                }
                if(window->curr->item != node->item) {
                    #if DEBUG
                    cout << "Searched item " << node->item << " not in list\n";
                    #endif

                    OperationDescriptor<T> *failure = new OperationDescriptor<T>(phase, OperationType::FAILURE, node, nullptr);
                    if((*state[tid]).compare_exchange_strong(op, failure)) {
                        return;
                    }
                } else {
                    #if DEBUG
                    cout << "found item, ready for execute\n";
                    #endif

                    OperationDescriptor<T> *found = new OperationDescriptor<T>(phase, OperationType::EXECUTE_REMOVE, node, window);
                    (*state[tid]).compare_exchange_strong(op, found);
                }
            } else if(op->type == OperationType::EXECUTE_REMOVE) {
                WaitFreeNode<T>* next = (WaitFreeNode<T>*)getPointer(op->search_result->curr->next);
                #if DEBUG
                cout << "Try to mark node\n";
                #endif

                if(!attemptFlag(op->search_result->curr->next, next, true)) {
                     #if DEBUG
                    cout << "Unable to mark, try again\n";
                    printf("del logical CAS failed\n");
                    #endif
                    (*cas_misses)++;
                    continue;
                }
                #if DEBUG
                cout << "Flag set to " << getFlag(op->search_result->curr->next) <<"\n";

                #endif
                //removes node physically
                search(tid, op->node->item, phase, cas_misses_find);
                #if DEBUG
                //cout << "Node removed by search\n";

                #endif

                OperationDescriptor<T> *determine = new OperationDescriptor<T>(op->phase, OperationType::DETERMINE_REMOVE, op->node, op->search_result);
                (*state[tid]).compare_exchange_strong(op, determine);
                return;
            }
        }
    }

    private : bool compareAndSet(atomic<WaitFreeNode<T>*> &current, WaitFreeNode<T>* expected, WaitFreeNode<T>* desired, bool expectedFlag, bool desiredFlag) {
        WaitFreeNode<T>* current_old = current;
        WaitFreeNode<T>* desired_full = desired;
        if(desiredFlag) {
            setFlag((void **) &desired_full);
        }
        setStamp((void **) &desired_full, getStamp(current) + 1);
        return expected == (WaitFreeNode<T>*)getPointer(current_old) &&
            expectedFlag == getFlag(current_old) &&
            ((desired == (WaitFreeNode<T>*)getPointer(current_old) &&
            desiredFlag == getFlag(current_old)) ||
            current.compare_exchange_strong(current_old, desired_full));
    }

    private : bool compareAndSetVersion(int version, atomic<WaitFreeNode<T>*> &current, WaitFreeNode<T>* expected, WaitFreeNode<T>* desired, bool expectedFlag, bool desiredFlag) {
        WaitFreeNode<T>* current_old = current;
        WaitFreeNode<T>* desired_full = desired;
        if(desiredFlag) {
            setFlag((void **) &desired_full);
        }
        setStamp((void **) &desired_full, getStamp(current) + 1);

        return expected == (WaitFreeNode<T>*)getPointer(current_old) &&
            expectedFlag == getFlag(current_old) &&
            version == getStamp(current_old) &&
            ((desired == (WaitFreeNode<T>*)getPointer(current_old) &&
            desiredFlag == getFlag(current_old)) ||
            current.compare_exchange_strong(current_old, desired_full));
    }

    private : bool attemptFlag(atomic<WaitFreeNode<T>*> &current, WaitFreeNode<T>* expected, bool newFlag) {
        WaitFreeNode<T>* current_old = current;
        WaitFreeNode<T>* desired_full = expected;

        if(newFlag) {
            setFlag((void **) &desired_full);
        }
        
        setStamp((void **) &desired_full, getStamp(current) + 1);
        
        return expected == (WaitFreeNode<T>*)getPointer(current_old)
            && (newFlag == getFlag(current_old) ||
            current.compare_exchange_strong(current_old, desired_full));

    }

    private: long maxPhase() {
        long result = current_max_phase;
        // the max phase has to be increased before the thread proceeds
        current_max_phase.compare_exchange_strong(result, result + 1);
        return result;
    }

    private: bool isSearchStillPending(int tid, long ph)  {
        OperationDescriptor<T> curr = **state[tid];
        return curr.phase == ph && (curr.type == OperationType::ADD || curr.type == OperationType::SEARCH_REMOVE
                || curr.type == OperationType::EXECUTE_REMOVE);
    }

    public: void print() {
        WaitFreeNode<T>* ptr = head;
        printf("List:[");
        while (ptr != nullptr) {
            int val = ptr->item;
            printf("%d,", val);
            ptr = (WaitFreeNode<T> *)getPointer(ptr->next);
        }
        printf("]\n");
    }
};
