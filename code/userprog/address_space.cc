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

#include <string>
#include "machine/mmu.hh"

#include <string.h>
#include <string>

/// First, set up the translation from program memory to physical memory.
/// For now, this is really simple (1:1), since we are only uniprogramming,
/// and we have a single unsegmented page table.
AddressSpace::AddressSpace(OpenFile *_executable_file)
{
    executable_file = _executable_file;
    ASSERT(executable_file != nullptr);

    Executable exe(executable_file);
    ASSERT(exe.CheckMagic());

    // How big is address space?

    unsigned size = exe.GetSize() + USER_STACK_SIZE;
    // We need to increase the size to leave room for the stack.
    numPages = DivRoundUp(size, PAGE_SIZE);
    size = numPages * PAGE_SIZE;
#ifndef SWAP_ENABLED
    ASSERT(numPages <= pageMap->CountClear());
#endif
    // Check we are not trying to run anything too big -- at least until we
    // have virtual memory.

    DEBUG('a', "Initializing address space, num pages %u, size %u\n",
          numPages, size);

    // First, set up the translation.

    char *mainMemory = machine->GetMMU()->mainMemory;

    pageTable = new TranslationEntry[numPages];
    for (unsigned i = 0; i < numPages; i++)
    {

#ifdef DEMAND_LOADING
        pageTable[i].physicalPage = 0;
        pageTable[i].valid = false;
#else
#ifdef SWAP_ENABLED
        pageTable[i].physicalPage = pageMap->Find(i);
#else
        pageTable[i].physicalPage = pageMap->Find();
#endif
        pageTable[i].valid = true;
#endif
        // if virtual page is numPages is because its swapped
        pageTable[i].virtualPage = i;
        pageTable[i].use = false;
        pageTable[i].dirty = false;
        // If the code segment was entirely on a separate page, we could
        // set its pages to be read-only.
        pageTable[i].readOnly = false;
        // Zero out the entire address space, to zero the unitialized data
        // segment and the stack segment.
        memset(mainMemory + pageTable[i].physicalPage * PAGE_SIZE, 0, PAGE_SIZE);
    }

#ifndef DEMAND_LOADING

    // Then, copy in the code and data segments into memory.
    uint32_t codeSize = exe.GetCodeSize();
    uint32_t initDataSize = exe.GetInitDataSize();

    if (codeSize > 0)
    {
        uint32_t virtualAddr = exe.GetCodeAddr();

        unsigned codeOffset = 0;
        uint32_t leftOverSize = codeSize;

        while (leftOverSize > 0)
        {
            uint32_t virtualPage;
            uint32_t physicalAddr = TranslateVirtualAddrToPhysicalAddr(virtualAddr, &virtualPage);

            uint32_t toRead = leftOverSize < PAGE_SIZE ? leftOverSize : PAGE_SIZE;

            uint32_t pageOffset = virtualAddr % PAGE_SIZE;
            uint32_t pageRemaining = PAGE_SIZE - pageOffset;
            DEBUG('d', "The page offset is %d\n", pageOffset);
            if (pageRemaining < toRead)
            {
                toRead = pageRemaining;
            }

            DEBUG('d', "Initializing code segment, at virtual address 0x%X, physical address 0x%X size %u\n",
                  virtualAddr, physicalAddr, toRead);

            exe.ReadCodeBlock(&mainMemory[physicalAddr], toRead, codeOffset);
            codeOffset += toRead;
            leftOverSize -= toRead;
            virtualAddr += toRead;

            if (leftOverSize > 0 && pageOffset == 0)
            {
                pageTable[virtualPage].readOnly = true;
            }
        };
    }
    if (initDataSize > 0)
    {
        uint32_t virtualAddr = exe.GetInitDataAddr();

        unsigned dataOffset = 0;
        uint32_t leftOverSize = initDataSize;

        while (leftOverSize > 0)
        {
            uint32_t virtualPage;
            uint32_t physicalAddr = TranslateVirtualAddrToPhysicalAddr(virtualAddr, &virtualPage);

            // if the code segment and the data segment share a page this will remove the readonly property
            pageTable[virtualPage].readOnly = false;

            uint32_t toRead = leftOverSize < PAGE_SIZE ? leftOverSize : PAGE_SIZE;

            uint32_t pageOffset = virtualAddr % PAGE_SIZE;
            uint32_t pageRemaining = PAGE_SIZE - pageOffset;
            DEBUG('d', "The page offset is %d\n", pageOffset);
            if (pageRemaining < toRead)
            {
                toRead = pageRemaining;
            }

            DEBUG('d', "Initializing data segment, at virtual address 0x%X, physical address 0x%X size %u\n",
                  virtualAddr, physicalAddr, toRead);

            exe.ReadDataBlock(&mainMemory[physicalAddr], toRead, dataOffset);
            dataOffset += toRead;
            leftOverSize -= toRead;
            virtualAddr += toRead;
        };
    }
#endif
}

