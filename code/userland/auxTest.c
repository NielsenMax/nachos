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
    Create("../userland/test.txt");
    OpenFileId o = Open("../userland/test.txt");
    Write("Open file\n", 10, CONSOLE_OUTPUT);
    Write("Estoy escribiendo un archivo\n", 29, o);
    Close(o);

    Ps();

    // Halt();
    Exit(1);
}