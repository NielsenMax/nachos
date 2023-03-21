#include "channel.hh"

#include <cstring>
#include <stdio.h>


Channel::Channel(const char* debugName) {
    lockName = new char[strlen(debugName) + 14];
    sprintf(lockName, "ChannelLock::%s", debugName);
    lock = new Lock(lockName);

    conditionNameEmptyBuffer = new char[strlen(debugName) + 16];
    sprintf(conditionNameEmptyBuffer, "ConditionEmpty::%s", debugName);
    emptyBuffer = new Condition(conditionNameEmptyBuffer, lock);

    conditionNameFullBuffer = new char[strlen(debugName) + 15];
    sprintf(conditionNameFullBuffer, "ConditionFull::%s", debugName);
    fullBuffer = new Condition(conditionNameFullBuffer, lock);

    conditionNameCommunicationAck = new char[strlen(debugName) + 14];
    sprintf(conditionNameCommunicationAck, "ConditionAck::%s", debugName);
    communicationAck = new Condition(conditionNameCommunicationAck, lock);

    buffer = NULL;
}

Channel::~Channel() {
    delete lock;
    delete conditionNameCommunicationAck;
    delete conditionNameEmptyBuffer;
    delete conditionNameFullBuffer;
    delete communicationAck;
    delete fullBuffer;
    delete emptyBuffer;
    delete lockName;
}


void
Channel::Send(int msg) {
    lock->Acquire();

    while(buffer != nullptr) {
        emptyBuffer->Wait();
    } 

    buffer = &msg;
    fullBuffer->Signal();
    communicationAck->Wait();
    emptyBuffer->Signal();

    lock->Release();
}

void
Channel::Receive(int* buf) {
    lock->Acquire();

    while (buffer == nullptr)
    {
        fullBuffer->Wait();
    }
    *buf = *buffer;
    buffer = nullptr;
    communicationAck->Signal();

    lock->Release();
}