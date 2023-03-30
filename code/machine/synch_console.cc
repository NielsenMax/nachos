#include "synch_console.hh"
#include "threads/system.hh"

#include <stdio.h>

static void
ReadAvailProxy(void *args)
{
    ASSERT(data != nullptr);
    ((SynchConsole *)data)->ReadAvail();
}

static void
WriteDoneProxy(void *args)
{
    ASSERT(data != nullptr);
    ((SynchConsole *)data)->WriteDone();
}

SynchConsole::SynchConsole(const char *in, const char *out)
{
    readAvail = new Semaphore("ReadAvailSemaphore", 0);
    writeDone = new Semaphore("WriteDoneSemaphore", 0);
    writeLock = new Lock("SynchConsoleWriteLock");
    readLock = new Lock("SynchConsoleReadLock");
    console = new Console(in, out, ReadAvailProxy, WriteDoneProxy, this);
}

void SynchConsole::ReadAvail()
{
    readAvail->V();
}

void SynchConsole::WriteDone()
{
    writeDone->V();
}

SynchConsole::~SynchConsole()
{
    delete readAvail;
    delete writeDone;
    delete writeLock;
    delete console;
}

char SynchConsole::GetChar()
{
    readLock->Acquire();

    readAvail->P();
    char output = console->GetChar();
    
    readLock->Release();

    return output;
}

void SynchConsole::PutChar(char ch)
{
    writeLock->Acquire();

    console->PutChar(ch);
    writeDone->P();

    writeLock->Release();
}
