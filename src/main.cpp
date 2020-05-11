#include <omp.h>
#include <stdio.h>
#include <iostream>

#include "lock_free_list.cpp"
#include "wait_free_list.cpp"
#include "marked_pointer.h"

int main(int argc, char *argv[])
{
    int nthreads, tid;
    int misses_add, misses_find, misses_del;
    double start_time, end_time, time;

    LockFreeList<int> *list = new LockFreeList<int>(INT32_MIN, INT32_MAX);

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

    const int items = 1E5;
    omp_set_num_threads(8);

    int cas_misses_add[nthreads];
    int cas_misses_find[nthreads];
    int cas_misses_del[nthreads];
   
    #pragma omp parallel private(tid, misses_add, misses_find, misses_del)
    {   
        nthreads = omp_get_num_threads();
        tid = omp_get_thread_num();
        misses_add = 0;
        misses_find = 0;
        misses_del = 0;

        if (tid == 0)
        {   
             printf("\n_____________________\nAdding %d items using %d threads\n", items, nthreads);
            start_time = omp_get_wtime();
        }
        #pragma omp barrier

        int i;
        for (i = (items * tid) / nthreads; i < (items * (tid + 1)) / nthreads; i++)
        {
            list->add(i, &misses_add, &misses_find);
        }

        #pragma omp barrier
        if (tid == 0)
        {
            end_time = omp_get_wtime();
            time = end_time - start_time;
        }

        cas_misses_add[tid] = misses_add;
        cas_misses_del[tid] = misses_del;
        cas_misses_find[tid] = misses_find;

    } //end parallel

    int all_misses_add = 0;
    int all_misses_find = 0;
    int all_misses_del = 0;

    for (int i = 0; i < nthreads; i++)
    {
        all_misses_add += cas_misses_add[i];
        all_misses_find += cas_misses_find[i];
        all_misses_del += cas_misses_del[i];
    }

    printf("CAS Misses: ADD: %d, DEL: %d, FIND: %d\n", 
           all_misses_add, all_misses_del, all_misses_find);
    printf("Duration: %.3lf seconds\n", time);

    //list->print();

    return 0;
}
