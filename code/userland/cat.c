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

  if (argc != 2)
  {
    puts_lib("Error: wrong amount of arguments.\n");
    Exit(-1);
  }

  OpenFileId fid = Open(argv[1]);

  if (fid < 2)
  {
    puts_lib("Error: could not open the file.\n");
    Exit(-1);
  }

  char buffer[1];
  while (Read(buffer, 1, fid))
  {
    Write(buffer, 1, CONSOLE_OUTPUT);
  }
  Write("\n", 1, CONSOLE_OUTPUT);

  Close(fid);

  Exit(0);

  return 0;
}