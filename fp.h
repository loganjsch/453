hungrysnakes.c                                                                                      0000600 �    4�Z0023417 00000006165 14562612323 014601  0                                                                                                    ustar   kchoi21                         domain users                                                                                                                                                                                                           /*
 * snake:  This is a demonstration program to investigate the viability
 *         of a curses-based assignment.
 *
 * Author: Dr. Phillip Nico
 *         Department of Computer Science
 *         California Polytechnic State University
 *         One Grand Avenue.
 *         San Luis Obispo, CA  93407  USA
 *
 * Email:  pnico@csc.calpoly.edu
 *
 * Revision History:
 *         $Log: hungrysnakes.c,v $
 *         Revision 1.7  2023-01-28 14:36:33-08  pnico
 *         Summary: lwp_create() no longer takes a size
 *
 *         Revision 1.6  2023-01-28 14:27:44-08  pnico
 *         checkpointing as launched
 *
 *         Revision 1.5  2013-04-24 11:45:28-07  pnico
 *         pesky longlines.
 *
 *         Revision 1.4  2013-04-13 06:38:58-07  pnico
 *         doubled initial stack size
 *
 *         Revision 1.3  2013-04-10 11:22:27-07  pnico
 *         turned off the gdb sleep
 *
 *         Revision 1.2  2013-04-07 12:13:43-07  pnico
 *         Changed new_lwp() to lwp_create()
 *
 *         Revision 1.1  2013-04-02 16:39:24-07  pnico
 *         Initial revision
 *
 *         Revision 1.2  2004-04-13 12:31:50-07  pnico
 *         checkpointing with listener
 *
 *         Revision 1.1  2004-04-13 09:53:55-07  pnico
 *         Initial revision
 *
 *         Revision 1.1  2004-04-13 09:52:46-07  pnico
 *         Initial revision
 *
 */

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>
#include <signal.h>
#include <sys/time.h>
#include "snakes.h"
#include "lwp.h"
#include "util.h"

#define MAXSNAKES  100

int main(int argc, char *argv[]){
  int i,cnt,err;
  snake s[MAXSNAKES];

  err = 0;
  for (i=1;i<argc;i++) {                /* check options */
    fprintf(stderr,"%s: unknown option\n",argv[i]);
    err++;
  }
  if ( err ) {
    fprintf(stderr,"usage: %s\n",argv[0]);
    fprintf(stderr," SIGINT(^C) handler calls kill_snake().\n");
    fprintf(stderr," SIGQUIT(^\\) handler calls lwp_stop().\n");
    exit(err);
  }

  install_handler(SIGINT, SIGINT_handler);   /* SIGINT will kill a snake */
  install_handler(SIGQUIT,SIGQUIT_handler);  /* SIGQUIT will end lwp     */


  /* wait to gdb
  fprintf(stderr,"%d\n",getpid());
  sleep(5);
  */

  #ifdef FAST_SNAKES
  /*PLN*/set_snake_delay(1);
  #endif

  start_windowing();            /* start up curses windowing */

  /* Initialize Snakes */
  cnt = 0;
  /* snake new_snake(int y, int x, int len, int dir, int color) ;*/
  s[cnt++] = new_snake( 8,30,10, E,1);/* all start at color one */
  s[cnt++] = new_snake(10,30,10, E,1);
  s[cnt++] = new_snake(12,30,10, E,1);
  s[cnt++] = new_snake( 8,50,10, W,1);
  s[cnt++] = new_snake(10,50,10, W,1);
  s[cnt++] = new_snake(12,50,10, W,1);
  s[cnt++] = new_snake( 4,40,10, N,1);

  /* Draw each snake */
  draw_all_snakes();

  /* turn each snake loose as an individual LWP */
  for(i=0;i<cnt;i++) {
    s[i]->lw_pid = lwp_create((lwpfun)run_hungry_snake,(void*)(s+i));
  }

  lwp_start();                    

  for(i=0;i<cnt;i++)
    lwp_wait(NULL);

  end_windowing();              /* close down curses windowing */

  printf("Goodbye.\n");         /* Say goodbye, Gracie */
  return err;
}

                                                                                                                                                                                                                                                                                                                                                                                                           lwp.c                                                                                               0000600 �    4�Z0023417 00000021707 14571212223 012654  0                                                                                                    ustar   kchoi21                         domain users                                                                                                                                                                                                           /*
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
                                                         lwp.h                                                                                               0000600 �    4�Z0023417 00000006110 14563565276 012674  0                                                                                                    ustar   kchoi21                         domain users                                                                                                                                                                                                           #ifndef LWPH
#define LWPH
#include <sys/types.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define WORD_SIZE sizeof(unsigned long)

#if defined(__x86_64)
#include "fp.h"

typedef struct __attribute__ ((aligned(16))) __attribute__ ((packed))
registers {
  unsigned long rax;            /* the sixteen architecturally-visible regs. */
  unsigned long rbx;
  unsigned long rcx;
  unsigned long rdx;
  unsigned long rsi;
  unsigned long rdi;
  unsigned long rbp;
  unsigned long rsp;
  unsigned long r8;
  unsigned long r9;
  unsigned long r10;
  unsigned long r11;
  unsigned long r12;
  unsigned long r13;
  unsigned long r14;
  unsigned long r15;
  struct fxsave fxsave;   /* space to save floating point state */
} rfile;
#else
  #error "This only works on x86 for now"
