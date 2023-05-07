#include "syscall.h"

unsigned
strlen(const char *s)
{
  unsigned i;
  for (i = 0; s[i] != '\0'; i++)
    ;

  return i;
}

void puts_lib(const char *s)
{
  Write(s, strlen(s), CONSOLE_OUTPUT);
  Write("\n", 1, CONSOLE_OUTPUT);
}

void reverse_lib(char *str, int len)
{
  int i, j;
  char temp;
  for (i = 0, j = len - 1; i < j; i++, j--)
  {
    temp = str[i];
    str[i] = str[j];
    str[j] = temp;
  }
}

void itoa(int n, char *str)
{
  int i = 0, sign;
  if ((sign = n) < 0)
  {
    n = -n;
  }

  do
  {
    str[i++] = n % 10 + '0';
  } while ((n /= 10) > 0);

  if (sign < 0)
  {
    str[i++] = '-';
  }

  str[i] = '\0';
  reverse(str, i);
}