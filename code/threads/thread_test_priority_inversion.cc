/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#include "thread_test_simple.hh"
#include "system.hh"

#include "lock.hh"

#include <stdio.h>
#include <string.h>

class ThreadState
{
public:
    ThreadState(char *name_, Lock *lock_)
    {
        name = name_;
        lock = lock_;
    };
    ~ThreadState()
    {
        delete name;
        delete lock;
    };
    char *GetName() { return name; };
    Semaphore *GetLock() { return lock; };

private:
    char *name;
    Semaphore *semaphore;
};

void SimpleThread(void *args)
{
    char *name = (char *)(((void **)args)[0]);
    Lock *lock = (Lock *)(((void **)args)[1]);

    DEBUG('b', "Thread %s is aquiring lock", name);
    lock->Acquire();
}

Thread *
GenerateThread(const char *threadName, Semaphore *sem, int priority = 0)
{
    char *name = new char[64];
    strncpy(name, threadName, 64);
    Thread *newThread = new Thread(name, true, priority);
    newThread->Fork(SimpleThread, (void *)state);

    return newThread;
}

/// Set up a ping-pong between several threads.
///
/// Do it by launching one thread which calls `SimpleThread`, and finally
/// calling `SimpleThread` on the current thread.

void
ThreadTestPriorityInversion()
{
    Lock* lock = new Lock("ThreadTestPriorityInversion::Lock");
    void *args[NUM_TURNSTILES][3];
    Thread *threads[NUM_TURNSTILES];

    // Launch a new thread for each turnstile.
    char *name = new char[64];
    strncpy(name, threadName, 64);
    Thread *newThread = new Thread(name, true, priority);
    newThread->Fork(SimpleThread, (void *)state);

    // Wait until all turnstile threads finish their work.  `Thread::Join` is
    // not implemented at the beginning, therefore an ad-hoc workaround is
    // applied here.
    for (unsigned i = 0; i < NUM_TURNSTILES; i++) {
        threads[i]->Join();
    }
    printf("All turnstiles finished. Final count is %u (should be %u).\n",
           count, ITERATIONS_PER_TURNSTILE * NUM_TURNSTILES);
}
