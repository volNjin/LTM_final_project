#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

/*
Lấy password
*/
int getPassword(char pass[])
{
  int i = 0;
  while ((pass[i] = getchar()) != '\n')
  {
    i++;
  }
  pass[i] = '\0';
  return i;
}

/*
clear screen
*/
void clearScreen()
{
  printf("%c[2J%c[;H", (char)27, (char)27);
}

int getCode(char *buffer)
{
  int code;
  sscanf(buffer, "%d", &code);
  return code;
}

char *getMessage(char *buffer, char *outMessage)
{
  sscanf(buffer, "%*d %[^\t\n]", outMessage);
}

void pressEnterToContinue()
{
  printf("Press Enter to continue");
  getchar();
}

void clearBuffer()
{
  int c;
  while (c != '\n' && c != EOF)
    c = getchar();
}