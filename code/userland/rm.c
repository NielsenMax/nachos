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

  if (argc != 2)
  {
    puts_lib("Error: wrong amount of arguments.\n");
    Exit(-1);
  }

  int value = Remove(argv[1]);

  if (value == 0)
  {
    puts_lib("File removed succesfully\n");
  }
  else
  {
    puts_lib("Could not remove file\n");
  }

  Exit(0);
  return 0;
}