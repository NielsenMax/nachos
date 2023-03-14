/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#include "thread_test_prod_cons.hh"

#include "lock.hh"
#include "condition.hh"
#include <stdio.h>
#include <cstring>

static const unsigned NUM_PRODUCERS = 2;
static const unsigned NUM_CONSUMERS = 5;
static const unsigned BUFFER_SIZE = 20;

template <class Item>
class Buffer
{
public:
    Buffer();
    ~Buffer();

    void
    Put(Item item);

    Item
    Pop();

private:
    Item *list;
    int count = 0;
    int maxItems = BUFFER_SIZE - 1;

    Lock *listLock;
    Condition *emptyList;
    Condition *fullList;
};

template <class Item>
Buffer<Item>::Buffer()
{
    list = new Item[BUFFER_SIZE];
    listLock = new Lock("Buffer");
    emptyList = new Condition("Buffer::emptyList", listLock);
    fullList = new Condition("Buffer::fullList", listLock);
}

template <class Item>
Buffer<Item>::~Buffer()
{
    delete[] list;
    delete listLock;
    delete emptyList;
}

template <class Item>
void Buffer<Item>::Put(Item item)
{
    listLock->Acquire();

    while (count == maxItems)
    {
        fullList->Wait();
    }

    list[count++] = item;

    emptyList->Signal();
    listLock->Release();
}

template <class Item>
Item Buffer<Item>::Pop()
{
    listLock->Acquire();

    while (count == 0)
    {
        emptyList->Wait();
    }
    Item elem = list[count--];

    fullList->Signal();
    listLock->Release();

    return elem;
}

void ProducerThread(void *args)
{
    while (true)
    {
        char *name = (char *)(((void **)args)[0]);
        Buffer<int> *buffer = (Buffer<int> *)(((void **)args)[1]);

        buffer->Put(1);
        DEBUG('t', "Producer %s generate item\n", name);
    }
}

void ConsumerThread(void *args)
{
    while (true)
    {
        char *name = (char *)(((void **)args)[0]);
        Buffer<int> *buffer = (Buffer<int> *)(((void **)args)[1]);

        int item = buffer->Pop();
        DEBUG('t', "Consumer %s consume %i\n", name, item);
    }
}

void GenerateProducerThread(const char *threadName, Buffer<int> *buff)
{
    void *args[2];

    char *name = new char[strlen(threadName) + 20];
    sprintf(name, "Producer::%s", threadName);

    args[0] = name;
    args[1] = buff;

    Thread *newThread = new Thread(name);
    newThread->Fork(ProducerThread, (void *)args);
}

void GenerateConsumerThread(const char *threadName, Buffer<int> *buff)
{
    void *args[2];

    char *name = new char[strlen(threadName) + 20];
    sprintf(name, "Producer::%s", threadName);

    args[0] = name;
    args[1] = buff;

    Thread *newThread = new Thread(name);
    newThread->Fork(ProducerThread, args);
}

void ThreadTestProdCons()
{
    Buffer<int>* buff = new Buffer<int>();

    for (unsigned i = 0; i < NUM_PRODUCERS; i++)
    {
        char *name = new char[64];
        snprintf(name, 64, "%i", i + 1);

        GenerateProducerThread(name, buff);
    };

    for (unsigned i = 0; i < NUM_CONSUMERS; i++)
    {
        char *name = new char[64];
        snprintf(name, 64, "%i", i + 1);

        GenerateConsumerThread(name, buff);
    };

    while (true)
    {
        currentThread->Yield();
    };
}
