/// All global variables used in Nachos are defined here.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_THREADS_SYSTEM__HH
#define NACHOS_THREADS_SYSTEM__HH

#include "thread.hh"
#include "scheduler.hh"
#include "lib/utility.hh"
#include "machine/interrupt.hh"
#include "machine/statistics.hh"
#include "machine/timer.hh"


// TODO: borrar.
// #define USER_PROGRAM 1
// #define FILESYS_NEEDED 1
// #define SWAP_ENABLED 1 

/// Initialization and cleanup routines.

// Initialization, called before anything else.
extern void Initialize(int argc, char **argv);

// Cleanup, called when Nachos is done.
extern void Cleanup();

extern Thread *currentThread;       ///< The thread holding the CPU.
extern Thread *threadToBeDestroyed; ///< The thread that just finished.
extern Scheduler *scheduler;        ///< The ready list.
extern Interrupt *interrupt;        ///< Interrupt status.
extern Statistics *stats;           ///< Performance metrics.
extern Timer *timer;                ///< The hardware alarm clock.

#ifdef USER_PROGRAM
#include "userprog/synch_console.hh"
#include "machine/machine.hh"
#include "lib/bitmap.hh"
#include "lib/table.hh"

class SynchConsole;

extern Machine *machine; // User program memory and registers.
extern SynchConsole *synchConsole; // Console used in syscall testing
#ifdef SWAP_ENABLED
#include "vmem/coremap.hh"
extern Coremap* pageMap;
#else
extern Bitmap* pageMap;
#endif
extern Table<Thread*> *threadsTable;

#endif

#ifdef FILESYS_NEEDED // *FILESYS* or *FILESYS_STUB*.
class FileSystem;

extern FileSystem *fileSystem;
#endif

#ifdef FILESYS
#include "filesys/synch_disk.hh"
class SynchDisk;
extern SynchDisk *synchDisk;
#endif

#ifdef NETWORK
#include "network/post.hh"
extern PostOffice *postOffice;
#endif

#endif