#endif

typedef unsigned long tid_t;
#define NO_THREAD 0             /* an always invalid thread id */


typedef struct threadinfo_st *thread;
typedef struct threadinfo_st {
  tid_t         tid;            /* lightweight process id  */
  unsigned long *stack;         /* Base of allocated stack */
  size_t        stacksize;      /* Size of allocated stack */
  rfile         state;          /* saved registers         */
  unsigned int  status;         /* exited? exit status?    */
  thread        right;        /* Two pointers reserved   */
  thread        left;        /* for use by the library  */
  thread        next;      /* Two more for            */
  thread        prev;      /* schedulers to use       */
  thread        exited;         /* and one for lwp_wait()  */
} context;

typedef struct Node {
    thread data;
    struct Node* next;
} Node;

typedef int (*lwpfun)(void *);  /* type for lwp function */

/* Tuple that describes a scheduler */
typedef struct scheduler {
  void   (*init)(void);            /* initialize any structures     */
  void   (*shutdown)(void);        /* tear down any structures      */
  void   (*admit)(thread new);     /* add a thread to the pool      */
  void   (*remove)(thread victim); /* remove a thread from the pool */
  thread (*next)(void);            /* select a thread to schedule   */
  int    (*qlen)(void);            /* number of ready threads       */
} *scheduler;

/* rr functions */
void rr_init(void);
void rr_shutdown(void);
void rr_admit(thread new_thread);
void rr_remove_thread(thread victim);
thread rr_next(void);
int rr_qlen(void);

/* lwp functions */
extern tid_t lwp_create(lwpfun,void *);
extern void  lwp_exit(int status);
extern tid_t lwp_gettid(void);
extern void  lwp_yield(void);
extern void  lwp_start(void);
extern tid_t lwp_wait(int *);
extern void  lwp_set_scheduler(scheduler fun);
extern scheduler lwp_get_scheduler(void);
extern thread tid2thread(tid_t tid);

/* for lwp_wait */
#define TERMOFFSET        8
#define MKTERMSTAT(a,b)   ( (a)<<TERMOFFSET | ((b) & ((1<<TERMOFFSET)-1)) )
#define LWP_TERM          1
#define LWP_LIVE          0
#define LWPTERMINATED(s)  ( (((s)>>TERMOFFSET)&LWP_TERM) == LWP_TERM )
#define LWPTERMSTAT(s)    ( (s) & ((1<<TERMOFFSET)-1) )

