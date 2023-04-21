/// Routines to manage address spaces (memory for executing user programs).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#include "address_space.hh"
#include "executable.hh"
#include "threads/system.hh"
#include "lib/utility.hh"
#include "machine/mmu.hh"

#include <string.h>

/// First, set up the translation from program memory to physical memory.
/// For now, this is really simple (1:1), since we are only uniprogramming,
/// and we have a single unsegmented page table.
AddressSpace::AddressSpace(OpenFile *executable_file)
{
    ASSERT(executable_file != nullptr);

    Executable exe(executable_file);
    ASSERT(exe.CheckMagic());

    // How big is address space?

    unsigned size = exe.GetSize() + USER_STACK_SIZE;
    // We need to increase the size to leave room for the stack.
    numPages = DivRoundUp(size, PAGE_SIZE);
    size = numPages * PAGE_SIZE;

    ASSERT(numPages <= pageMap->CountClear());
    // Check we are not trying to run anything too big -- at least until we
    // have virtual memory.

    DEBUG('a', "Initializing address space, num pages %u, size %u\n",
          numPages, size);

    // First, set up the translation.

    pageTable = new TranslationEntry[numPages];
    for (unsigned i = 0; i < numPages; i++)
    {
        pageTable[i].virtualPage = i;
        // For now, virtual page number = physical page number.
        pageTable[i].physicalPage = pageMap->Find();
        pageTable[i].valid = true;
        pageTable[i].use = false;
        pageTable[i].dirty = false;
        pageTable[i].readOnly = false;
        // If the code segment was entirely on a separate page, we could
        // set its pages to be read-only.
    }

    char *mainMemory = machine->GetMMU()->mainMemory;

    // Zero out the entire address space, to zero the unitialized data
    // segment and the stack segment.
    for (unsigned i = 0; i < numPages; i++)
    {
        memset(mainMemory + pageTable[i].physicalPage * PAGE_SIZE, 0, PAGE_SIZE);
    }

    // Then, copy in the code and data segments into memory.
    uint32_t codeSize = exe.GetCodeSize();
    uint32_t initDataSize = exe.GetInitDataSize();

    if (codeSize > 0)
    {
        uint32_t virtualAddr = exe.GetCodeAddr();

        unsigned codeOffset = 0;
        uint32_t leftOverSize = codeSize;

        for (unsigned i = 0; i < DivRoundUp(codeSize, PAGE_SIZE); i++)
        {
            uint32_t physicalAddr = TranslateVirtualAddrToPhysicalAddr(virtualAddr);

            uint32_t toRead = leftOverSize < PAGE_SIZE ? leftOverSize : PAGE_SIZE;

            DEBUG('a', "Initializing data segment, at virtual address 0x%X, physical address 0x%X size %u\n",
                  virtualAddr, physicalAddr, toRead);

            exe.ReadCodeBlock(&mainMemory[physicalAddr], toRead, codeOffset);
            codeOffset += toRead;
            leftOverSize -= toRead;
            virtualAddr += toRead;
        };
    }
    if (initDataSize > 0)
    {
        uint32_t virtualAddr = exe.GetInitDataAddr();

        unsigned dataOffset = 0;
        uint32_t leftOverSize = initDataSize;

        for (unsigned i = 0; i < DivRoundUp(initDataSize, PAGE_SIZE); i++)
        {
            uint32_t physicalAddr = TranslateVirtualAddrToPhysicalAddr(virtualAddr);

            uint32_t toRead = leftOverSize < PAGE_SIZE ? leftOverSize : PAGE_SIZE;

            DEBUG('a', "Initializing data segment, at virtual address 0x%X, physical address 0x%X size %u\n",
                  virtualAddr, physicalAddr, toRead);

            exe.ReadDataBlock(&mainMemory[physicalAddr], toRead, dataOffset);
            dataOffset += toRead;
            leftOverSize -= toRead;
            virtualAddr += toRead;
        };
    }
}

uint32_t
AddressSpace::TranslateVirtualAddrToPhysicalAddr(uint32_t virtualAddr)
{
    uint32_t virtualPage = DivRoundDown(virtualAddr, PAGE_SIZE);
    uint32_t pageOffset = virtualAddr % PAGE_SIZE;

    return pageTable[virtualPage].physicalPage * PAGE_SIZE + pageOffset;
}

/// Deallocate an address space.
///
/// Nothing for now!
AddressSpace::~AddressSpace()
{
    for (int i = 0; i <= numPages; i++)
    {
        pageMap->Clear(pageTable[i].physicalPage);
    }

    delete[] pageTable;
}

/// Set the initial values for the user-level register set.
///
/// We write these directly into the “machine” registers, so that we can
/// immediately jump to user code.  Note that these will be saved/restored
/// into the `currentThread->userRegisters` when this thread is context
/// switched out.
void AddressSpace::InitRegisters()
{
    for (unsigned i = 0; i < NUM_TOTAL_REGS; i++)
    {
        machine->WriteRegister(i, 0);
    }

    // Initial program counter -- must be location of `Start`.
    machine->WriteRegister(PC_REG, 0);

    // Need to also tell MIPS where next instruction is, because of branch
    // delay possibility.
    machine->WriteRegister(NEXT_PC_REG, 4);

    // Set the stack register to the end of the address space, where we
    // allocated the stack; but subtract off a bit, to make sure we do not
    // accidentally reference off the end!
    machine->WriteRegister(STACK_REG, numPages * PAGE_SIZE - 16);
    DEBUG('a', "Initializing stack register to %u\n",
          numPages * PAGE_SIZE - 16);
}

/// On a context switch, save any machine state, specific to this address
/// space, that needs saving.
///
/// For now, nothing!
void AddressSpace::SaveState()
{
    // int stackAddr = machine->ReadRegister(STACK_REG);

    // for (unsigned i = 0; i < NUM_TOTAL_REGS; i++)
    // {
    //     int registryValue = machine->ReadRegister(i);
    //     int physicalAddr = TranslateVirtualAddrToPhysicalAddr(stackAddr);

    //     machine->GetMMU()->WriteMem(physicalAddr, 4, registryValue);
    //     stackAddr-=4;
    // }
}

/// On a context switch, restore the machine state so that this address space
/// can run.
///
/// For now, tell the machine where to find the page table.
void AddressSpace::RestoreState()
{
    machine->GetMMU()->pageTable = pageTable;
    machine->GetMMU()->pageTableSize = numPages;
}
