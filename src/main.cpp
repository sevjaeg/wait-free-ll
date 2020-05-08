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
    list->add(4000);
    list->remove(4);
    list->print();

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

    #pragma omp parallel private(tid)
    {
        tid = omp_get_thread_num();
        nthreads = omp_get_num_threads();
        
        printf("Hello World from thread = %d\n", tid);
    }



    return 0;
}
