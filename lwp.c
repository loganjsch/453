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

rfile iRegisters; /* Initial registers */

tid_t count = 1;
thread lwps = NULL;
thread curr = NULL;

typedef struct Queue {
    struct Node *front, *rear;
} Queue;

struct Queue *ready;
struct Queue *waiting; 
struct Queue *exited;
/* 
    Now calling a function pointer is derefrencing it and applying to arg
    thread nxt;
    nxt = RoundRobin->next();
*/

/* create a new node */
struct Node* newNode(thread data){
    struct Node* temp = (struct Node*)malloc(sizeof(struct Node));
    temp->data = data;
    temp->next = NULL;
    return temp;
}

/* create a new queue */
struct Queue* createQueue(){
    struct Queue* q = (struct Queue*)malloc(sizeof(struct Queue));
    q->front = q->rear = NULL;
    return q;
}

/* add a new node to the queue */
void enqueue(struct Queue* q, thread data){
    struct Node* temp = newNode(data);
    if (q->rear == NULL){
        q->front = q->rear = temp;
        return;
    }
    q->rear->next = temp;
    q->rear = temp;
}

/* remove head */
Node* dequeue(struct Queue* q){
    if (q->front == NULL){
        return NULL;
    }
    struct Node* temp = q->front;
    q->front = q->front->next;
    if (q->front == NULL){
        q->rear = NULL;
    }

    return temp;
}


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

   /* initialize ready waiting and terminated */
    if (ready == NULL){
        ready = createQueue();
    }
    if (waiting == NULL){
        waiting = createQueue();
    }
    if (exited == NULL){
        exited = createQueue();
    }

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
    thislwp->stacksize = stackSize * WORD_SIZE;

    /* pointer to base of stack */
    thislwp->stack = (unsigned long *)malloc(stackSize * WORD_SIZE);

    //size_t totalStackSize = stackSize * WORD_SIZE;
    //thislwp->stack = (unsigned long *)mmap(NULL, stackSize * WORD_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);

    
    /* allocate space for lwp */
    /* MAP_ANONYMOUS & MAP_STACK part of <sys/mman.h> according to documentation... 
    stack = mmap(NULL, stackSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    if (stack == MAP_FAILED){
        perror("mmap");
        exit(-1);
    }
    if (munmap(stack, stackSize) == -1){
        perror("munmap");
        exit(-1);
    }*/

    
    /*thislwp->stack = stack; */
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

    /* now schedule it and add to ready */
    RoundRobin->admit(thislwp);
    enqueue(ready, thislwp);
    return thislwp->tid;
}

void lwp_start(void){
    /*
    Starts LWP system by converting calling thread into a LWP
    and lwp_yield() to whichever thread the scheduler choses. 
    */
    // curr = RoundRobin->next(); 
    // /* save original context and switch to thread context */
    //  swap_rfiles(&iRegisters, &curr->state);
    // return; 
    thread callingThread = malloc(sizeof(context));
    callingThread->tid = count++;
    callingThread->stack = NULL;
    callingThread->stacksize = 0;
    callingThread->state = iRegisters;
    curr = callingThread;
    RoundRobin->admit(callingThread);

}

void lwp_yield(void){
    /*
    Yields control to another LWP based on scheduler.
    Save's current LWP context, picks next one, restores that threads context, returns.
    */
    thread temp;
    temp = curr;
    curr = RoundRobin->next();
    //thread next = RoundRobin->next();
    // if (next != NULL){
    //     /* save original context and switch to thread context */
    //     swap_rfiles(&curr->state, &next->state);
    //     return;
    // }
    // else{
    //     swap_rfiles(NULL, &iRegisters);
    //     return;
    // }
    swap_rfiles(&temp->state, &curr->state);

}

void lwp_exit(int status){
    /*
    Terminates the current LWP and yields to whichever thread the scheduler chooses.
    */
    /* remove from ready and add to exited */
    // thread temp = dequeue(ready)->data;
    // temp->status = status;
    // enqueue(exited, temp);
    // lwp_yield();

    if (curr) {
        RoundRobin->remove(curr);
        free(curr->stack);
        free(curr);
        curr = RoundRobin->next();
    }

    return;
}



tid_t lwp_wait(int *status){
    /*
    Waits for thread to terminate, deallocate resources, and reports temination status if non null 
    return tid of terminated thread or NO_THREAD
    */
    /* if there are no terminated threads, caller of wait will be blocked
        deschedule and place on waiting queue */
    if (exited->front == NULL){
        /* if no more threads to potentially block, then return NO_THREAD */
        if (ready->front == NULL){
            return NO_THREAD;
        }
        thread temp = dequeue(ready)->data;
        *status = MKTERMSTAT(*status, 0xFF);
        enqueue(waiting, temp);
        lwp_yield();
        RoundRobin->remove(temp);
        return temp->tid;
    }
    else{
        /* if a thread calls lwp_exit() then remove from waiting queue and reschedule with scheduler->admit()*/
        thread temp = dequeue(exited)->data;
        *status = temp->status;
        RoundRobin->admit(temp);
        enqueue(ready, temp);
        return temp->tid;
    }
}
    

tid_t lwp_gettid(void){
    /*
    returns the tid of the calling LWP or NO_THREAD if not calle by a LWP
    */
    if (!curr){
        return NO_THREAD;
    }
    return curr->tid;
}

thread tid2thread(tid_t tid){
    /*
    returns teh thread corresponding to given threadID, else NULL
    */
    thread searchthread;

    searchthread = curr;

    while (searchthread){
        if (searchthread->tid == tid){
            return searchthread;
        }
        searchthread = searchthread->next;
    }

    return NULL;
}

void lwp_set_scheduler(scheduler sched){
    /*
    causes LWP package to use the given scheduler to choose next process to run
    transfers all threads from the old scheduler to the new one in next() order
    if scheduler is NULL the library should return to round-robin scheduling
    */
    // Pointer to hold the old scheduler.
    scheduler oldSched = RoundRobin;

    // Check if the new scheduler is NULL, and set to default round-robin if it is.
    if (sched == NULL) {
        sched = RoundRobin;  // Assume round_robin_scheduler is your default scheduler
    }

    // Set the current scheduler to the new scheduler.
    RoundRobin = sched;

    // Transfer all threads from the old scheduler to the new one.
    if (oldSched != NULL) {
        thread tempThread;
        while ((tempThread = oldSched->next()) != NULL) {
            oldSched->remove(tempThread);  // Remove thread from old scheduler
            sched->admit(tempThread);   // Admit thread to new scheduler
        }
    }
   

}

scheduler lwp_get_scheduler(void){
    /*
    returns pointer to current scheduler
    */
    return RoundRobin;
}

/*
int main(){
    lwpfun fake;
    void *fakeP;
    tid_t bubba = lwp_create(fake, fakeP);
    return 0;
}
*/
