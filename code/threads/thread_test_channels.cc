/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#include "thread_test_channels.hh"

#include "channel.hh"
#include <stdio.h>
#include <cstring>

static const unsigned NUM_RECEIVERS = 2;
static const unsigned NUM_SENDERS = 5;

void SenderThread(void *args)
{
    char *name = (char *)(((void **)args)[0]);
    Channel *chan = (Channel *)(((void **)args)[1]);

    DEBUG('t', "Starting sender %s\n", name);

    while (true)
    {
        DEBUG('t', "Sender %s send item\n", name);
        chan->Send(2);
        // currentThread->Yield();
    }
}

void ReceiverThread(void *args)
{
    char *name = (char *)(((void **)args)[0]);
    Channel *chan = (Channel *)(((void **)args)[1]);

    DEBUG('t', "Starting receiver %s\n", name);
    while (true)
    {
        int item;
        chan->Receive(&item);
        DEBUG('t', "Receiver %s receive %i\n", name, item);
        // currentThread->Yield();
    }
}

void ThreadTestChannels()
{
    void *args[NUM_RECEIVERS + NUM_SENDERS][2];
    Channel* chan = new Channel("TestChannel");

    for (unsigned i = 0; i < NUM_RECEIVERS; i++)
    {
        char *name = new char[64];
        sprintf(name, "Sender::%i", i + 1);

        args[i][0] = name;
        args[i][1] = chan;

        Thread *newThread = new Thread(name);
        newThread->Fork(SenderThread, (void *)args[i]);
    };

    for (unsigned i = 0; i < NUM_SENDERS; i++)
    {
        char *name = new char[64];
        sprintf(name, "Receiver::%i", i + 1);
        int index = i + NUM_RECEIVERS;

        args[index][0] = name;
        args[index][1] = chan;

        Thread *newThread = new Thread(name);
        newThread->Fork(ReceiverThread, (void *)args[index]);
    };

    while (true)
    {
        currentThread->Yield();
    };
}
