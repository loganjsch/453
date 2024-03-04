/* File created to hold code initially used to test the creation of the stack */
#include <stdio.h>
#include <stdlib.h>
#include "lwp.h"
#include <pthread.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <bits/mman.h>

int main()
{

    void *s;
    struct rlimit r;
    size_t stackSize = 0;
    long testVal = sysconf(_SC_PAGE_SIZE);
    int result = getrlimit(RLIMIT_STACK, &r);
    if (result == 0)
    {
        /* check if soft limit is RLIM_INFINITY */
        if (r.rlim_cur == RLIM_INFINITY)
        {
            stackSize = 8 * 1024 * 1024;
        }
        else
        {
            if ((r.rlim_cur % testVal) == 0)
            {
                stackSize = r.rlim_cur;
            }
            else
            {
                /* round up to nearest page size */
                stackSize = (r.rlim_cur + testVal) - (r.rlim_cur % testVal);
            }
        }
    }
    else
    {
        perror("GetRLimit");
        exit(-1);
    }

    printf("_SC_PAGE_SIZE: %ld \n", testVal);
    printf("RLIMIT_STACK: %ld\n", r.rlim_cur);

    printf("limit / pagesize = %ld\n", r.rlim_cur / testVal);
    printf("remainder %ld\n", r.rlim_cur % testVal);

    /* allocate space for lwp */
    /* MAP_ANONYMOUS & MAP_STACK part of <sys/mman.h> according to documentation... */
    s = mmap(NULL, stackSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    if (s == MAP_FAILED)
    {
        perror("mmap");
        exit(-1);
    }

    if (munmap(s, stackSize) == -1)
    {
        perror("munmap");
        exit(-1);
    }
}