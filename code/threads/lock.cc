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

#include "lock.hh"
#include <cstring>
#include <stdio.h>

/// Dummy functions -- so we can compile our later assignments.

Lock::Lock(const char *debugName)
{
    name = debugName;
    semName = new char[strlen(debugName) + 16];
    sprintf(semName, "LockSemaphore::%s", debugName);
    semaphore = new Semaphore(semName, 1);
}

Lock::~Lock()
{
    delete semaphore;
    delete semName;
}

const char *
Lock::GetName() const
{
    return name;
}

void Lock::Acquire()
{
    DEBUG('t', "ACQUIRING %s: The owner is %p and the current is %p\n", GetName(), owner, currentThread);
    ASSERT(!IsHeldByCurrentThread());

#ifdef LOCK_INVERSION_PRIORITY_SAFE
    int priority = currentThread->GetPriority();
    if (owner != nullptr && owner->GetPriority() < priority)
    {
        DEBUG('b', "The owner %s has lower priority than the current thread %s.\n", owner->GetName(), currentThread->GetName());
        scheduler->SwitchPriority(owner, priority);
    }
#endif

    semaphore->P();
    owner = currentThread;
}

void Lock::Release()
{
    DEBUG('t', "RELEASING %s: The owner is %p and the current is %p\n", GetName(), owner, currentThread);
    ASSERT(IsHeldByCurrentThread());
#ifdef LOCK_INVERSION_PRIORITY_SAFE
    currentThread->ResetPriority();
#endif
    owner = nullptr;
    semaphore->V();
}

bool Lock::IsHeldByCurrentThread() const
{
    DEBUG('t', "FUNCTION: The owner %p is current thread %p? %d\n", owner, currentThread, owner == currentThread);
    return owner == currentThread;
}
