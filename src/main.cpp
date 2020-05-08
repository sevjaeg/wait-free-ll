#include <omp.h>
#include <stdio.h>
#include <iostream>

#include "lock_free_list.cpp"
#include "marked_pointer.h"

int main(int argc, char *argv[]) {

    int nthreads, tid;

    LockFreeList<int>* list = new LockFreeList<int>(INT32_MIN, INT32_MAX);
    list->add(4);
    list->add(4000);
    list->remove(6);
    list->print();
    list->add(-4);
    list->add(-40);
    list->add(4000);
    list->remove(4);
    list->print();
    list->contains(4);
    list->contains(5);

    /*
    Node<int> n;
    n.item = 4;
    n.next = nullptr;

    Node<int> * nptr;
    nptr = & n;

    std::cout << getPointer(nptr) <<"\n";
    std::cout << getFlag(nptr)<<"\n";

    setFlag((void**)&nptr);

    std::cout << getPointer(nptr)<<"\n";
    std::cout << getFlag(nptr)<<"\n";

    resetFlag((void**)&nptr);
    std::cout << getFlag(nptr)<<"\n";
    */

    const int items = 1000;

    omp_set_num_threads(8);

    #pragma omp parallel private(tid)
    {
        tid = omp_get_thread_num();
        printf("thread %d\n", tid);

        nthreads = omp_get_num_threads();
        for(int i = (items*tid)/nthreads; i<(items*(tid+1))/nthreads; i++){
            list->add(i);
        }
        
    }
    list->print();

    return 0;
}
