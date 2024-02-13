/*
CSC 453 Project 2 Lightweight Processes

Kenneth Choi & Logan Schwarz

*/
#include <stdio.h>
#include "lwp.h"
#include <pthread.h>
#include <unistd.h>


tid_t lwp_create(lwpfun function, void *argument){
    /* 
    Creates a new lightweight pricess which
    executes the given function with the given argument
    returns thread id of new_thread, else NO_THREAD
    */
    long testVal = sysconf(_SC_PAGE_SIZE);
    printf("Pagesize in bytes: %ld \n", testVal);

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