/* prototypes for asm functions */
void swap_rfiles(rfile *old, rfile *new);

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                        numbersmain.c                                                                                       0000600 �    4�Z0023417 00000005401 14564027200 014364  0                                                                                                    ustar   kchoi21                         domain users                                                                                                                                                                                                           /*
 * snake:  This is a demonstration program to investigate the viability
 *         of a curses-based assignment.
 *
 * Author: Dr. Phillip Nico
 *         Department of Computer Science
 *         California Polytechnic State University
 *         One Grand Avenue.
 *         San Luis Obispo, CA  93407  USA
 *
 * Email:  pnico@csc.calpoly.edu
 *
 * Revision History:
 *         $Log: numbersmain.c,v $
 *         Revision 1.5  2023-01-28 14:35:46-08  pnico
 *         Summary: lwp_create() no longer takes a size
 *
 *         Revision 1.4  2023-01-28 14:27:44-08  pnico
 *         checkpointing as launched
 *
 *         Revision 1.3  2013-04-07 12:13:43-07  pnico
 *         Changed new_lwp() to lwp_create()
 *
 *         Revision 1.2  2013-04-02 17:04:17-07  pnico
 *         forgot to include the header
 *
 *         Revision 1.1  2013-04-02 16:39:24-07  pnico
 *         Initial revision
 *
 *         Revision 1.2  2004-04-13 12:31:50-07  pnico
 *         checkpointing with listener
 *
 *         Revision 1.1  2004-04-13 09:53:55-07  pnico
 *         Initial revision
 *
 *         Revision 1.1  2004-04-13 09:52:46-07  pnico
 *         Initial revision
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "lwp.h"
#include "schedulers.h"

#define MAXSNAKES  100

static void indentnum(void *num);

int main(int argc, char *argv[]){
  long i;

  printf("Launching LWPS\n");

  /* spawn a number of individual LWPs */
  /*
  for(i=1;i<=5;i++) {
    lwp_create((lwpfun)indentnum,(void*)i);
  }
  */

  for (i = 1; i <= 5; i++) {
    printf("Trying...");
    tid_t tid = lwp_create((lwpfun)indentnum, (void *)(uintptr_t)i);
    if (tid == (tid_t)-1) {
        // Handle the error case if lwp_create failed
        fprintf(stderr, "Failed to create LWP for iteration %d\n", i);
    } else {
        printf("Created LWP with thread ID: %lu\n", (unsigned long)tid);
    }
  }

  lwp_start();

  /* wait for the other LWPs */
  for(i=1;i<=5;i++) {
    int status,num;
    tid_t t;
    t = lwp_wait(&status);
    num = LWPTERMSTAT(status);
    printf("Thread %ld exited with status %d\n",t,num);
  }

  printf("Back from LWPS.\n");
  lwp_exit(0);
  return 0;
}

