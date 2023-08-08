/// Routines for managing the disk file header (in UNIX, this would be called
/// the i-node).
///
/// The file header is used to locate where on disk the file's data is
/// stored.  We implement this as a fixed size table of pointers -- each
/// entry in the table points to the disk sector containing that portion of
/// the file data (in other words, there are no indirect or doubly indirect
/// blocks). The table size is chosen so that the file header will be just
/// big enough to fit in one disk sector,
///
/// Unlike in a real system, we do not keep track of file permissions,
/// ownership, last modification date, etc., in the file header.
///
/// A file header can be initialized in two ways:
///
/// * for a new file, by modifying the in-memory data structure to point to
///   the newly allocated data blocks;
/// * for a file already on disk, by reading the file header from disk.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "file_header.hh"
#include "threads/system.hh"

#include <ctype.h>
#include <stdio.h>
#include <algorithm>


/// Initialize a fresh file header for a newly created file.  Allocate data
/// blocks for the file out of the map of free disk blocks.  Return false if
/// there are not enough free blocks to accomodate the new file.
///
/// * `freeMap` is the bit map of free disk sectors.
/// * `fileSize` is the bit map of free disk sectors.
bool
FileHeader::Allocate(Bitmap* freeMap, unsigned fileSize)
{
    ASSERT(freeMap != nullptr);

    if (fileSize > MAX_FILE_SIZE) {
        return false;
    }

    raw.numBytes = fileSize;
    raw.numSectors = DivRoundUp(fileSize, SECTOR_SIZE);
    unsigned headersNeeded = 0;
    unsigned remaining = raw.numSectors;
    if (remaining > NUM_DIRECT) {
        headersNeeded++;
    }
    if (remaining > NUM_DIRECT + NUM_INDIRECT) {
        unsigned doubleIndHeaders = DivRoundUp(remaining - (NUM_DIRECT + NUM_INDIRECT), NUM_INDIRECT);
        if (raw.doubleIndirection == -1) {
            headersNeeded += doubleIndHeaders + 1;
        }
        else {
            headersNeeded += doubleIndHeaders - doubleIndirectionArray.size();
        }
    }

    if (freeMap->CountClear() < raw.numSectors + headersNeeded) {
        return false;  // Not enough space.
    }

    unsigned numDirect = std::min(raw.numSectors, NUM_DIRECT);

    for (unsigned i = 0; i < numDirect; i++) {
        raw.dataSectors[i] = freeMap->Find();
    }

    remaining -= numDirect;
    if (remaining > 0) {
        unsigned numFirstIndirection = std::min(remaining, NUM_INDIRECT);
        raw.singleIndirection = freeMap->Find();
        for (unsigned i = 0; i < numFirstIndirection; i++) { // Populate single indirection
            singleIndirection.dataSectors[i] = freeMap->Find();
        }
        remaining -= numFirstIndirection;
        if (remaining > 0) {
            raw.doubleIndirection = freeMap->Find();
            for (unsigned j = 0; remaining > 0; j++) { // Populate double indirecction
                doubleIndirection.dataSectors[j] = freeMap->Find();
                RawFileIndirection rawInd;
                unsigned numSecondIndirection = std::min(remaining, NUM_INDIRECT);
                for (unsigned i = 0; i < numSecondIndirection; i++) { // Populate each indirection
                    rawInd.dataSectors[i] = freeMap->Find();
                }
                doubleIndirectionArray.push_back(rawInd);
                remaining -= numSecondIndirection;
            }
        }
    }
    return true;
}

/// De-allocate all the space allocated for data blocks for this file.
///
/// * `freeMap` is the bit map of free disk sectors.
void
FileHeader::Deallocate(Bitmap* freeMap)
{
    ASSERT(freeMap != nullptr);

    unsigned remaining = raw.numSectors;

    unsigned numDirect = std::min(raw.numSectors, NUM_DIRECT);
    for (unsigned i = 0; i < numDirect; i++) {
        ASSERT(freeMap->Test(raw.dataSectors[i]));  // ought to be marked!
        freeMap->Clear(raw.dataSectors[i]);
    }
    remaining -= numDirect;
    if (remaining > 0) {
        unsigned numFirstIndirection = std::min(remaining, NUM_INDIRECT);
        for (unsigned i = 0; i < numFirstIndirection; i++) {
            freeMap->Clear(singleIndirection.dataSectors[i]);
        }
        freeMap->Clear(raw.singleIndirection);
        remaining -= numFirstIndirection;
        if (remaining > 0) {
            for (unsigned j = 0; j < doubleIndirectionArray.size(); j++) { // Populate double indirecction
                RawFileIndirection rawInd = doubleIndirectionArray[j];
                unsigned numSecondIndirection = std::min(remaining, NUM_INDIRECT);
                for (unsigned i = 0; i < numSecondIndirection; i++) { // Populate each indirection
                    freeMap->Clear(rawInd.dataSectors[i]);
                }
                freeMap->Clear(doubleIndirection.dataSectors[j]);
                remaining -= numSecondIndirection;
            }
            freeMap->Clear(raw.doubleIndirection);
        }
    }
}

