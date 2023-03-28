#ifndef NACHOS_THREADS_CHANNEL__HH
#define NACHOS_THREADS_CHANNEL__HH

#include "lock.hh"
#include "semaphore.hh"
#include "synch_list.hh"

class Channel {
public:
    Channel(const char* debugName);
    ~Channel();

    void Send(int message);
    void Receive(int* message);

private:
    char* lockName;
    char* semaphoreNameRecievers;
    char* semaphoreNameCommunicationAck;
    
    Lock* lock;
    Semaphore* recievers;
    Semaphore* communicationAck;
    
    int buffer;
};

#endif