static void indentnum(void *num) {
  /* print the number num num times, indented by 5*num spaces
   * Not terribly interesting, but it is instructive.
   */
  long i;
  int howfar;

  howfar=(long)num;              /* interpret num as an integer */
  for(i=0;i<howfar;i++){
    printf("%*d\n",howfar*5,howfar);
    lwp_yield();                /* let another have a turn */
  }
  lwp_exit(i);                  /* bail when done.  This should
                                 * be unnecessary if the stack has
                                 * been properly prepared
                                 */
}

                                                                                                                                                                                                                                                               randomsnakes.c                                                                                      0000600 �    4�Z0023417 00000006040 14562612324 014536  0                                                                                                    ustar   kchoi21                         domain users                                                                                                                                                                                                           /*
 * snake:  This is a demonstration program to investigate the viability
 *         of a curses-based assignment.
 *
 * Author: Dr. Phillip Nico
 *         Department of Computer Science
 *         California Polytechnic State University
 *         One Grand Avenue.
 *         San Luis Obispo, CA  93407  USA
 *
 * Email:  pnico@csc.calpoly.edu
 *
 * Revision History:
 *         $Log: randomsnakes.c,v $
 *         Revision 1.5  2023-01-28 14:36:06-08  pnico
 *         Summary: lwp_create() no longer takes a size
 *
 *         Revision 1.4  2023-01-28 14:27:44-08  pnico
 *         checkpointing as launched
 *
 *         Revision 1.3  2013-04-13 10:15:44-07  pnico
 *         embiggened the stack
 *
 *         Revision 1.2  2013-04-07 12:13:43-07  pnico
 *         Changed new_lwp() to lwp_create()
 *
 *         Revision 1.1  2013-04-02 16:39:24-07  pnico
 *         Initial revision
 *
 *         Revision 1.2  2004-04-13 12:31:50-07  pnico
 *         checkpointing with listener
 *
 *         Revision 1.1  2004-04-13 09:53:55-07  pnico
 *         Initial revision
 *
 *         Revision 1.1  2004-04-13 09:52:46-07  pnico
 *         Initial revision
 *
 */

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>
#include <signal.h>
#include <sys/time.h>
#include "snakes.h"
#include "lwp.h"
#include "util.h"

#define MAXSNAKES  100

