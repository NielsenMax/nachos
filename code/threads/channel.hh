#ifndef NACHOS_THREADS_CHANNEL__HH
#define NACHOS_THREADS_CHANNEL__HH

#include "lock.hh"
#include "synch_list.hh"
#include <tuple>

enum Kind { receivers, senders };

typedef std::tuple<int, Semaphore*> ChannelItem;

class Channel {
public:
    Channel(const char* debugName);
    ~Channel();

    void Send(int message);
    void Receive(int* message);

private:
    char* lockName;
    Lock* lock;

    Kind kindOfWaiters;
    int waiters = 0;
    SynchList<ChannelItem*>* list;
};

#endif