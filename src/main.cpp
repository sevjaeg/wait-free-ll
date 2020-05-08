#include <omp.h>
#include <stdio.h>
#include <iostream>

#include "lock_free_list.cpp"
#include "markable_reference.cpp"

 int main(int argc, char *argv[]) {

    int nthreads, tid;

    LockFreeList<int>* list = new LockFreeList<int>();
    std::cout << list->contains(4) << "\n";
    list->find(4);

    #pragma omp parallel private(tid)
    {
        tid = omp_get_thread_num();
        nthreads = omp_get_num_threads();
        
        printf("Hello World from thread = %d\n", tid);
    }



    return 0;
 }