/// Fetch contents of file header from disk.
///
/// * `sector` is the disk sector containing the file header.
void
FileHeader::FetchFrom(unsigned sector)
{
    synchDisk->ReadSector(sector, (char*)&raw);
    if (raw.singleIndirection != -1) {
        synchDisk->ReadSector(raw.singleIndirection, (char*)&singleIndirection);
        if (raw.doubleIndirection != -1) {
            synchDisk->ReadSector(raw.doubleIndirection, (char*)&doubleIndirection);
            // Check this math, (sectors - direct sector - singleInd) / num of indirections = num of doubleInd used  
            unsigned numDoubleIndirections = DivRoundUp(raw.numSectors - NUM_DIRECT - NUM_INDIRECT, NUM_INDIRECT);
            doubleIndirectionArray.clear();
            for (unsigned i = 0; i < numDoubleIndirections; i++) {
                RawFileIndirection fileInd;
                synchDisk->ReadSector(doubleIndirection.dataSectors[i], (char*)&fileInd);
                doubleIndirectionArray.push_back(fileInd);
            }
        }
    }
}

/// Write the modified contents of the file header back to disk.
///
/// * `sector` is the disk sector to contain the file header.
void
FileHeader::WriteBack(unsigned sector)
{
    synchDisk->WriteSector(sector, (char*)&raw);
    if (raw.singleIndirection != -1) {
        synchDisk->WriteSector(raw.singleIndirection, (char*)&singleIndirection);
        if (raw.doubleIndirection != -1) {
            synchDisk->WriteSector(raw.doubleIndirection, (char*)&doubleIndirection);
            for (unsigned i = 0; i < doubleIndirectionArray.size();i++) {
                RawFileIndirection fileInd = doubleIndirectionArray[i];
                synchDisk->WriteSector(doubleIndirection.dataSectors[i], (char*)&fileInd);
            }
        }
    }
}

/// Return which disk sector is storing a particular byte within the file.
/// This is essentially a translation from a virtual address (the offset in
/// the file) to a physical address (the sector where the data at the offset
/// is stored).
///
/// * `offset` is the location within the file of the byte in question.
unsigned
FileHeader::ByteToSector(unsigned offset)
{
    unsigned index = offset / SECTOR_SIZE;
    if (index < NUM_DIRECT) {
        return raw.dataSectors[index];
    }
    index -= NUM_DIRECT;
    if (index < NUM_INDIRECT) {
        return singleIndirection.dataSectors[index];
    }
    index -= NUM_INDIRECT;
    unsigned doubleIndTable = index / NUM_INDIRECT;
    return doubleIndirectionArray[doubleIndTable].dataSectors[index % NUM_INDIRECT];
}

bool FileHeader::Extend(Bitmap* freeMap, unsigned extendSize) {
    ASSERT(freeMap != nullptr);

    if (extendSize < raw.numBytes) {
        return false;
    }

    // The new size is over the max
    if (extendSize > MAX_FILE_SIZE) {
        return false;
    }

    // No new space is needed because it fit on the already assign space
    if (extendSize <= (raw.numSectors * SECTOR_SIZE)) {
        raw.numBytes = extendSize;
        return true;
    }

    unsigned sectorsNeeded = DivRoundUp(extendSize, SECTOR_SIZE) - raw.numSectors;
    unsigned sectors = sectorsNeeded + raw.numSectors;
    unsigned headersNeeded = 0;
    if (raw.singleIndirection == -1 && sectors > NUM_DIRECT) {
        headersNeeded++;
    }
    if (sectors > NUM_DIRECT + NUM_INDIRECT) {
        unsigned doubleIndHeaders = DivRoundUp(sectors - (NUM_DIRECT + NUM_INDIRECT), NUM_INDIRECT);
        if (raw.doubleIndirection == -1) {
            headersNeeded += doubleIndHeaders + 1;
        }
        else {
            headersNeeded += doubleIndHeaders - doubleIndirectionArray.size();
        }
    }
    if (freeMap->CountClear() < sectorsNeeded + headersNeeded) {
        return false;
    }
    // Direct sectors are available
    if (raw.numSectors < NUM_DIRECT) {
        for (unsigned i = raw.numSectors;i < NUM_DIRECT && sectorsNeeded > 0; i++) {
            raw.dataSectors[i] = freeMap->Find();
            sectorsNeeded--;
        }
    }
    // Single indirect sectors are available
    if (sectorsNeeded > 0 && raw.numSectors < NUM_DIRECT + NUM_INDIRECT) {
        if (raw.singleIndirection == -1) {
            raw.singleIndirection = freeMap->Find();
        }
        for (unsigned i = raw.numSectors - NUM_DIRECT; i < NUM_INDIRECT && sectorsNeeded > 0; i++) {
            singleIndirection.dataSectors[i] = freeMap->Find();
            sectorsNeeded--;
        }
    }
    if (sectorsNeeded > 0) {
        if (raw.doubleIndirection == -1) {
            raw.doubleIndirection = freeMap->Find();
        }
        unsigned index = doubleIndirectionArray.size();
        // If there space left on the current double indirection
        unsigned sectorsOnDoubleInd = raw.numSectors - NUM_DIRECT - NUM_INDIRECT;
        if (sectorsOnDoubleInd < (index)*NUM_INDIRECT) {
            for (unsigned i = sectorsOnDoubleInd % NUM_INDIRECT; i < NUM_INDIRECT && sectorsNeeded > 0; i++) {
                doubleIndirectionArray[index - 1].dataSectors[i] = freeMap->Find();
                sectorsNeeded--;
            }
        }
        while (sectorsNeeded > 0) {
            RawFileIndirection fileInd;
            doubleIndirection.dataSectors[index] = freeMap->Find();
            unsigned numDoubleInd = std::min(sectorsNeeded, NUM_INDIRECT);
            for (unsigned i = 0; i < numDoubleInd; i++) {
                fileInd.dataSectors[i] = freeMap->Find();
            }
            doubleIndirectionArray.push_back(fileInd);
            sectorsNeeded -= numDoubleInd;
            index++;
        }
    }
    raw.numBytes = extendSize;
    raw.numSectors = DivRoundUp(raw.numBytes, SECTOR_SIZE);
    return true;
}

