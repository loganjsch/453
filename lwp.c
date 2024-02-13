/*
CSC 453 Project 2 Lightweight Processes

Kenneth Choi & Logan Schwarz

*/
#include <stdio.h>
#include <stdlib.h>
#include "lwp.h"
#include <pthread.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <bits/mman.h>


#
/*
* _SC_PAGE_SIZE = 4096 bytes
* RLIMIT_STACK = 8388608 bytes
* 8388608 / 4096 = 2048 ("safe size = 8 mil")
*/

tid_t lwp_create(lwpfun function, void *argument){
    /* 
    Creates a new lightweight pricess which
    executes the given function with the given argument
    returns thread id of new_thread, else NO_THREAD
    */
    void *s;
    struct rlimit r;
    size_t stackSize = 0;
    long testVal = sysconf(_SC_PAGE_SIZE);
    int result = getrlimit(RLIMIT_STACK, &r);
    if (result == 0){
        /* check if soft limit is RLIM_INFINITY */
        if (r.rlim_cur == RLIM_INFINITY){
            stackSize = 8 * 1024 * 1024;
        }
        else{
            if ((r.rlim_cur % testVal) == 0){
                stackSize = r.rlim_cur;
            }
            else{
                /* round up to nearest page size */
                stackSize = (r.rlim_cur + testVal) - (r.rlim_cur % testVal);
            }
        }
    }
    else{
        perror("GetRLimit");
        exit(-1);
    }

    

    printf("_SC_PAGE_SIZE: %ld \n", testVal);
    printf("RLIMIT_STACK: %ld\n", r.rlim_cur);

    printf("limit / pagesize = %ld\n", r.rlim_cur/testVal);
    printf("remainder %ld\n", r.rlim_cur % testVal);

    /* allocate space for lwp */
    /* MAP_ANONYMOUS & MAP_STACK part of <sys/mman.h> according to documentation... */
    s = mmap(NULL, stackSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    if (s == MAP_FAILED){
        perror("mmap");
        exit(-1);
    }

    if (munmap(s, stackSize) == -1){
        perror("munmap");
        exit(-1);
    }
    
    return 0;
}

void lwp_start(void){
    /*
    Starts LWP system by converting calling thread into a LWP
    and lwp_yield() to whichever thread the scheduler choses. 
    */
    
}

void lwp_yield(void){
    /*
    Yields control to another LWP based on scheduler.
    Save's current LWP context, picks next one, restores that threads context, returns.
    */

}

void lwp_exit(int status){
    /*
    Terminates the current LWP and yields to whichever thread the scheduler chooses.
    */
}

tid_t lwp_wait(int *status){
    /*
    Waits for thread to terminate, deallocate resources, and reports temination status if non null 
    return tid of terminated thread or NO_THREAD
    */
}
    

tid_t lwp_gettid(void){
    /*
    returns the tid of the calling LWP or NO_THREAD if not calle by a LWP
    */
}

thread tid2thread(tid_t tid){
    /*
    returns teh thread corresponding to given threadID, else NULL
    */
}

void lwp_set_scheduler(scheduler sched){
    /*
    causes LWP package to use the given scheduler to choose next process to run
    transfers all threads from the old scheduler to the new one in next() order
    if scheduler is NULL the library should return to round-robin scheduling
    */
}

scheduler lwp_get_scheduler(void){
    /*
    returns pointer to current scheduler
    */
}

int main(){
    lwpfun fake;
    void *fakeP;
    tid_t bubba = lwp_create(fake, fakeP);
    return 0;
}