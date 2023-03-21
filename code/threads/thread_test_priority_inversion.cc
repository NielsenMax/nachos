/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#include "thread_test_priority_inversion.hh"
#include "system.hh"

#include "lock.hh"

#include <stdio.h>
#include <string.h>

Lock *lock;

void High(void *args)
{
    lock->Acquire();
    lock->Release();

    printf("High priority task done.\n");
}

void Med(void *args)
{
    printf("Medium priority infinite loop...\n");
    while (1)
        currentThread->Yield();
}

void Low(void *args)
{
    lock->Acquire();
    currentThread->Yield();
    lock->Release();

    printf("Low priority task done.\n");
}

Thread *
GenerateThread(const char *threadName, int priority)
{
    char *name = new char[64];
    strncpy(name, threadName, 64);
    Thread *newThread = new Thread(name, false, priority);

    return newThread;
}

/// Set up the priority inversion problem.
///
///
void ThreadTestPriorityInversion()
{
    lock = new Lock("Lock");

    Thread *high = GenerateThread("High", MAX_PRIORITY);
    Thread *mid1 = GenerateThread("Mid1", 3);
    Thread *mid2 = GenerateThread("Mid2", 3);
    Thread *low = GenerateThread("Low", 0);

    low->Fork(Low, nullptr);
    currentThread->Yield();
    mid1->Fork(Med, nullptr);
    mid2->Fork(Med, nullptr);
    high->Fork(High, nullptr);
}
