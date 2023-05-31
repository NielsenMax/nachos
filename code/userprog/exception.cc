/// Entry points into the Nachos kernel from user programs.
///
/// There are two kinds of things that can cause control to transfer back to
/// here from user code:
///
/// * System calls: the user code explicitly requests to call a procedure in
///   the Nachos kernel.  Right now, the only function we support is `Halt`.
///
/// * Exceptions: the user code does something that the CPU cannot handle.
///   For instance, accessing memory that does not exist, arithmetic errors,
///   etc.
///
/// Interrupts (which can also cause control to transfer from user code into
/// the Nachos kernel) are handled elsewhere.
///
/// For now, this only handles the `Halt` system call.  Everything else core-
/// dumps.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#include "exception.hh"
#include "transfer.hh"
#include "syscall.h"
#include "filesys/directory_entry.hh"

#include <stdio.h>

#define USE_TLB 1

static void
IncrementPC()
{
    unsigned pc;

    pc = machine->ReadRegister(PC_REG);
    machine->WriteRegister(PREV_PC_REG, pc);
    pc = machine->ReadRegister(NEXT_PC_REG);
    machine->WriteRegister(PC_REG, pc);
    pc += 4;
    machine->WriteRegister(NEXT_PC_REG, pc);
}

/// Do some default behavior for an unexpected exception.
///
/// NOTE: this function is meant specifically for unexpected exceptions.  If
/// you implement a new behavior for some exception, do not extend this
/// function: assign a new handler instead.
///
/// * `et` is the kind of exception.  The list of possible exceptions is in
///   `machine/exception_type.hh`.
static void
DefaultHandler(ExceptionType et)
{
    int exceptionArg = machine->ReadRegister(2);
    fprintf(stderr, "Unexpected user mode exception: %s, arg %d.\n",
            ExceptionTypeToString(et), exceptionArg);
    ASSERT(false);
}

void runProgram(void *argv_)
{
    currentThread->space->InitRegisters(); // Set the initial register values.
    currentThread->space->RestoreState();  // Load page table register.

    DEBUG('e', "Running program.\n");

    if (argv_ != nullptr)
    {
        char **argv = (char **)argv_;

        unsigned argc = WriteArgs(argv);

        // Required "register saves" space
        int argvAddr = machine->ReadRegister(STACK_REG);

        machine->WriteRegister(4, argc);
        machine->WriteRegister(5, argvAddr);
        machine->WriteRegister(STACK_REG, argvAddr - 24); // MIPS call convention
    }

    machine->Run();
}

