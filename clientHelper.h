/*
Nhập từ bàn phím không yêu cầu enter để tiếp tục.
*/ 
void setCustomTerminal();

void setDefaultTerminal();

/*
Lấy password
*/
int getPassword(char pass[]);

// clear screen
void clearScreen();

int getCode(char *buffer);

char *getMessage(char *buffer, char *outMessage);

void pressEnterToContinue();

void clearBuffer();