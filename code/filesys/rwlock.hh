#ifndef RW_LOCK__HH
#define RW_LOCK__HH

class Lock;
class Semaphore;

class RWLock {
public:
    RWLock(char *name);
    ~RWLock();

    void RAcquire();
    void RRelease();
    void Acquire();
    void Release();
private:
    char *name;

    char* nameReadersLock;
    Lock *readersLock;
    unsigned readers;

    // Use for locking
    char* nameInUse;
    Semaphore *inUse;

    // Prevents new readers when a writer is present
    char* nameWantsToWrite;
    Semaphore *wantsToWrite;
};


#endif