uint32_t
AddressSpace::TranslateVirtualAddrToPhysicalAddr(uint32_t virtualAddr, uint32_t *virtualPagePointer)
{
    uint32_t virtualPage = DivRoundDown(virtualAddr, PAGE_SIZE);
    uint32_t pageOffset = virtualAddr % PAGE_SIZE;

    if (virtualPagePointer != nullptr)
    {

        *virtualPagePointer = virtualPage;
    }

    return pageTable[virtualPage].physicalPage * PAGE_SIZE + pageOffset;
}

/// Deallocate an address space.
///
/// Nothing for now!
AddressSpace::~AddressSpace()
{
    for (unsigned i = 0; i < numPages; i++)
    {
        pageMap->Clear(pageTable[i].physicalPage);
    }

    delete[] pageTable;
    delete executable_file;
#ifdef SWAP_ENABLED
    if (swapFile != nullptr)
    {
        delete swapFile;
        fileSystem->Remove(swapName);
        delete[] swapName;
    }
#endif
}

TranslationEntry *AddressSpace::LoadPage(unsigned virtualAddr)
{
    uint32_t virtualPage = DivRoundDown(virtualAddr, PAGE_SIZE);

    // The page is valid so we can return it
    if (pageTable[virtualPage].valid)
    {
        return &pageTable[virtualPage];
    }

    // The page was never loaded
#ifdef SWAP_ENABLED
    uint32_t physicalPage = pageMap->Find(virtualPage);
#else
    uint32_t physicalPage = pageMap->Find();
#endif
    pageTable[virtualPage].physicalPage = physicalPage;
    pageTable[virtualPage].valid = true;

#ifdef SWAP_ENABLED
    // The page was swapped
    if (pageTable[virtualPage].virtualPage != virtualPage)
    {
        bool err = UnswapPage(virtualPage);
        DEBUG('d', "the unswap was succesful %d\n", err);
        return &pageTable[virtualPage];
    }
#endif

    Executable exe(executable_file);
    char *mainMemory = machine->GetMMU()->mainMemory;

    DEBUG('k', "Exe file %p\n", executable_file);
    DEBUG('k', "Current thread %s\n", currentThread->GetName());

    uint32_t codeSize = exe.GetCodeSize();
    uint32_t initialCodeVirtualAddr = exe.GetCodeAddr();
    uint32_t lastCodeVirtualAddr = initialCodeVirtualAddr + codeSize;

    uint32_t initDataSize = exe.GetInitDataSize();
    uint32_t initialDataVirtualAddr = exe.GetInitDataAddr();
    uint32_t lastInitDataVirtualAddr = initDataSize + initialDataVirtualAddr;

    uint32_t firstPageToLoadVirtualAddr = virtualPage * PAGE_SIZE;
    uint32_t lastPageToLoadVirtualAddr = firstPageToLoadVirtualAddr + PAGE_SIZE;

    // Init everything in 0
    uint32_t physicalAddr = TranslateVirtualAddrToPhysicalAddr(firstPageToLoadVirtualAddr, nullptr);
    memset(&mainMemory[physicalAddr], 0, PAGE_SIZE);

    DEBUG('k', "Page to load %d\n", virtualPage);
    DEBUG('k', "Code size %d\n", codeSize);
    DEBUG('k', "Data size %d\n", initDataSize);
    DEBUG('k', "Init page virtual addr %d\n", firstPageToLoadVirtualAddr);
    DEBUG('k', "Last page virtual addr %d\n", lastPageToLoadVirtualAddr);
    DEBUG('k', "Init code virtual addr %d\n", initialCodeVirtualAddr);
    DEBUG('k', "Last code virtual addr %d\n", lastCodeVirtualAddr);
    DEBUG('k', "Init data virtual addr %d\n", initialDataVirtualAddr);
    DEBUG('k', "Last data virtual addr %d\n", lastInitDataVirtualAddr);

    uint32_t codeStart = std::max(firstPageToLoadVirtualAddr, initialCodeVirtualAddr);
    uint32_t codeEnd = std::min(lastPageToLoadVirtualAddr, lastCodeVirtualAddr);
    if (codeStart < codeEnd)
    {
        physicalAddr = TranslateVirtualAddrToPhysicalAddr(
            codeStart,
            nullptr);
        DEBUG('k', "Writing Code to physical %d amount %d with offset %d \n", physicalAddr, codeEnd - codeStart, codeStart - initialCodeVirtualAddr);
        exe.ReadCodeBlock(&mainMemory[physicalAddr], codeEnd - codeStart, codeStart - initialCodeVirtualAddr);
    }

    uint32_t dataStart = std::max(firstPageToLoadVirtualAddr, initialDataVirtualAddr);
    uint32_t dataEnd = std::min(lastPageToLoadVirtualAddr, lastInitDataVirtualAddr);
    if (dataStart < dataEnd)
    {
        physicalAddr = TranslateVirtualAddrToPhysicalAddr(
            dataStart,
            nullptr);
        DEBUG('k', "Writing Data to physical %d amount %d with offset %d \n", physicalAddr, dataEnd - dataStart, dataStart - initialDataVirtualAddr);
        exe.ReadDataBlock(&mainMemory[physicalAddr], dataEnd - dataStart, dataStart - initialDataVirtualAddr);
    }

    return &pageTable[virtualPage];
};
#ifdef SWAP_ENABLED

