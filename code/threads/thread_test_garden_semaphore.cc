/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_garden_semaphore.hh"
#include "system.hh"
#include "semaphore.hh"

#include <stdio.h>


static const unsigned NUM_TURNSTILES = 2;
static const unsigned ITERATIONS_PER_TURNSTILE = 50;
static bool done[NUM_TURNSTILES];
static int count;

static void
Turnstile(void *args)
{
    unsigned *n = (unsigned *)(((void **)args)[0]);
    Semaphore *semaphore = (Semaphore *)(((void **)args)[1]);


    for (unsigned i = 0; i < ITERATIONS_PER_TURNSTILE; i++) {
        semaphore->P();
        int temp = count;
        currentThread->Yield();
        count = temp + 1;
        semaphore->V();
    }
    printf("Turnstile %u finished. Count is now %u.\n", *n, count);
    done[*n] = true;
    delete n;
}

void
ThreadTestGardenSemaphore()
{
    Semaphore* sem = new Semaphore("semaphore_garden_semaphore", 1);
    void *args[NUM_TURNSTILES][3];
    Thread *threads[NUM_TURNSTILES];

    // Launch a new thread for each turnstile.
    for (unsigned i = 0; i < NUM_TURNSTILES; i++) {
        printf("Launching turnstile %u.\n", i);
        char *name = new char [16];
        sprintf(name, "Turnstile %u", i);
        unsigned *n = new unsigned;
        *n = i;

        args[i][0] = n;
        args[i][1] = sem;

        Thread *t = new Thread(name);
        threads[i] = t;
        t->Fork(Turnstile, (void *) args[i]);
    }

    // Wait until all turnstile threads finish their work.  `Thread::Join` is
    // not implemented at the beginning, therefore an ad-hoc workaround is
    // applied here.
    for (unsigned i = 0; i < NUM_TURNSTILES; i++) {
        threads[i]->Join();
    }
    printf("All turnstiles finished. Final count is %u (should be %u).\n",
           count, ITERATIONS_PER_TURNSTILE * NUM_TURNSTILES);
}
