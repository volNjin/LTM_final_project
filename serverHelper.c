#include <stdio.h>
#include <string.h>

/*
Kiểm tra username, password
Output: 1 - user, pass hợp lệ; 0 - không hợp lệ
*/
int isValid(char *username, char *password)
{
  FILE *f = fopen("user.txt", "r+");
  if (f == NULL)
  {
    printf("Error open file!!!\n");
    return 0;
  }
  char line[100];
  char *temp;
  if (password != NULL)
  {
    while (fgets(line, 100, f) != NULL)
    {
      temp = line;
      while (temp[0] != '#')
        temp++; // get user pass, gap # thi dung
      temp[0] = '\0';
      temp++;
      if (temp[strlen(temp) - 1] == '\n')
        temp[strlen(temp) - 1] = '\0';
      if (strcmp(line, username) == 0 && strcmp(temp, password) == 0)
      {
        fclose(f);
        return 1;
      }
    }
    fclose(f);
    return 0;
  }
  else
  {
    while (fgets(line, 100, f) != NULL)
    {
      temp = line;
      while (temp[0] != '#')
        temp++;
      temp[0] = '\0';
      if (strcmp(line, username) == 0)
      {
        fclose(f);
        return 1;
      }
    }
    fclose(f);
    return 0;
  }
}

/*
Đăng kí user mới
*/
void registerUser(char *username, char *password)
{
  FILE *f = fopen("user.txt", "a");
  fprintf(f, "%s#%s\n", username, password);
  fclose(f);
}