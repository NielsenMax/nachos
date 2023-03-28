/// Routines for synchronizing threads.
///
/// The implementation for this primitive does not come with base Nachos.
/// It is left to the student.
///
/// When implementing this module, keep in mind that any implementation of a
/// synchronization routine needs some primitive atomic operation.  The
/// semaphore implementation, for example, disables interrupts in order to
/// achieve this; another way could be leveraging an already existing
/// primitive.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "condition.hh"
#include <cstring>
#include <stdio.h>

/// Dummy functions -- so we can compile our later assignments.
///
/// Note -- without a correct implementation of `Condition::Wait`, the test
/// case in the network assignment will not work!

Condition::Condition(const char *debugName, Lock *conditionLock_)
{
    name = debugName;
    semName = new char[strlen(debugName) + 21];
    sprintf(semName, "ConditionSemaphore::%s", debugName);
    signal = new Semaphore(semName, 0); 

    conditionLock = conditionLock_;

    lockName = new char[strlen(debugName) + 16];
    sprintf(lockName, "ConditionLock::%s", debugName);
    waitingLock = new Lock(lockName); 
}

Condition::~Condition()
{
    delete signal;
    delete semName;
    delete waitingLock;
    delete lockName;
}

const char *
Condition::GetName() const
{
    return name;
}

void
Condition::Wait()
{
    ASSERT(conditionLock->IsHeldByCurrentThread());

    waitingLock->Acquire();
    waiting++;
    waitingLock->Release();

    conditionLock->Release();
    signal->P();
    conditionLock->Acquire();
}

void
Condition::Signal()
{
    waitingLock->Acquire();
    if(waiting > 0) {
        waiting--;
        signal->V();
    }
    waitingLock->Release();
}

void
Condition::Broadcast()
{
    waitingLock->Acquire();
    for(;waiting > 0;waiting--) {
        signal->V();
    }
    waitingLock->Release();
}
