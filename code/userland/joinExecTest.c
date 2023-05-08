/// Simple program to test whether running a user program works.
///
/// Just do a “syscall” that shuts down the OS.
///
/// NOTE: for some reason, user programs with global data structures
/// sometimes have not worked in the Nachos environment.  So be careful out
/// there!  One option is to allocate data structures as automatics within a
/// procedure, but if you do this, you have to be careful to allocate a big
/// enough stack to hold the automatics!

#include "syscall.h"

int
main(void)
{
    Write("Calling exec\n", 13, CONSOLE_OUTPUT);
    SpaceId thread = Exec("../userland/auxTest", 0, 1);
    // Join(Exec("../userland/auxTest", 0, 1));
    Write("Returning from the exec\n", 24, CONSOLE_OUTPUT);

    if(thread < 0){
        Write("Error creating thread\n", 22, CONSOLE_OUTPUT);
        Halt();
    }
    Write("Thread created\n", 15, CONSOLE_OUTPUT);
    Join(thread);

    OpenFileId o = Open("../userland/test.txt");
    if(o < 0){
        Write("Error file didnt exists\n", 24, CONSOLE_OUTPUT);
        Halt();
    }
    char aux[64];
    int len = Read(aux, 64, o);
    Write(aux, len, CONSOLE_OUTPUT);
    Write("\n", 1, CONSOLE_OUTPUT);



    // Hopefully reached.
    Write("This should be printed.\n", 28, CONSOLE_OUTPUT);
    Halt();
}