/// Return the number of bytes in the file.
unsigned
FileHeader::FileLength() const
{
    return raw.numBytes;
}

/// Print the contents of the file header, and the contents of all the data
/// blocks pointed to by the file header.
void
FileHeader::Print(const char* title)
{
    char* data = new char[SECTOR_SIZE];

    if (title == nullptr) {
        printf("File header:\n");
    }
    else {
        printf("%s file header:\n", title);
    }

    printf("    size: %u bytes\n"
        "    direct block indexes: ",
        raw.numBytes);

    unsigned remaining = raw.numSectors;
    unsigned numDirect = std::min(raw.numSectors, NUM_DIRECT);

    for (unsigned i = 0; i < numDirect; i++) {
        printf("%u ", raw.dataSectors[i]);
    }
    printf("\n");
    remaining -= numDirect;
    if (remaining > 0) {
        printf("    single inderect header: %u\n",
            raw.singleIndirection);
        unsigned numFirstIndirection = std::min(remaining, NUM_INDIRECT);
        printf("    single indirection block indexes: ");
        for (unsigned i = 0; i < numFirstIndirection; i++) {
            printf("%u ", singleIndirection.dataSectors[i]);
        }
        printf("\n");
        remaining -= numFirstIndirection;
        if (remaining > 0) {
            printf("    double inderect header: %u\n",
                raw.doubleIndirection);
            for (unsigned j = 0; j < doubleIndirectionArray.size(); j++) {
                printf("    double indirection block %u indexes: ", doubleIndirection.dataSectors[j]);
                RawFileIndirection rawInd = doubleIndirectionArray[j];
                unsigned numSecondIndirection = std::min(remaining, NUM_INDIRECT);
                for (unsigned i = 0; i < numSecondIndirection; i++) {
                    printf("%u ", rawInd.dataSectors[i]);
                }
                printf("\n");
                remaining -= numSecondIndirection;
            }
        }
    }
    printf("\n");

    for (unsigned i = 0, k = 0; i < raw.numSectors; i++) {
        unsigned sector;
        unsigned index = i;
        if (i < NUM_DIRECT) {
            sector = raw.dataSectors[index];
        }
        else {
            index -= NUM_DIRECT;
            if (index < NUM_INDIRECT) {
                sector = singleIndirection.dataSectors[index];
            }
            else {
                index -= NUM_INDIRECT;
                unsigned doubleIndTable = DivRoundDown(index, NUM_INDIRECT);
                sector = doubleIndirectionArray[doubleIndTable].dataSectors[index % NUM_INDIRECT];
            }
        }

        printf("    contents of block %u:\n", sector);
        synchDisk->ReadSector(sector, data);
        for (unsigned j = 0; j < SECTOR_SIZE && k < raw.numBytes; j++, k++) {
            if (isprint(data[j])) {
                printf("%c", data[j]);
            }
            else {
                printf("\\%X", (unsigned char)data[j]);
            }
        }
        printf("\n");
    }
    delete[] data;
}

const RawFileHeader*
FileHeader::GetRaw() const
{
    return &raw;
}

const RawFileIndirection *FileHeader::GetRawSingleIndirection() const{
    return &singleIndirection;
}
const RawFileIndirection *FileHeader::GetRawDoubleIndirection() const{
    return &doubleIndirection;
}
const RawFileIndirection *FileHeader::GetRawSingleIndirectioOfDouble(unsigned index) const {
    return &doubleIndirectionArray[index];
}