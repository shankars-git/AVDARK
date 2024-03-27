/**
 * Gauss-Seidel implementation using pthreads.
 *
 *
 * Course: Advanced Computer Architecture, Uppsala University
 * Course Part: Lab assignment 3
 *
 * Original author: Frédéric Haziza <daz@it.uu.se>
 * Heavily modified by: Andreas Sandberg <andreas.sandberg@it.uu.se>
 *
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdatomic.h>

#include "gs_interface.h"

/**
 * Tell the startup code that we want run in parallel mode.
 */
const int gsi_is_parallel = 1;

/**
 * Thread data structure passed to the thread entry function.
 */
typedef struct {
        int thread_id;
        pthread_t thread;

        float error;

        _Atomic int progress;

} thread_info_t;

/** Define to enable debug mode */
#define DEBUG 1 /* 1 */

/** Debug output macro. Only active when DEBUG is non-0 */
#define dprintf(...)                            \
        if (DEBUG)                              \
                fprintf(stderr, __VA_ARGS__)

/** Vector with information about all active threads */
thread_info_t *threads = NULL;

/** The global error for the last iteration */
// static _Atomic double global_error;
static double global_error;

pthread_barrier_t mybarrier;

void
gsi_init()
{
        gs_verbose_printf("\t****  Initializing the  environment ****\n");

        threads = (thread_info_t *)malloc(gs_nthreads * sizeof(thread_info_t));
        if (!threads) {
                fprintf(stderr,
                        "Failed to allocate memory for thread information.\n");
                exit(EXIT_FAILURE);
        }

        /* Initialize global_error to something larger than the
         * tolerance to get the algorithm started */
        global_error = gs_tolerance + 1;

        /* TASK: Initialize global variables here */
        pthread_barrier_init(&mybarrier, NULL, gs_nthreads);
}

void
gsi_finish()
{
        gs_verbose_printf("\t****  Cleaning environment ****\n");

        /* TASK: Be nice and cleanup the stuff you initialized in
         * gsi_init()
         */
        pthread_barrier_destroy(&mybarrier);

        if (threads)
                free(threads);
}

static void
thread_sweep(int tid, int iter, int lbound, int rbound)
{
        // threads[tid].error = 0.0;
        float local_error = 0.0;

        for (int row = 1; row < gs_size - 1; row++) {
                dprintf("%d: checking wait condition\n"
                        "\titeration: %i, row: %i\n",
                        tid,
                        iter, row);

                if (tid > 0) {
                        while (atomic_load(&threads[tid - 1].progress) < row) {
                                continue;
                        }
                }

                dprintf("%d: Starting on row: %d\n", tid, row);
                dprintf("%d: Listed row: %d\n", tid, threads[tid].progress);

                /* Update this thread's part of the matrix */
                for (int col = lbound; col < rbound; col++) {
                        double new_value = 0.25 * (
                                gs_matrix[GS_INDEX(row + 1, col)] +
                                gs_matrix[GS_INDEX(row - 1, col)] +
                                gs_matrix[GS_INDEX(row, col + 1)] +
                                gs_matrix[GS_INDEX(row, col - 1)]);
                        // threads[tid].error +=
                        local_error +=
                                fabs(gs_matrix[GS_INDEX(row, col)] - new_value);
                        gs_matrix[GS_INDEX(row, col)] = new_value;
                }

                atomic_store(&(threads[tid].progress), row);

                dprintf("%d: row %d done\n", tid, row);
        }
        threads[tid].error = local_error;

}

/**
 * Computing routine for each thread
 */
static void *
thread_compute(void *_self)
{
        thread_info_t *self = (thread_info_t *)_self;
        const int tid = self->thread_id;

        int lbound = 0, rbound = 0;

        /* TASK: Compute bounds for this thread */ //DONE
        int region_width = gs_size / gs_nthreads;
        lbound = tid * region_width;
        rbound = lbound + region_width - 1;

        // if (lbound == 0) {lbound++;}

        printf("tid %d: lbound = %d, rbound = %d\n", tid, lbound, rbound);

        gs_verbose_printf("%i: lbound: %i, rbound: %i\n",
                          tid, lbound, rbound);

        for (int iter = 0;
             iter < gs_iterations && global_error > gs_tolerance;
             iter++) {
                dprintf("%i: Starting iteration %i\n", tid, iter);

                thread_sweep(tid, iter, lbound, rbound);

                /* TASK: Update global error */
                /* Note: The reduction should only be done by one
                 * thread after all threads have updated their local
                 * errors */
                /* Hint: Which thread is guaranteed to complete its
                 * sweep last? */

                dprintf("%d: iteration %d done\n", tid, iter);

                /* TASK: Iteration barrier */
                pthread_barrier_wait(&mybarrier);
                
                if (tid == gs_nthreads - 1) {
                        global_error = 0.0;
                        for (int i = 0; i < gs_nthreads; ++i) {
                                global_error += threads[tid].error;
                        }
                }
        }

        gs_verbose_printf(
                "\t****  Thread %d done after %d iterations ****\n",
                tid, gs_iterations);

        return NULL;
}

/**
 * Parallel implementation of the GS algorithm. Called from
 * gs_common.c to start the solver.
 */
void
gsi_calculate()
{
        int err;

        for (int t = 0; t < gs_nthreads; t++) {
                gs_verbose_printf("\tSpawning thread %d\n",t);

                threads[t].thread_id = t;
                threads[t].progress = 0;
                err = pthread_create(&threads[t].thread, NULL,
                                     thread_compute, &threads[t]);
                if (err) {
                        fprintf(stderr,
                                "Error: pthread_create() failed: %d, "
                                "thread %d\n",
                                err, t);
                        exit(EXIT_FAILURE);
                }
        }
  
        /* Calling pthread_join on a thread will block until the
         * thread terminates. Since we are joining all threads, we
         * wait until all threads have exited. */
        for (int t = 0; t < gs_nthreads; t++) {
                err = pthread_join(threads[t].thread, NULL);
                if (err) {
                        fprintf(stderr,
                                "Error: pthread_join() failed: %d, "
                                "thread %d\n",
                                err, t);
                        exit(EXIT_FAILURE);
                }
        }

        if (global_error <= gs_tolerance) {
                printf("Solution converged!\n");
        } else {
                printf("Reached maximum number of iterations. Solution did "
                       "NOT converge.\n");
                printf("Note: This is normal if you are using the "
                       "default settings.\n");
        }
        printf("PTH global error: %f\n", global_error);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * indent-tabs-mode: nil
 * c-file-style: "linux"
 * End:
 */
