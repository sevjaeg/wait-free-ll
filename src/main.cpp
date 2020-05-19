#include <omp.h>
#include <stdio.h>
#include <iostream>

#include "lock_free_list.cpp"
#include "wait_free_list.cpp"
#include "marked_pointer.h"

int main(int argc, char *argv[])
{
    int nthreads, tid;
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

    const int items = 5E4;
    omp_set_num_threads(8);

    int cas_misses_add[nthreads + 32];
    int cas_misses_del[nthreads + 32];
   
    #pragma omp parallel private(tid)
    {   
        nthreads = omp_get_num_threads();
        tid = omp_get_thread_num();
        int misses_add = 0;
        int misses_del = 0;

        if (tid == 0)
        {   
            printf("\n_____________________\nAdding %d items using %d threads\n", items, nthreads);
            start_time = omp_get_wtime();
        }
        #pragma omp barrier

        int i;
        for (i = (items * tid) / nthreads; i < (items * (tid + 1)) / nthreads; i++)
        {   
            int misses =  list->add(i);
            misses_add += misses;
            /*if(misses != 0) {
                printf("miss @t%d,i=%d\n", tid, i);
            }*/
        }

        #pragma omp barrier
        if (tid == 0)
        {
            end_time = omp_get_wtime();
            time = end_time - start_time;
        }

        cas_misses_add[tid] = misses_add;
        cas_misses_del[tid] = misses_del;

        

    } //end parallel

    printf("ADD: ");
    for(int i = 0; i < nthreads; i++) {
        printf("%d ", cas_misses_add[i]);
    }
    printf("\n");

    printf("DEL: ");
    for(int i = 0; i < nthreads; i++) {
        printf("%d ", cas_misses_del[i]);
    }
    printf("\n");



    int all_misses_add = 0;
    int all_misses_del = 0;

    for (int i = 0; i < nthreads; i++)
    {
        all_misses_add += cas_misses_add[i];
        all_misses_del += cas_misses_del[i];
    }

    printf("CAS Misses: ADD: %d, DEL: %d\n", 
           all_misses_add, all_misses_del);
    printf("Duration: %.3lf seconds\n", time);

    //list->print();

    return 0;
}
