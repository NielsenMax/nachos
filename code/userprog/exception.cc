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

#include "transfer.hh"
#include "syscall.h"
#include "filesys/directory_entry.hh"
#include "threads/system.hh"

#include <stdio.h>

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
    case SC_OPEN:
    {
        int filenameAddr = machine->ReadRegister(4);
        if (filenameAddr == 0)
        {
            DEBUG('e', "Error: address to filename string is null.\n");
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
        }
        else
        {
            DEBUG('a', "Thread %s open file %s.\n", currentThread->GetName(), filename);
        }

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
        char output = synchConsole->GetChar();
        DEBUG('e', "Reading from console char %c.\n", output);
        machine->WriteRegister(2, output);
        break;
    }
    case SC_WRITE:
    {
        int input = machine->ReadRegister(4);
        synchConsole->PutChar(input);
        DEBUG('e', "PUtting: %c on console.\n", input);
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
            DEBUG('e', "Error: address to filename string is null.\n");
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
        currentThread->Finish(status);

        DEBUG('e', "Finish thread %s with status %d\n", currentThread->GetName(), status);

        break;
    }

    case SC_REMOVE:
    {
        int filenameAddr = machine->ReadRegister(4);
        if (filenameAddr == 0)
        {
            DEBUG('e', "Error: address to filename string is null.\n");
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

    default:
        fprintf(stderr, "Unexpected system call: id %d.\n", scid);
        ASSERT(false);
    }

    IncrementPC();
}

/// By default, only system calls have their own handler.  All other
/// exception types are assigned the default handler.
void SetExceptionHandlers()
{
    machine->SetHandler(NO_EXCEPTION, &DefaultHandler);
    machine->SetHandler(SYSCALL_EXCEPTION, &SyscallHandler);
    machine->SetHandler(PAGE_FAULT_EXCEPTION, &DefaultHandler);
    machine->SetHandler(READ_ONLY_EXCEPTION, &DefaultHandler);
    machine->SetHandler(BUS_ERROR_EXCEPTION, &DefaultHandler);
    machine->SetHandler(ADDRESS_ERROR_EXCEPTION, &DefaultHandler);
    machine->SetHandler(OVERFLOW_EXCEPTION, &DefaultHandler);
    machine->SetHandler(ILLEGAL_INSTR_EXCEPTION, &DefaultHandler);
}