bool AddressSpace::UnswapPage(unsigned virtualPage)
{
    DEBUG('e', "going to unswap page %d\n", virtualPage);

    ASSERT(swapFile != nullptr);
    char *mainMemory = machine->GetMMU()->mainMemory;
    uint32_t virtualAddr = virtualPage * PAGE_SIZE;
    uint32_t physicalAddr = TranslateVirtualAddrToPhysicalAddr(virtualAddr, nullptr);

    // The file should be a copy of the address space, therefore the offset is the virtualAddr
    swapFile->ReadAt(&mainMemory[physicalAddr], PAGE_SIZE, virtualAddr);
    pageTable[virtualPage].valid = true;
    pageTable[virtualPage].virtualPage = virtualPage;
    return true;
}

/// Swap a page. If is call for the first time the swap file is created.
/// Return false if the creation of the file fails.
bool AddressSpace::SwapPage(unsigned virtualPage)
{
    DEBUG('e', "going to swap page %d\n", virtualPage);
    // The wasn't created
    if (swapFile == nullptr)
    {
        swapName = new char[10];
        sprintf(swapName, "SWAP.%u", spaceId); // spaceId should be set by this point
        // ? Maybe be a good idea to check if the file already exist
        // ? I supose be should also delete it from the fs when we are destroy
        if (!fileSystem->Create(swapName, numPages * PAGE_SIZE))
        {
            return false;
        }
        swapFile = fileSystem->Open(swapName);
        if (swapFile == nullptr)
        {
            return false;
        }
    }

    if (currentThread->space == this)
    {
        TranslationEntry *tlb = machine->GetMMU()->tlb;
        for (unsigned i = 0; i < TLB_SIZE; ++i)
            if (tlb[i].valid && tlb[i].virtualPage == virtualPage)
            {
                SyncTlbEntry(i);
            }
    }

    char *mainMemory = machine->GetMMU()->mainMemory;
    uint32_t virtualAddr = virtualPage * PAGE_SIZE;
    uint32_t physicalAddr = TranslateVirtualAddrToPhysicalAddr(virtualAddr, nullptr);

    // The file should be a copy of the address space, therefore the offset is the virtualAddr
    swapFile->WriteAt(&mainMemory[physicalAddr], PAGE_SIZE, virtualAddr);
    pageTable[virtualPage].valid = false;
    pageTable[virtualPage].virtualPage = numPages;
    DEBUG('d', "invalidating page %d of the TLB\n", pageTable[virtualPage].physicalPage);

    machine->GetMMU()->InvalidateTLBPage(pageTable[virtualPage].physicalPage);
    return true;
}
#endif
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

void AddressSpace::SyncTlbEntry(unsigned entry)
{
    DEBUG('v', "Synching from TLB \n");
    TranslationEntry *tlb = machine->GetMMU()->tlb;
    if (tlb[entry].valid)
    {
        pageTable[tlb[entry].virtualPage].dirty = tlb[entry].dirty;
        pageTable[tlb[entry].virtualPage].use = tlb[entry].use;
    }

    tlb[entry].valid = false;
}

/// On a context switch, save any machine state, specific to this address
/// space, that needs saving.
///
/// For now, nothing!
void AddressSpace::SaveState()
{
#ifdef USE_TLB
    for (unsigned i = 0; i < TLB_SIZE; ++i)
    {
        SyncTlbEntry(i);
    }
#endif
}

/// On a context switch, restore the machine state so that this address space
/// can run.
///
/// For now, tell the machine where to find the page table.
void AddressSpace::RestoreState()
{
#ifdef USE_TLB
    machine->GetMMU()->InvalidateTLB();
#else // Use linear page table.
    machine->GetMMU()->pageTable = pageTable;
    machine->GetMMU()->pageTableSize = numPages;
#endif
}