int main(int argc, char *argv[]){
  int i,cnt,err;
  snake s[MAXSNAKES];

  #ifdef DEBUG
  fprintf(stderr,"pid=%d\n",getpid());
  sleep(5);
  #endif

  err = 0;
  for (i=1;i<argc;i++) {                /* check options */
    fprintf(stderr,"%s: unknown option\n",argv[i]);
    err++;
  }
  if ( err ) {
    fprintf(stderr,"usage: %s\n",argv[0]);
    fprintf(stderr," SIGINT(^C) handler calls kill_snake().\n");
    fprintf(stderr," SIGQUIT(^\\) handler calls lwp_stop().\n");
    exit(err);
    exit(err);
  }

  install_handler(SIGINT, SIGINT_handler);   /* SIGINT will kill a snake */
  install_handler(SIGQUIT,SIGQUIT_handler);  /* SIGQUIT will end lwp     */

  /* wait to gdb  */
  if (getenv("WAITFORIT") ) {
    fprintf(stderr,"This is %d (hit enter to continue)\n",getpid());
    getchar();
  }

  start_windowing();            /* start up curses windowing */

  /* Initialize Snakes */
  cnt = 0;
  /* snake new_snake(int y, int x, int len, int dir, int color) ;*/

  s[cnt++] = new_snake( 8,30,10, E,1);/* each starts a different color */
  s[cnt++] = new_snake(10,30,10, E,2);
  s[cnt++] = new_snake(12,30,10, E,3);
  s[cnt++] = new_snake( 8,50,10, W,4);
  s[cnt++] = new_snake(10,50,10, W,5);
  s[cnt++] = new_snake(12,50,10, W,6);
  s[cnt++] = new_snake( 4,40,10, S,7);

  /* Draw each snake */
  draw_all_snakes();

  /* turn each snake loose as an individual LWP */
  for(i=0;i<cnt;i++) {
    s[i]->lw_pid = lwp_create((lwpfun)run_snake,(void*)(s+i));
  }

  lwp_start();                     

  for(i=0;i<cnt;i++)
    lwp_wait(NULL);

  end_windowing();              /* close down curses windowing */

  printf("Goodbye.\n");         /* Say goodbye, Gracie */
  return err;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                rr.c                                                                                                0000600 �    4�Z0023417 00000005556 14563565276 012525  0                                                                                                    ustar   kchoi21                         domain users                                                                                                                                                                                                           /* Round Robin Scheduler */

#include <stdio.h>
#include "lwp.h"
#include <assert.h>

Node* head = NULL;
Node* tail = NULL;

void rr_init(void){
    /* Initilializeds the scheduler */
    head = tail = NULL;
}

void rr_shutdown(void){
    /* Frees all nodes in the queue */
    while (head != NULL){
        Node* temp = head;
        head = head->next;
        free(temp);
    }
    tail = NULL;
}

void rr_admit(thread new_thread){
    /* adds new thread to end of queue */
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (new_node == NULL) {
        perror("Unable to allocate memory for new thread");
        return;
    }
    new_node->data = new_thread;
    new_node->next = NULL;

    if (tail == NULL) {
        head = tail = new_node;
    } else {
        tail->next = new_node;
        tail = new_node;
    }
}

void rr_remove_thread(thread victim){
    /* removes vicitm thread from the queue */
    Node* temp = head;
    Node* prev = NULL;

    while (temp != NULL && temp->data != victim) {
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL) return; /* Didn't find the victim */

    if (temp == head){
        head = head->next;
        if (head == NULL) tail = NULL;
    } else {
        prev->next = temp->next;
        if (temp == tail) tail = prev;
    }

    free(temp);

}

thread rr_next(){
    /* retrieves next thread and the moves it to the end of the queue*/
    if (head == NULL) return -1; /* Indicate that no thread is available */

    thread next_thread = head->data;
    /* Move the head to the end of the queue to achieve round-robin behavior */
    Node* temp = head;
    head = head->next;
    if (head == NULL) {
        tail = NULL;
    } else {
        tail->next = temp;
        tail = temp;
        tail->next = NULL;
    }

    return next_thread;
}

int rr_qlen(){
    /* returns num threads in queue */
    int length = 0;
    Node* current = head;
    while (current != NULL) {
        length++;
        current = current->next;
    }
    return length;
}

/*
void test_init_shutdown() {
    rr_init();
    assert(head == NULL && tail == NULL);
    rr_shutdown();
    assert(head == NULL && tail == NULL);
}

void test_admit_remove() {
    rr_init();
    rr_admit(1);
    rr_admit(2);
    assert(rr_qlen() == 2);
    rr_remove_thread(1);
    assert(rr_qlen() == 1);
    rr_remove_thread(2);
    assert(rr_qlen() == 0);
    rr_shutdown();
}

void test_next() {
    rr_init();
    rr_admit(1);
    rr_admit(2);
    assert(rr_next() == 1);
    assert(rr_next() == 2);
    assert(rr_next() == 1); // Should cycle back to the start
    rr_shutdown();
}

void test_qlen() {
    rr_init();
    assert(rr_qlen() == 0);
    rr_admit(1);
    rr_admit(2);
    assert(rr_qlen() == 2);
    rr_shutdown();
}

int main() {
    test_init_shutdown();
    test_admit_remove();
    test_next();
    test_qlen();

    printf("All tests passed.\n");
    return 0;
}
*/                                                                                                                                                  rr.h                                                                                                0000600 �    4�Z0023417 00000000105 14563317010 012470  0                                                                                                    ustar   kchoi21                         domain users                                                                                                                                                                                                           #ifndef RR_H
#define RR_H
#include "lwp.h"


/* Functions */

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                           schedulers.h                                                                                        0000600 �    4�Z0023417 00000000311 14563552315 014216  0                                                                                                    ustar   kchoi21                         domain users                                                                                                                                                                                                           #ifndef SCHEDULERSH
#define SCHEDULERSH

#include "lwp.h"
extern scheduler AlwaysZero;
extern scheduler ChangeOnSIGTSTP;
extern scheduler ChooseHighestColor;
extern scheduler ChooseLowestColor;
#endif
                                                                                                                                                                                                                                                                                                                       util.h                                                                                              0000600 �    4�Z0023417 00000000640 14562612324 013033  0                                                                                                    ustar   kchoi21                         domain users                                                                                                                                                                                                           #ifndef UTILH
#define UTILH

typedef void (*sigfun)(int signum);

extern void SIGINT_handler(int num);
extern void SIGQUIT_handler(int num);
extern void SIGTSTP_handler(int num);
extern void install_handler(int sig, sigfun fun);

#ifdef OLDSCHEDULERS
extern int AlwaysZero();
extern int ChangeOnSIGTSTP();
extern int ChooseLowestColor();
extern int ChooseHighestColor();
#else
#include "schedulers.h"
#endif

#endif
                                                                                                util.c                                                                                              0000600 �    4�Z0023417 00000006227 14562612324 013035  0                                                                                                    ustar   kchoi21                         domain users                                                                                                                                                                                                           /*
 * util:   Some utility routines (schedulers and signal handlers)
 *         for the snake demos
 *
 * Author: Dr. Phillip Nico
 *         Department of Computer Science
 *         California Polytechnic State University
 *         One Grand Avenue.
 *         San Luis Obispo, CA  93407  USA
 *
 * Email:  pnico@csc.calpoly.edu
 *
 * Revision History:
 *         $Log: util.c,v $
 *         Revision 1.2  2023-01-28 14:27:44-08  pnico
 *         checkpointing as launched
 *
 *         Revision 1.1  2013-04-02 16:39:24-07  pnico
 *         Initial revision
 *
 *
 */

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "snakes.h"
#include "lwp.h"
#include "util.h"


#ifdef OLDSCHEDULERS
/********************************************************
 * This section contains potential scheduling algorithms
 ********************************************************/
int OLD_AlwaysZero() {
  /* always run the first one */
  return 0;
}

int SIGTSTPcounter = 0;
int OLD_ChangeOnSIGTSTP() {
  /* Move to the next one, counting TSTPs*/
  return SIGTSTPcounter%lwp_procs;
}

int OLD_ChooseLowestColor(){
  /* choose the lowest available color.  Round-robin within
   * colors.  (I.e. Always advance)
   * We'll do the round-robin within the snake array
   */
  int i,color,next;
  snake s;

  color = MAX_VISIBLE_SNAKE+1;
  for(i=0;i<lwp_procs;i++) {
    s = snakeFromLWpid(lwp_ptable[i].pid);
    if ( s && s->color < color )
      color = s->color;
  }
  next = -1;
  if ( color ){                 /* find the next one of the given color */
    next = (lwp_running+1)%lwp_procs;
    s = snakeFromLWpid(lwp_ptable[next].pid);
    while(s && (s->color != color) ) {
      next = (next+1)%lwp_procs;
      s = snakeFromLWpid(lwp_ptable[next].pid);
    }
  }

  return next;
}

int OLD_ChooseHighestColor(){
  /* choose the highest available color.  Round-robin within
   * colors.  (I.e. Always advance)
   * We'll do the round-robin within the snake array
   */
  int i,color,next;
  snake s;

  color = 0;
  for(i=0;i<lwp_procs;i++) {
    s = snakeFromLWpid(lwp_ptable[i].pid);
    if ( s && s->color > color )
      color = s->color;
  }
  next = -1;
  if ( color ){                 /* find the next one of the given color */
    next = (lwp_running+1)%lwp_procs;
    s = snakeFromLWpid(lwp_ptable[next].pid);
    while(s && (s->color != color) ) {
      next = (next+1)%lwp_procs;
      s = snakeFromLWpid(lwp_ptable[next].pid);
    }
  }

  return next;
}


/********************************************************
 * End scheduling algorithms
 * Now the signal handling material
 ********************************************************/

void SIGTSTP_handler(int num){
  SIGTSTPcounter++;              /* increment the counter */
}
#endif

void SIGINT_handler(int num){
  kill_snake();                 /* mark a snake for death */
}

void SIGQUIT_handler(int num){
}

void install_handler(int sig, sigfun fun){
  /* use sigaction to install a signal handler */
  struct sigaction sa;

  sa.sa_handler = fun;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;


  if ( sigaction(sig,&sa,NULL) < 0 ) {
    perror("sigaction");
    exit(-1);
  }
}

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         