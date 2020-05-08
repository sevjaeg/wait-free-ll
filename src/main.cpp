#include <omp.h>
#include <stdio.h>
#include <iostream>

#include "lock_free_list.cpp"
#include "marked_pointer.h"

int main(int argc, char *argv[]) {

    int nthreads, tid;

    LockFreeList<int>* list = new LockFreeList<int>(INT32_MIN, INT32_MAX);

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

    
    double start_time, end_time, time;

    const int items = 1E4;
    omp_set_num_threads(8);

    nthreads = omp_get_num_threads();
    int cas_misses_add[nthreads];
    int cas_misses_find[nthreads];
    int cas_misses_del[nthreads];

    #pragma omp parallel private(tid)
    {
        int cas_miss_add = 0;
        int cas_miss_find = 0;
        int cas_miss_del = 0;

        #pragma omp barrier
        if(tid == 0) {
            start_time = omp_get_wtime();
        }


        tid = omp_get_thread_num();
        //printf("thread %d\n", tid);
        
        int i;
        for(i = (items*tid)/nthreads; i<(items*(tid+1))/nthreads; i++){
            list->add(i);
        }


        #pragma omp barrier
        if(tid == 0) {
            end_time = omp_get_wtime();
            time = end_time - start_time;
        }

        cas_misses_add[tid] = cas_miss_add;
        cas_misses_del[tid] = cas_miss_del;
        cas_misses_find[tid] = cas_miss_del;
        
    } //end parallel

    int all_misses_add = 0;
    int all_misses_find = 0;
    int all_misses_del = 0;

    for(int i=0; i<nthreads; i++) {
        all_misses_add += cas_misses_add[i];
        all_misses_find += cas_misses_find[i];
        all_misses_del += cas_misses_del[i];
    }

    printf("CAS Misses\n \
            ADD: %d, DEL: %d, FIND: %d\n", all_misses_add, all_misses_del, all_misses_find);
    printf("Duration: %.3lf seconds\n", time);

    //list->print();

    return 0;
}
