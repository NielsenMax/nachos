#ifndef NACHOS_THREADS_CHANNEL__HH
#define NACHOS_THREADS_CHANNEL__HH

#include "lock.hh"
#include "condition.hh"
#include "synch_list.hh"

class Channel {
public:
    Channel(const char* debugName);
    ~Channel();

    void Send(int message);
    void Receive(int* message);

private:
    char* lockName;
    char* conditionNameEmptyBuffer;
    char* conditionNameFullBuffer;
    char* conditionNameCommunicationAck;
    Lock* lock;

    Condition* emptyBuffer;
    Condition* fullBuffer;
    Condition* communicationAck;
    int* buffer;
};

#endif