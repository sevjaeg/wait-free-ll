#include <omp.h>
#include <stdio.h>
#include <iostream>
#include <random>

#include "lock_free_list.cpp"
#include "wait_free_list.cpp"
#include "stamped_marked_pointer.h"

#define LOCK_FREE 1
#define WAIT_FREE 1

#define OPERATIONAL 1
#define CLEANUP 1

int main(int argc, char *argv[])
{
    // Experiment parameters
    const int items = 1E4;
    const int iterations_operational = 1E5;
    omp_set_num_threads(8);

    default_random_engine generator;
    uniform_int_distribution<int> dist_op(0,1);
    uniform_int_distribution<int> dist_item(0,2*items-1);

    double start_time, end_time;
    int nthreads, tid;    

#if LOCK_FREE
    LockFreeList<int> *lfList = new LockFreeList<int>(INT32_MIN, INT32_MAX);
    int cas_misses_add[nthreads + 32];
    int cas_misses_del[nthreads + 32];
    int cas_misses_find[nthreads + 32];
    double time_fill_lf, time_op_lf = 0, time_clean_lf = 0;

    #pragma omp parallel private(tid)
    {   
        nthreads = omp_get_num_threads();
        tid = omp_get_thread_num();

        int misses_add = 0;
        int misses_find = 0;
        int misses_del = 0;
        if (tid == 0)
        {   
            printf("\n_____________________\nLock-Free: %d items, %d threads\n", items, nthreads);
            start_time = omp_get_wtime();
        }
        #pragma omp barrier

        for (int i = (items * tid) / nthreads; i < (items * (tid + 1)) / nthreads; i++)
        {   
            volatile int misses_a = 0, misses_f = 0;
            if(lfList->add(i, &misses_a, &misses_f)) {
                misses_add += misses_a;
                misses_find += misses_f;
            }
        }  
        #pragma omp barrier
        if (tid == 0)
        {
            end_time = omp_get_wtime();
            time_fill_lf = end_time - start_time;
        }

        #if OPERATIONAL
        if(tid == 0) {
            printf("Operational Benchmark\n");
            start_time = omp_get_wtime();    
        }
        #pragma omp barrier

        for (int i = (iterations_operational * tid) / nthreads; i < (iterations_operational * (tid + 1)) / nthreads; i++) {
            int operation = dist_op(generator);
            int item = dist_item(generator);
            
            if(operation) {
                volatile int misses_d = 0, misses_f = 0;
                if(lfList->remove(item, &misses_d, &misses_f)) {
                    misses_del += misses_d;
                    misses_find += misses_f;
                }
            } else {
                volatile int misses_a = 0, misses_f = 0;
                if(lfList->add(item, &misses_a, &misses_f)) {
                    misses_add += misses_a;
                    misses_find += misses_f;
                }
            }
        }

        #pragma omp barrier
        if (tid == 0)
        {
            end_time = omp_get_wtime();
            time_op_lf = end_time - start_time;
        }
        #endif //operational

        #if CLEANUP
        if(tid == 0) {
            printf("Removing all elements\n");
            start_time = omp_get_wtime();
        }
        #pragma omp barrier
        for (int i = (2 * items * tid) / nthreads; i < (2 * items * (tid + 1)) / nthreads; i++)
        {   
            volatile int misses_d = 0, misses_f = 0;
            if(lfList->remove(i, &misses_d, &misses_f)) {
                misses_del += misses_d;
                misses_find += misses_f;
            }
        }
        #pragma omp barrier
        if (tid == 0)
        {
            end_time = omp_get_wtime();
            time_clean_lf = end_time - start_time;
        }
        #endif //cleanup

        cas_misses_add[tid] = misses_add;
        cas_misses_del[tid] = misses_del;
        cas_misses_find[tid] = misses_find;
    } //end parallel

    int all_misses_add = 0;
    int all_misses_del = 0;
    int all_misses_find = 0;

    for (int i = 0; i < nthreads; i++)
    {
        all_misses_add += cas_misses_add[i];
        all_misses_del += cas_misses_del[i];
        all_misses_find += cas_misses_find[i];
    }
    
    printf("\nDuration Lock-Free: \nFill: %.3lf seconds\nOperation: %.3lf seconds\nCleanup: %.3lf seconds\n",
        time_fill_lf, time_op_lf, time_clean_lf);
    printf("CAS Misses: ADD: %d, FIND: %d, DEL: %d\n", 
           all_misses_add, all_misses_del, all_misses_find);
    printf("ADD: ");
    for(int i = 0; i < nthreads; i++) {
        printf("%d ", cas_misses_add[i]);
    }
    printf("\n");
    printf("FIND: ");
    for(int i = 0; i < nthreads; i++) {
        printf("%d ", cas_misses_find[i]);
    }
    printf("\n");
    printf("DEL: ");
    for(int i = 0; i < nthreads; i++) {
        printf("%d ", cas_misses_del[i]);
    }
    printf("\n");
    //lfList->print();

#endif //lock-free benchmark

#if WAIT_FREE
    WaitFreeList<int> *wfList;
    double time_fill_wf, time_op_wf = 0, time_clean_wf = 0;

    #pragma omp parallel private(tid)
    {   
        nthreads = omp_get_num_threads();
        tid = omp_get_thread_num();
        if (tid == 0) {
            wfList = new WaitFreeList<int>(INT32_MIN, INT32_MAX, nthreads);
            printf("\n_____________________\nWait-Free: %d items, %d threads\n", items, nthreads);
            start_time = omp_get_wtime();
        }
        #pragma omp barrier

        for (int i = (items * tid) / (nthreads); i < (items * (tid + 1)) / (nthreads); i++)
        {   
            wfList->add(tid, i);
        }
        #pragma omp barrier
        if (tid == 0)
        {
            end_time = omp_get_wtime();
            time_fill_wf = end_time - start_time;
            //wfList->print();
        }

        #if OPERATIONAL
        if(tid == 0) {
            printf("Operational Benchmark\n");
            start_time = omp_get_wtime();    
        }
        #pragma omp barrier
        for (int i = (iterations_operational * tid) / nthreads; i < (iterations_operational * (tid + 1)) / nthreads; i++) {
            int operation = dist_op(generator);
            int item = dist_item(generator);
            
            if(operation) {
                wfList->remove(tid, item);
            } else {
                wfList->add(tid, item);
            }
        }
        #pragma omp barrier
        if (tid == 0)
        {
            end_time = omp_get_wtime();
            time_op_wf = end_time - start_time;
        }
        #endif //operational

        #if CLEANUP
        if(tid == 0) {
            printf("Removing all elements\n");
            start_time = omp_get_wtime();
        }
        #pragma omp barrier
        for (int i = (2 *items * tid) / (nthreads); i < (2 * items * (tid + 1)) / (nthreads); i++)
        {   
            //TODO does not work!
            if(wfList->contains(i)) {
                wfList->remove(tid, i);
            }
        }
        #pragma omp barrier
        if (tid == 0)
        {
            end_time = omp_get_wtime();
            time_clean_wf = end_time - start_time;
        }
        #endif //cleanup
    } // end parallel  

    printf("\nDuration Wait-Free: \nFill: %.3lf seconds\nOperation: %.3lf seconds\nCleanup: %.3lf seconds\n",
        time_fill_wf, time_op_wf, time_clean_wf);
    //wfList->print();

#endif //wait-free benchmark

    return 0;
}
