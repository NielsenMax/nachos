#ifndef FILE_TABLE__HH
#define FILE_TABLE__HH

#include "rwlock.hh"
#include "lib/table.hh"
#include "threads/lock.hh"


class FileRef {
public:
    unsigned sector;
    unsigned refCount;
    const char* name;
    bool toDelete;

    char* nameLock;
    RWLock* lock;

    FileRef(unsigned sector, const char* name_);
    ~FileRef();
};

class FileTable {
public:
    FileTable();
    ~FileTable();

    // Return the global fileid, the fileLock parameter is to get the rwlock
    // of the file
    int OpenFile(unsigned sector, const char* name,RWLock** fileLock = nullptr);

    // Return true if the file should be removed
    bool CloseFile(int fileid);

    // Return true if the file should be removed
    bool SetRemove(unsigned sector);

    const char* GetFileName(int fileid);
    int GetFileSector(int fileid);

private:
    int findFileRef(unsigned sector);

    char* nameLock;
    Lock* lock;
    Table<FileRef*>* files;
};

#endif