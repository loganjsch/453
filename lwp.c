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
#include "rr.c"

struct scheduler rr_publish = {NULL, NULL, rr_admit, rr_remove_thread, rr_next, rr_qlen};
scheduler RoundRobin = &rr_publish;

rfile icontext; /* Initial Context */

tid_t count = 0;
thread lwps = NULL;
thread curr = NULL;
/* 
    Now calling a function pointer is derefrencing it and applying to arg
    thread nxt;
    nxt = RoundRobin->next();
*/

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

    /* Initialize our lwp thread struct going no where */
    thread thislwp;
    thislwp = malloc(sizeof(context));
    thislwp->right = NULL;
    thislwp->left = NULL;
    thislwp->next = NULL;
    thislwp->prev = NULL;

    unsigned long *stack; 
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

    /* Assuming this gets us the stack size in bytes? */
    thislwp->stacksize = stackSize;

    printf("_SC_PAGE_SIZE: %ld \n", testVal);
    printf("RLIMIT_STACK: %ld\n", r.rlim_cur);

    printf("limit / pagesize = %ld\n", r.rlim_cur/testVal);
    printf("remainder %ld\n", r.rlim_cur % testVal);

    /* allocate space for lwp */
    /* MAP_ANONYMOUS & MAP_STACK part of <sys/mman.h> according to documentation... */
    stack = mmap(NULL, stackSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    if (stack == MAP_FAILED){
        perror("mmap");
        exit(-1);
    }

    /*
    if (munmap(stack, stackSize) == -1){
        perror("munmap");
        exit(-1);
    }
    */
    
    thislwp->stack = stack;
    thislwp->tid = count++;

    /* once we we set thislwp.stack pointer to base of stack  */
    /* now set to highest on stack because it's supposed to grow downwards */
    stack = (tid_t *)((tid_t)thislwp->stack + (tid_t)thislwp->stacksize - 1);

    /* now we start initializing the stack downwards */
    stack[0] = (unsigned long)lwp_exit;
    stack[-1] = (unsigned long)function;
    stack[-2] = (unsigned long)stack;

    /* Need to initialize registers */
    thislwp->state.rdi = (uintptr_t)argument;
    thislwp->state.rbp = (stack - 2);
    thislwp->state.fxsave = FPU_INIT; /* space to save floating point state */

    /* here we need to add it to lwp.c's list to track it */

    /* now schedule it */
    RoundRobin->admit(thislwp);

    return thislwp->tid;
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