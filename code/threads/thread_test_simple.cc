/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#include "thread_test_simple.hh"
#include "system.hh"

#include "semaphore.hh"

#include <stdio.h>
#include <string.h>

// #define SEMAPHORE_TEST 1

class ThreadState
{
public:
    ThreadState(char *name_, Semaphore *sem)
    {
        name = name_;
        semaphore = sem;
    };
    ~ThreadState()
    {
        delete name;
        delete semaphore;
    };
    char *GetName() { return name; };
    Semaphore *GetSemaphore() { return semaphore; };

private:
    char *name;
    Semaphore *semaphore;
};

/// Loop 10 times, yielding the CPU to another ready thread each iteration.
///
/// * `name` points to a string with a thread name, just for debugging
///   purposes.
void SimpleThread(void *args)
{
    ThreadState *state = (ThreadState *)args;

    char *name = state->GetName();

#ifdef SEMAPHORE_TEST
    Semaphore *sem = state->GetSemaphore();
    sem->P();
    DEBUG('t', "Thread %s is calling P on semaphore %s\n", name, sem->GetName());
#endif

    // If the lines dealing with interrupts are commented, the code will
    // behave incorrectly, because printf execution may cause race
    // conditions.
    for (unsigned num = 0; num < 10; num++)
    {
        printf("*** Thread `%s` is running: iteration %u\n", name, num);
        currentThread->Yield();
    }
#ifdef SEMAPHORE_TEST
    sem->V();
    DEBUG('t', "Thread %s is calling V on semaphore %s\n", name, sem->GetName());
#endif
    printf("!!! Thread `%s` has finished\n", name);
}

Thread *
GenerateThread(const char *threadName, Semaphore *sem, int priority = 0)
{
    char *name = new char[64];
    strncpy(name, threadName, 64);
    Thread *newThread = new Thread(name, true, priority);
    ThreadState *state = new ThreadState(name, sem);
    newThread->Fork(SimpleThread, (void *)state);

    return newThread;
}

/// Set up a ping-pong between several threads.
///
/// Do it by launching one thread which calls `SimpleThread`, and finally
/// calling `SimpleThread` on the current thread.
void ThreadTestSimple()
{
    Semaphore *sem = new Semaphore("semaphore_simple_test", 3);
    Thread *fst = GenerateThread("2dn", sem, 10);
    Thread *snd = GenerateThread("3rd", sem);
    Thread *thd = GenerateThread("4th", sem);
    Thread *fht = GenerateThread("5th", sem);

    char *name = new char[64];
    strncpy(name, "1st", 64);
    ThreadState *state = new ThreadState(name, sem);

    SimpleThread((void *)state);
    fst->Join();
    snd->Join();
    thd->Join();
    fht->Join();
    delete fst;
    delete snd;
    delete thd;
    delete fht;
}
