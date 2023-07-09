#include "file_table.hh"

#include <stdio.h>

FileRef::FileRef(unsigned sector_, const char *name_)
{
    sector = sector_;
    name = name_;
    nameLock = new char[12];
    sprintf(nameLock, "RWLock::%d", sector);
    lock = new RWLock(nameLock);

    refCount = 0;
    toDelete = false;
}

FileRef::~FileRef()
{
    delete lock;
    delete[] nameLock;
}

FileTable::FileTable()
{
    nameLock = new char[13];
    sprintf(nameLock, "ReadersLock");
    lock = new Lock(nameLock);

    files = new Table<FileRef *>();
}

FileTable::~FileTable()
{
    delete lock;
    delete[] nameLock;

    for (int i = 0; !files->IsEmpty() && files->HasKey(i); i++)
    {
        FileRef *fileRef = files->Get(i);
        delete fileRef;
    }

    delete files;
}

// MUST BE CALL WITH THE LOCK ADQUIRE
int FileTable::findFileRef(unsigned sector)
{
    int fileid = -1;

    for (unsigned i = 0; files->HasKey(i); i++)
    {
        FileRef *iter = files->Get(i);
        if (iter->sector == sector)
        {
            fileid = i;
            break;
        };
    }

    return fileid;
}

int FileTable::OpenFile(unsigned sector, const char *name, RWLock **fileLock)
{
    lock->Acquire();

    int fileid = findFileRef(sector);
    FileRef *fileRef = nullptr;
    if (fileid == -1)
    {
        fileRef = new FileRef(sector, name);
        fileid = files->Add(fileRef);
        if (fileid == -1)
        {
            lock->Release();
            delete fileRef;
            return fileid;
        }
    }
    else
    {
        fileRef = files->Get(fileid);
    }
    if (fileRef->toDelete)
    {
        // Was find but is to be delete
        fileid = -1;
    }
    else
    {
        // Was find
        fileRef->refCount++;
    }
    if (fileLock != nullptr && fileid != -1)
    {
        // Dont give fileRef only the lock
        *fileLock = fileRef->lock;
    }
    lock->Release();

    return fileid;
}

bool FileTable::CloseFile(int fileid)
{
    lock->Acquire();
    if (!files->HasKey(fileid))
    {
        lock->Release();
        return false;
    }
    FileRef *fileRef = files->Get(fileid);
    fileRef->refCount--;
    DEBUG('j', "the refcount of %u is %u\n", fileid,fileRef->refCount);
    if (fileRef->refCount == 0)
    {
        files->Remove(fileid);
        lock->Release();
        bool b = fileRef->toDelete;
        if (b)
        {
            delete fileRef;
        }
        return b;
    }
    lock->Release();
    return false;
}

bool FileTable::SetRemove(unsigned sector)
{
    lock->Acquire();
    int fileid = findFileRef(sector);
    if (fileid == -1)
    {
        lock->Release();
        // Could be remove because it isn't open
        return true;
    }
    FileRef *fileRef = files->Get(fileid);
    fileRef->toDelete = true;
    lock->Release();
    // If it's on the table is because fileRef->refCount>0
    return false;
}

const char *FileTable::GetFileName(int fileid)
{
    lock->Acquire();
    if (files->HasKey(fileid))
    {
        FileRef *fileRef = files->Get(fileid);
        lock->Release();
        return fileRef->name;
    }
    lock->Release();
    return nullptr;
}

int FileTable::GetFileSector(int fileid)
{
    lock->Acquire();
    if (files->HasKey(fileid))
    {
        FileRef *fileRef = files->Get(fileid);
        lock->Release();
        return fileRef->sector;
    }
    lock->Release();
    return -1;
}
