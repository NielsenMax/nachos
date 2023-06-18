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
#include <string>

/// First, set up the translation from program memory to physical memory.
/// For now, this is really simple (1:1), since we are only uniprogramming,
/// and we have a single unsegmented page table.
AddressSpace::AddressSpace(OpenFile* _executable_file)
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
        pageTable[i].readOnly = false;
        // If the code segment was entirely on a separate page, we could
        // set its pages to be read-only.
    }

#ifndef DEMAND_LOADING
    char* mainMemory = machine->GetMMU()->mainMemory;
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

        unsigned numCodePages = DivRoundUp(codeSize, PAGE_SIZE);

        for (unsigned i = 0; i < numCodePages; i++)
        {
            uint32_t virtualPage;
            uint32_t physicalAddr = TranslateVirtualAddrToPhysicalAddr(virtualAddr, &virtualPage);

            if (i + 1 < numCodePages)
            {
                pageTable[virtualPage].readOnly = true;
            }

            uint32_t toRead = leftOverSize < PAGE_SIZE ? leftOverSize : PAGE_SIZE;

            DEBUG('a', "Initializing code segment, at virtual address 0x%X, physical address 0x%X size %u\n",
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
            uint32_t virtualPage;
            uint32_t physicalAddr = TranslateVirtualAddrToPhysicalAddr(virtualAddr, &virtualPage);

            // if the code segment and the data segment share a page this will remove the readonly property
            pageTable[virtualPage].readOnly = false;

            uint32_t toRead = leftOverSize < PAGE_SIZE ? leftOverSize : PAGE_SIZE;

            DEBUG('a', "Initializing data segment, at virtual address 0x%X, physical address 0x%X size %u\n",
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
AddressSpace::TranslateVirtualAddrToPhysicalAddr(uint32_t virtualAddr, uint32_t* virtualPagePointer)
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
    if (swapFile != nullptr) {
        delete swapFile;
        fileSystem->Remove(swapName);
        delete[] swapName;
    }
#endif
}

TranslationEntry* AddressSpace::LoadPage(unsigned virtualAddr)
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
    if (pageTable[virtualPage].virtualPage != virtualPage) {
        bool err = UnswapPage(virtualPage);
        DEBUG('d', "the unswap was succesful %d\n", err);
        return &pageTable[virtualPage];
    }
#endif

    Executable exe(executable_file);
    uint32_t codeSize = exe.GetCodeSize();
    uint32_t initDataSize = exe.GetInitDataSize();
    char* mainMemory = machine->GetMMU()->mainMemory;

    uint32_t initialDataVirtualAddr = exe.GetInitDataAddr();
    uint32_t codeVirtualAddr = exe.GetCodeAddr();

    uint32_t lastCodePage = DivRoundDown(codeSize + codeVirtualAddr, PAGE_SIZE);
    uint32_t lastCodePageSize = codeSize % PAGE_SIZE;
    // uint32_t lastCodePageAddr = lastCodePage * PAGE_SIZE;

    uint32_t lastInitDataPage = DivRoundDown(initDataSize + initialDataVirtualAddr, PAGE_SIZE);
    uint32_t lastInitDataPageSize = initDataSize % PAGE_SIZE;

    // DEBUG('d', "lastCodePage %d\n", lastCodePage);
    // DEBUG('d', "lastCodePageSize %d\n", lastCodePageSize);
    // DEBUG('d', "lastCodePageAddr %d\n", lastCodePageAddr);
    // DEBUG('d', "lastInitDataPage %d\n", lastInitDataPage);
    // DEBUG('d', "lastInitDataPageSize %d\n", lastInitDataPageSize);
    // DEBUG('d', "virtualPage %d\n", virtualPage);
    // DEBUG('d', "initialDataVirtualAddr %d\n", initialDataVirtualAddr);
    // DEBUG('d', "numPages %d\n", numPages);
    // DEBUG('d', "codeSize %d\n", codeSize);
    // DEBUG('d', "initDataSize %d\n", initDataSize);

    uint32_t remaining = PAGE_SIZE;
    uint32_t pageInit = virtualPage * PAGE_SIZE;

    if (virtualPage <= lastCodePage)
    {
        uint32_t toRead = PAGE_SIZE;
        if (virtualPage == lastCodePage)
        {
            toRead = lastCodePageSize;
        }
        uint32_t physicalAddr = TranslateVirtualAddrToPhysicalAddr(pageInit, nullptr);
        DEBUG('d', "Writing %d to %d\n", toRead, pageInit);
        exe.ReadCodeBlock(&mainMemory[physicalAddr], toRead, pageInit);
        remaining -= toRead;
    }
    if (remaining > 0 && initDataSize > 0 && virtualPage <= lastInitDataPage)
    {
        uint32_t toRead = remaining;
        if (virtualPage == lastInitDataPage)
        {
            ASSERT(lastInitDataPageSize <= remaining);
            toRead = lastInitDataPageSize;
        }
        uint32_t physicalAddr = TranslateVirtualAddrToPhysicalAddr(pageInit + PAGE_SIZE - remaining, nullptr);
        exe.ReadDataBlock(&mainMemory[physicalAddr], toRead, (virtualPage - lastCodePage) * PAGE_SIZE);
        remaining -= toRead;
    }
    if (remaining > 0)
    {
        uint32_t physicalAddr = TranslateVirtualAddrToPhysicalAddr(pageInit - remaining + PAGE_SIZE, nullptr);
        memset(&mainMemory[physicalAddr], 0, remaining);
    }

    return &pageTable[virtualPage];
};

#ifdef SWAP_ENABLED

bool AddressSpace::UnswapPage(unsigned virtualPage) {
    DEBUG('e', "going to unswap page %d\n", virtualPage);

    ASSERT(swapFile != nullptr);
    char* mainMemory = machine->GetMMU()->mainMemory;
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
bool AddressSpace::SwapPage(unsigned virtualPage) {
    DEBUG('e', "going to swap page %d\n", virtualPage);
    // The wasn't created
    if (swapFile == nullptr) {
        swapName = new char[10];
        sprintf(swapName, "SWAP.%u", spaceId); // spaceId should be set by this point
        // ? Maybe be a good idea to check if the file already exist
        // ? I supose be should also delete it from the fs when we are destroy
        if (!fileSystem->Create(swapName, numPages * PAGE_SIZE)) {
            return false;
        }
        swapFile = fileSystem->Open(swapName);
        if (swapFile == nullptr) {
            return false;
        }
    }

    char* mainMemory = machine->GetMMU()->mainMemory;
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

/// On a context switch, save any machine state, specific to this address
/// space, that needs saving.
///
/// For now, nothing!
void AddressSpace::SaveState()
{
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
