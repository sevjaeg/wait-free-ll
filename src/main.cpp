#include <omp.h>
#include <stdio.h>

 int main(int argc, char *argv[]) {

    int nthreads, tid;

    /* Fork a team of threads with each thread having a private tid variable */
    #pragma omp parallel private(tid)
    {
        tid = omp_get_thread_num();

        printf("Hello World from thread = %d\n", tid);

    }  /* All threads join master thread and terminate */
    
    return 0;
 }