/// Handle a system call exception.
///
/// * `et` is the kind of exception.  The list of possible exceptions is in
///   `machine/exception_type.hh`.
///
/// The calling convention is the following:
///
/// * system call identifier in `r2`;
/// * 1st argument in `r4`;
/// * 2nd argument in `r5`;
/// * 3rd argument in `r6`;
/// * 4th argument in `r7`;
/// * the result of the system call, if any, must be put back into `r2`.
///
/// And do not forget to increment the program counter before returning. (Or
/// else you will loop making the same system call forever!)
static void
SyscallHandler(ExceptionType _et)
{
    int scid = machine->ReadRegister(2);

    switch (scid)
    {
    case SC_EXEC:
    {
        int filenameAddr = machine->ReadRegister(4);
        int argvAddr = machine->ReadRegister(5);
        int enableJoin = machine->ReadRegister(6);

        if (filenameAddr == 0)
        {
            DEBUG('e', "1Error: address to filename string is null.\n");
            machine->WriteRegister(2, -1);
            break;
        }

        char *filename = new char[FILE_NAME_MAX_LEN + 1];
        if (!ReadStringFromUser(filenameAddr,
                                filename, FILE_NAME_MAX_LEN))
        {
            DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                  FILE_NAME_MAX_LEN);
            machine->WriteRegister(2, -1);
            break;
        }
        DEBUG('d', "[d] Filename to be exec %s\n", filename);
        OpenFile *file = fileSystem->Open(filename);
        if (file == nullptr)
        {
            DEBUG('e', "Error: file %s to be exec not found\n", filename);
            machine->WriteRegister(2, -1);
            break;
        }

        AddressSpace *newAddrSpace = new AddressSpace(file);
        // newAddrSpace->InitRegisters(); // Set the initial register values.
        // newAddrSpace->RestoreState();  // Load page table register.

        Thread *newThread = new Thread(filename, bool(enableJoin), currentThread->GetPriority());

        int spaceId = newThread->SetAddressSpace(newAddrSpace);

        if (argvAddr == 0)
        {
            newThread->Fork(runProgram, nullptr);
        }
        else
        {
            newThread->Fork(runProgram, SaveArgs(argvAddr));
        }

        DEBUG('d', "Returning space id %d\n", spaceId);
        machine->WriteRegister(2, spaceId);

        // delete file;
        DEBUG('d', "Returning from exec\n");

        break;
    }
    case SC_JOIN:
    {
        int spaceId = machine->ReadRegister(4);
        DEBUG('d', "Join to %d was called\n", spaceId);

        if (threadsTable->HasKey(spaceId))
        {
            DEBUG('d', "The user program %d exists\n", spaceId);
            Thread *programThread = threadsTable->Get(spaceId);
            int returnCode = programThread->Join();
            machine->WriteRegister(2, returnCode);
            break;
        }

        machine->WriteRegister(2, 1);
        break;
    }
    case SC_OPEN:
    {
        int filenameAddr = machine->ReadRegister(4);
        if (filenameAddr == 0)
        {
            DEBUG('e', "2Error: address to filename string is null.\n");
            machine->WriteRegister(2, -1);
            break;
        }

        char filename[FILE_NAME_MAX_LEN + 1];
        if (!ReadStringFromUser(filenameAddr,
                                filename, sizeof filename))
        {
            DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                  FILE_NAME_MAX_LEN);
            machine->WriteRegister(2, -1);
            break;
        }

        OpenFile *file = fileSystem->Open(filename);
        int fileId = currentThread->AddFile(file);

        if (fileId == -1)
        {
            DEBUG('a', "Error: fileTable of %s is full.\n", currentThread->GetName());
            machine->WriteRegister(2, -1);
            break;
        }
        DEBUG('a', "Thread %s open file %s.\n", currentThread->GetName(), filename);

        machine->WriteRegister(2, fileId);

        break;
    }
    case SC_CLOSE:
    {
        int fileId = machine->ReadRegister(4);

        if (currentThread->HasFile(fileId))
        {
            currentThread->RemoveFile(fileId);
            DEBUG('a', "Close requested for id %u.\n", fileId);
            machine->WriteRegister(2, 1);
        }
        else
        {
            DEBUG('a', "Error: file %d not open.\n");
            machine->WriteRegister(2, 0);
        }

        break;
    }
    case SC_READ:
    {

        int bufferAddr = machine->ReadRegister(4);
        if (bufferAddr == 0)
        {
            DEBUG('e', "Error: .\n");
            machine->WriteRegister(2, -1);
            break;
        }

        int size = machine->ReadRegister(5);
        int fileId = machine->ReadRegister(6);

        char *string = new char[size + 1];
        int read = 0;
        if (fileId != CONSOLE_INPUT)
        {
            if (!currentThread->HasFile(fileId))
            {
                DEBUG('e', "Error: file %d is not open for current thread.\n", fileId);
                machine->WriteRegister(2, -1);
                delete[] string;
                break;
            }
            OpenFile *file = currentThread->GetFile(fileId);
            read = file->Read(string, size);
        }
        else
        {
            int i;
            for (i = 0; i < size; i++)
            {
                string[i] = synchConsole->GetChar();
            }
            string[i] = '\0';
            DEBUG('d', "[d] Readed %s\n", string);
            read = size;
        }
        WriteBufferToUser(string, bufferAddr, read);
        machine->WriteRegister(2, read);

        delete[] string;

        break;
    }
    case SC_WRITE:
    {
        // int input = machine->ReadRegister(4);
        // synchConsole->PutChar(input);
        // DEBUG('e', "PUtting: %c on console.\n", input);
        int bufferAddr = machine->ReadRegister(4);
        if (bufferAddr == 0)
        {
            DEBUG('e', "3Error: address to filename string is null.\n");
            machine->WriteRegister(2, -1);
            break;
        }

        int size = machine->ReadRegister(5);
        int fileId = machine->ReadRegister(6);

        char *string = new char[size + 1];

        ReadBufferFromUser(bufferAddr, string, size);

        int writed = 0;
        if (fileId != CONSOLE_OUTPUT)
        {
            if (!currentThread->HasFile(fileId))
            {
                DEBUG('e', "Error: file %d is not open for current thread.\n", fileId);
                machine->WriteRegister(2, -1);
                delete[] string;
                break;
            }
            OpenFile *file = currentThread->GetFile(fileId);
            writed = file->Write(string, size);
        }
        else
        {
            for (int i = 0; i < size; i++)
            {
                synchConsole->PutChar(string[i]);
            }
            writed = size;
        }
        machine->WriteRegister(2, writed);
        delete[] string;

        break;
    }
    case SC_HALT:
    {

        DEBUG('e', "Shutdown, initiated by user program.\n");
        interrupt->Halt();
        break;
    }
    case SC_CREATE:
    {
        int filenameAddr = machine->ReadRegister(4);
        if (filenameAddr == 0)
        {
            DEBUG('e', "4Error: address to filename string is null.\n");
            machine->WriteRegister(2, -1);
            break;
        }

        char filename[FILE_NAME_MAX_LEN + 1];
        if (!ReadStringFromUser(filenameAddr,
                                filename, sizeof filename))
        {
            DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                  FILE_NAME_MAX_LEN);
            machine->WriteRegister(2, -1);
            break;
        }

        DEBUG('e', "`Create` requested for file `%s`.\n", filename);
        bool success = fileSystem->Create(filename, 0);

        if (success)
        {
            DEBUG('e', "File `%s` created successfully.\n", filename);
            machine->WriteRegister(2, 0);
        }
        else
        {
            DEBUG('e', "Failed to create file `%s`.\n", filename);
            machine->WriteRegister(2, -1);
        }

        break;
    }

    case SC_EXIT:
    {
        int status = machine->ReadRegister(4);
        DEBUG('d', "Finishing thread %s with status %d\n", currentThread->GetName(), status);
        currentThread->Finish(status);
        DEBUG('e', "Thread finished.\n");
        break;
    }

    case SC_REMOVE:
    {
        int filenameAddr = machine->ReadRegister(4);
        if (filenameAddr == 0)
        {
            DEBUG('e', "5Error: address to filename string is null.\n");
            machine->WriteRegister(2, -1);
            break;
        }

        char filename[FILE_NAME_MAX_LEN + 1];
        if (!ReadStringFromUser(filenameAddr,
                                filename, sizeof filename))
        {
            DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                  FILE_NAME_MAX_LEN);
            machine->WriteRegister(2, -1);
            break;
        }

        int success = fileSystem->Remove(filename);
        if (success)
        {
            DEBUG('e', "File `%s` removed successfully.\n", filename);
            machine->WriteRegister(2, 0);
        }
        else
        {
            DEBUG('e', "Failed to remove file `%s`.\n", filename);
            machine->WriteRegister(2, -1);
        }

        break;
    }

    case SC_PS:
    {
        scheduler->Print();
        break;
    }

    default:
        fprintf(stderr, "Unexpected system call: id %d.\n", scid);
        ASSERT(false);
    }

    IncrementPC();
}

