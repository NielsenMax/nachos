#include "syscall.h"
#include "lib.c"

int main(int argc, char **argv)
{
  // char args_amnt[20];
  // puts_lib("Number of arguments:");
  // itoa(argc, args_amnt);
  // puts_lib(args_amnt);
  // puts_lib("Arguments:");
  // puts_lib(argv[0]);
  // puts_lib(argv[1]);
  // puts_lib(argv[2]);

  if (argc != 3)
  {
    puts_lib("Error: wrong amount of arguments.\n");
    Exit(-1);
  }
  OpenFileId fsource = Open(argv[1]);

  Create(argv[2]);
  OpenFileId ftarget = Open(argv[2]);

  if (fsource < 2 || ftarget < 2)
  {
    puts_lib("Error: could not open the file.\n");
    Exit(-1);
  }

  char buffer[1];
  while (Read(buffer, 1, fsource))
  {
    Write(buffer, 1, ftarget);
  }
  puts_lib("File copy succesfully\n");

  Close(fsource);
  Close(ftarget);

  Exit(0);
  return 0;
}