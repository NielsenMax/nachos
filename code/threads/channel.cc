#include "channel.hh"

#include <cstring>
#include <stdio.h>


Channel::Channel(const char* debugName) {
    lockName = new char[strlen(debugName) + 14];
    sprintf(lockName, "ChannelLock::%s", debugName);
    lock = new Lock(lockName);

    list = new SynchList<ChannelItem*>();
}

Channel::~Channel() {
    delete lock;
    delete lockName;
    delete list;
}


void
Channel::Send(int number) {
    lock->Acquire();
    if (waiters > 0 && kindOfWaiters == receivers) {
        ChannelItem* tuple = list->Pop();
        std::get<0>(*tuple) = number;
        std::get<1>(*tuple)->V();
        lock->Release();
        return;
    }
    kindOfWaiters = senders;
    waiters++;
    ChannelItem* tuple = new ChannelItem();
    std::get<0>(*tuple) = number;
    Semaphore* sem = new Semaphore("senderSemaphore", 0);
    std::get<1>(*tuple) = sem;
    list->Append(tuple);
    lock->Release();
    sem->P();
    lock->Acquire();
    waiters--;
    lock->Release();
    delete sem;
    delete tuple;
}

void
Channel::Receive(int* number) {
    lock->Acquire();
    if (waiters > 0 && kindOfWaiters == senders) {
        ChannelItem* tuple = list->Pop();
        *number = std::get<0>(*tuple);
        std::get<1>(*tuple)->V();
        lock->Release();
        return;
    }
    kindOfWaiters = receivers;
    waiters++;
    ChannelItem* tuple = new ChannelItem();
    Semaphore* sem = new Semaphore("recieverSemaphore", 0);
    std::get<1>(*tuple) = sem;
    list->Append(tuple);
    lock->Release();
    sem->P();
    lock->Acquire();
    waiters--;
    *number = std::get<0>(*tuple);
    lock->Release();
    delete sem;
    delete tuple;
}