#ifdef USE_TLB
int TLB_FIFO = 0;

static void
PageFaultExceptionHanlder(ExceptionType et)
{
    int virtualAddr = machine->ReadRegister(BAD_VADDR_REG);

    uint32_t virtualPage;
    currentThread->space->TranslateVirtualAddrToPhysicalAddr(virtualAddr, &virtualPage);
    unsigned tlbEntryIndex = TLB_FIFO;

    TranslationEntry *spaceEntry = currentThread->space->LoadPage(virtualAddr);
    #ifdef SWAP_ENABLED
    pageMap->Get(spaceEntry->physicalPage);
    #endif
    stats->numPageFaults++;

    TranslationEntry *tlbEntry = &machine->GetMMU()->tlb[tlbEntryIndex];

    tlbEntry->valid = spaceEntry->valid;
    tlbEntry->virtualPage = virtualPage;
    tlbEntry->physicalPage = spaceEntry->physicalPage;
    tlbEntry->readOnly = spaceEntry->readOnly;
    tlbEntry->use = spaceEntry->use;
    tlbEntry->dirty = spaceEntry->dirty;

    DEBUG('d', "Virtual page: %d\nPhysical page: %d\nTLB entry: %d\nVirtual address: %d\n", virtualPage, spaceEntry->physicalPage, tlbEntryIndex, virtualAddr);

    TLB_FIFO = (TLB_FIFO + 1) % TLB_SIZE;
}

static void
ReadOnlyHandler(ExceptionType _et)
{
    currentThread->Finish(-1);
}
#endif

/// By default, only system calls have their own handler.  All other
/// exception types are assigned the default handler.
void SetExceptionHandlers()
{
    machine->SetHandler(NO_EXCEPTION, &DefaultHandler);
    machine->SetHandler(SYSCALL_EXCEPTION, &SyscallHandler);
#ifdef USE_TLB
    machine->SetHandler(PAGE_FAULT_EXCEPTION, &PageFaultExceptionHanlder);
    machine->SetHandler(READ_ONLY_EXCEPTION, &ReadOnlyHandler);
#else
    machine->SetHandler(PAGE_FAULT_EXCEPTION, &DefaultHandler);
    machine->SetHandler(READ_ONLY_EXCEPTION, &DefaultHandler);
#endif
    machine->SetHandler(BUS_ERROR_EXCEPTION, &DefaultHandler);
    machine->SetHandler(ADDRESS_ERROR_EXCEPTION, &DefaultHandler);
    machine->SetHandler(OVERFLOW_EXCEPTION, &DefaultHandler);
    machine->SetHandler(ILLEGAL_INSTR_EXCEPTION, &DefaultHandler);
}
