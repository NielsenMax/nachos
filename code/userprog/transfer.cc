/// Copyright (c) 2019-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#include "transfer.hh"
#include "lib/utility.hh"
#include "threads/system.hh"

void ReadBufferFromUser(int userAddress, char *outBuffer,
                        unsigned byteCount)
{
    ASSERT(userAddress != 0);
    ASSERT(outBuffer != nullptr);
    ASSERT(byteCount != 0);

    unsigned count = 0;
    do
    {
        int temp;
        // ASSERT(machine->ReadMem(userAddress++, 1, &temp));
        for (unsigned i = 0; (i < MAX_MMU_RETIRES) && !machine->ReadMem(userAddress, 1, &temp); i++)
        {
        }
        userAddress++;
        *outBuffer = (unsigned char)temp;
        count++;
        outBuffer++;
    } while (count < byteCount);
}

bool ReadStringFromUser(int userAddress, char *outString,
                        unsigned maxByteCount)
{
    ASSERT(userAddress != 0);
    ASSERT(outString != nullptr);
    ASSERT(maxByteCount != 0);

    unsigned count = 0;
    do
    {
        int temp;
        count++;
        // ASSERT(machine->ReadMem(userAddress++, 1, &temp));
        for (unsigned i = 0; (i < MAX_MMU_RETIRES) && !machine->ReadMem(userAddress, 1, &temp); i++)
        {
        }
        userAddress++;
        *outString = (unsigned char)temp;

    } while (*outString++ != '\0' && count < maxByteCount);

    return *(outString - 1) == '\0';
}

void WriteBufferToUser(const char *buffer, int userAddress,
                       unsigned byteCount)
{
    ASSERT(userAddress != 0);
    ASSERT(buffer != nullptr);
    if (byteCount != 0)
    {
        for (unsigned count = 0; count < byteCount; count++)
        {
            for (unsigned i = 0; (i < MAX_MMU_RETIRES) && !machine->WriteMem(userAddress, 1, buffer[count]); i++)
            {
            }
            userAddress++;
            // ASSERT(machine->WriteMem(userAddress++, 1, buffer[count]));
        }
    }
}

void WriteStringToUser(const char *string, int userAddress)
{
    ASSERT(userAddress != 0);
    ASSERT(string != nullptr);

    for (unsigned count = 0; string[count] != '\0'; count++)
    {
        for (unsigned i = 0; (i < MAX_MMU_RETIRES) && !machine->WriteMem(userAddress, 1, string[count]); i++)
        {
        }
        userAddress++;
        // ASSERT(machine->WriteMem(userAddress++, 1, string[count]));
    }
}
