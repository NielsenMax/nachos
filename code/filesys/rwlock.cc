#include "rwlock.hh"
#include "threads/semaphore.hh"
#include "threads/lock.hh"

#include <cstring>
#include <stdio.h>

RWLock::RWLock(char *name_){
    name = name_;

    nameReadersLock = new char[strlen(name) + 14];
    sprintf(nameReadersLock, "ReadersLock::%s", name);
    readersLock = new Lock(nameReadersLock);
    readers = 0;

    nameInUse = new char[strlen(name) + 8];
    sprintf(nameInUse, "InUse::%s", name);
    inUse = new Semaphore(nameInUse, 1);

    nameWantsToWrite = new char[strlen(name) + 15];
    sprintf(nameWantsToWrite, "wantsToWrite::%s", name);
    wantsToWrite = new Semaphore(nameWantsToWrite, 1);
}

RWLock::~RWLock(){
    delete readersLock;
    delete [] nameReadersLock;

    delete inUse;
    delete [] nameInUse;

    delete wantsToWrite;
    delete [] nameWantsToWrite;
}

void RWLock::RAcquire() {
    // Check for writers
    wantsToWrite->P();
    wantsToWrite->V();

    readersLock->Acquire();
    // The first reader must adquire the use lock
    if (readers == 0) {
        inUse->P();
    }
    readers++;
    readersLock->Release();
}

void RWLock::RRelease() {
    readersLock->Acquire();
    readers--;
    if (readers == 0) {
        inUse->V();
    }
    readersLock->Release();
}

void RWLock::Acquire(){
    wantsToWrite->P();
    inUse->P();
}

void RWLock::Release(){
    wantsToWrite->V();
    inUse->V();
}