#include "channel.hh"

#include <cstring>
#include <stdio.h>


Channel::Channel(const char* debugName) {
    lockName = new char[strlen(debugName) + 15];
    sprintf(lockName, "ChannelLock::%s", debugName);
    lock = new Lock(lockName);

    semaphoreNameCommunicationAck = new char[strlen(debugName) + 15];
    sprintf(semaphoreNameCommunicationAck, "SemaphoreAck::%s", debugName);
    communicationAck = new Semaphore(semaphoreNameCommunicationAck, 0);

    semaphoreNameRecievers = new char[strlen(debugName) + 21];
    sprintf(semaphoreNameRecievers, "SemaphoreRecievers::%s", debugName);
    recievers = new Semaphore(semaphoreNameRecievers, 0);

    buffer = 0;
}

Channel::~Channel() {
    delete lock;
    delete lockName;
    delete recievers;
    delete semaphoreNameRecievers;
    delete communicationAck;
    delete semaphoreNameCommunicationAck;
}


void
Channel::Send(int msg) {
    lock->Acquire();
    buffer = msg;
    recievers->V();
    communicationAck->P();
    lock->Release();
}

void
Channel::Receive(int* buf) {
    recievers->P();
    *buf = buffer;
    communicationAck->V();    
}