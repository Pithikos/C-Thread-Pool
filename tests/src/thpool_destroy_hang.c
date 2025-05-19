/*
 * Reproduces the “hang inside thpool_destroy()” bug.
 *   – Fails (SIGALRM) with the original binary-semaphore implementation.
 *   – Succeeds instantly once bsem_wait / bsem_post_all are patched.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include "../../thpool.h"

#define TIMEOUT 1

/* -------------------------------------------------------------- */
/* 2. SIGALRM handler: triggers when thpool_destroy() does not    */
/*    return within 1 sec, indicating the bug is reproduced.      */
/* -------------------------------------------------------------- */
static void timeout_handler(int sig)
{
    (void)sig;
    fprintf(stderr,
            "FAIL: thpool_destroy() did not finish within %d s "
            "(bug reproduced)\n", TIMEOUT);
    _exit(EXIT_FAILURE);
}

int main(void)
{
    const int THREADS = 4000;

    /* Watchdog: if we are still alive after 3 s, the bug is present */
    signal(SIGALRM, timeout_handler);
    alarm(TIMEOUT);

    printf("Creating pool with %d threads …\n", THREADS);
    threadpool tp = thpool_init(THREADS);

    struct timeval t0, t1;
    gettimeofday(&t0, NULL);

    printf("Calling thpool_destroy() …\n");
    thpool_destroy(tp);      /* ← hangs here when not patched */

    gettimeofday(&t1, NULL);
    alarm(0); /* disarm watchdog */

    double elapsed =
        (t1.tv_sec - t0.tv_sec) + (t1.tv_usec - t0.tv_usec) / 1e6;

    printf("PASS: thpool_destroy() returned in %.3f s (patch OK)\n",
           elapsed);
    return 0;
}
