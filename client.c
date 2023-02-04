#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>

#include "checkinput.h"
#include "clientHelper.h"
#define SIZE 12

int sockfd, Port;
char *serverAddress;
char username[21];
char oppUsername[21];
char yourRole, oppRole; // 'X' or 'O'
char **board;
int gameOver = 1;

void clearBoard()
{
    for (int i = 0; i < SIZE; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            board[i][j] = ' ';
        }
    }
}

void draw_board()
{
    printf("  ");
    for (int i = 0; i < SIZE; i++)
        printf(" %-2d ", i + 1);
    printf("\n");

    for (int i = 0; i < SIZE; i++)
    {
        printf("%-2d", i + 1);
        for (int j = 0; j < SIZE; j++)
        {
            if (board[i][j] == yourRole)
                printf(" \033[0;33m%c\033[0m ", board[i][j]);
            else if (board[i][j] == oppRole)
                printf(" \033[0;31m%c\033[0m ", board[i][j]);
            else
                printf(" %c ", board[i][j]);
            if (j != SIZE - 1)
                printf("|");
        }
        printf("\n");

        if (i == SIZE - 1)
            break;
        printf("  ");
        for (int j = 0; j < SIZE; j++)
        {
            printf("---");
            if (j != SIZE - 1)
                printf("+");
        }
        printf("\n");
    }
}

void registerUser()
{
    char buffer[64];
    char message[64];
    char password[21];
    char confirmPassword[21];
    while (1)
    {
        clearScreen();
        printf("+-------Register-------+\n");
        printf("Username: ");
        fgets(username, 21, stdin);
        username[strlen(username) - 1] = '\0';

        printf("Password: ");
        getPassword(password);

        printf("Confirm password: ");
        getPassword(confirmPassword);
        // check password match confirmPassword
        if (strcmp(password, confirmPassword) == 0)
        {
            // register
            sprintf(buffer, "REGISTER %s#%s", username, password);
            send(sockfd, buffer, strlen(buffer), 0);
            memset(buffer, 0, sizeof(buffer));
            recv(sockfd, buffer, sizeof(buffer), 0);
            getMessage(buffer, message);
            printf("%s\n", message);
            pressEnterToContinue();
            if (getCode(buffer) == 201)
                break;
        }
        else
        {
            printf("%s\n", "Password does not match");
            pressEnterToContinue();
        }
    }
}
void login()
{
    char buffer[64];
    char message[64];
    char password[21];
    while (1)
    {
        clearScreen();
        printf("+-------Sign in------+\n");
        printf("Username: ");
        fgets(username, 21, stdin);
        username[strlen(username) - 1] = '\0';

        printf("Password: ");
        getPassword(password);

        // check username and password
        sprintf(buffer, "LOGIN %s#%s", username, password);

        send(sockfd, buffer, strlen(buffer), 0);
        memset(buffer, 0, sizeof(buffer));
        recv(sockfd, buffer, sizeof(buffer), 0);
        getMessage(buffer, message);

        if (getCode(buffer) == 211)
            break;
    }
}

// return respond code
int findPlayer()
{
    char buffer[64], request[64], message[64];
    strcpy(request, "FIND");
    send(sockfd, request, strlen(request), 0);

    fd_set read;
    struct timeval tv;
    int count_loop_finding = 0;
    while (1)
    {
        clearScreen();
        printf("Finding player ");
        for (int i = 0; i <= count_loop_finding % 3; i++)
            printf(".");
        printf("\n");
        fflush(stdout); // fixbug not print immediately
        count_loop_finding++;

        tv.tv_sec = 1;
        tv.tv_usec = 0;
        FD_ZERO(&read);
        FD_SET(sockfd, &read);
        select(FD_SETSIZE, &read, NULL, NULL, &tv);
        if (FD_ISSET(sockfd, &read))
        {
            recv(sockfd, buffer, sizeof(buffer), 0);
            sscanf(buffer, "%*d %s", oppUsername);
            return getCode(buffer);
        }
    }
}

void getOpponentMove(char *response)
{
    int code, x, y;
    sscanf(response, "%d%d%d", &code, &x, &y);
    board[x][y] = oppRole;
}

void getYourMove()
{
    char buffer[64];
    int x, y;
    printf("\nYour Turn (x y): ");
    while (1)
    {
        fgets(buffer, 64, stdin);
        if (sscanf(buffer, "%d%d", &x, &y) == 2)
        {
            x--;
            y--;
            if (0 <= x && x < SIZE && 0 <= y && y < SIZE && board[x][y] == ' ')
            {
                board[x][y] = yourRole;
                sprintf(buffer, "MOVE %d %d", x + 1, y + 1);
                send(sockfd, buffer, strlen(buffer), 0);
                break;
            }
        }
        printf("Invalid Move, try again: ");
    }
}

void newGame()
{
    int code;
    char buffer[64];
    int yourTurn;

    code = findPlayer();
    if (code == 120)
    {
        printf("TIMED OUT: Cannot find any player\n");
        pressEnterToContinue();
        return;
    }
    if (code == 220)
    {
        yourRole = 'X';
        oppRole = 'O';
        yourTurn = 1;
        printf("Player found: %s, you are X\n", oppUsername);
    }
    else if (code == 221)
    {
        yourRole = 'O';
        oppRole = 'X';
        yourTurn = 0;
        printf("Player found: %s, you are O\n", oppUsername);
    }
    printf("\n");
    pressEnterToContinue();

    clearBoard();
    gameOver = 0;
    while (!gameOver)
    {
        clearScreen();
        printf("You: %c\t\t%s: %c\n\n", yourRole, oppUsername, oppRole);
        draw_board();

        memset(buffer, 0, sizeof(buffer));
        if (yourTurn)
        {
            yourTurn = 0;
            getYourMove();
        }
        else
        {
            yourTurn = 1;
        }

        recv(sockfd, buffer, sizeof(buffer), 0);
        code = getCode(buffer);
        switch (code)
        {
        case 500:
            gameOver = 1;
            printf("Opponent disconnected\n");
            pressEnterToContinue();
            break;
        case 341: // You won
            gameOver = 1;
            // re-draw board
            clearScreen();
            printf("You: %c\t\t%s: %c\n\n", yourRole, oppUsername, oppRole);
            draw_board();

            printf("You won!\n");
            pressEnterToContinue();
            break;
        case 342: // Opponent won
            gameOver = 1;
            getOpponentMove(buffer);
            // re-draw board
            clearScreen();
            printf("You: %c\t\t%s: %c\n\n", yourRole, oppUsername, oppRole);
            draw_board();

            printf("%s won!\n", oppUsername);
            pressEnterToContinue();
            break;
        case 330:
            getOpponentMove(buffer);
            break;
        case 230:
            break;
        }
    }
}

int mainMenu()
{
    int option;
    clearScreen();
    printf("+-------Caro game server-------+\n");
    printf("\t1. Login\n");
    printf("\t2. Register\n");
    printf("\t3. Quit\n");
    printf("+------------------------------+\n");
    printf("\nSelect: ");
    while (1)
    {
        option = getchar();
        clearBuffer();
        if (option >= '1' && option <= '3')
            break;
        else
            printf("Invalid option, try again: ");
    }
    return option;
}

int getMenuOption()
{
    int option;
    clearScreen();
    printf("Logged in as %s\n", username);
    printf("-------- MENU --------\n");
    printf("1. New Game\n");
    printf("2. Quit\n");
    printf("\nSelect: ");
    while (1)
    {
        option = getchar();
        clearBuffer();
        if (option >= '1' && option <= '2')
            break;
        else
            printf("Invalid option, try again: ");
    }
    return option;
}

void start()
{
    while (1)
    {
        int option;
        option = mainMenu();
        if (option == '1')
        {
            login();
            while (1)
            {
                option = getMenuOption();
                if (option == '2')
                {
                    memset(username, 0, sizeof(username));
                    start();
                }

                newGame();
            }
        }
        else if (option == '2')
        {
            registerUser();

            login();
            while (1)
            {
                option = getMenuOption();
                if (option == '2')
                {
                    memset(username, 0, sizeof(username));
                    start();
                }

                newGame();
            }
        }
        else if (option == '3')
        {
            send(sockfd, "EXIT", 4, 0);
            exit(0);
            break;
        }
    }
}

int main(int argc, char **argv)
{

    if (argc != 3)
    {
        printf("Syntax Error.\n");
        printf("Syntax: ./client IPAddress PortNumber\n");
        return 0;
    }
    if (check_IP(argv[1]) == 0)
    {
        printf("IP address invalid\n");
        return 0;
    }
    if (check_port(argv[2]) == 0)
    {
        printf("Port invalid\n");
        return 0;
    }

    serverAddress = argv[1];
    Port = atoi(argv[2]);

    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(Port);
    saddr.sin_addr.s_addr = inet_addr(serverAddress);

    // init board
    board = (char **)calloc(SIZE, sizeof(char *));
    for (int i = 0; i < SIZE; i++)
    {
        board[i] = (char *)calloc(SIZE, sizeof(char));
    }

    int conn = connect(sockfd, (struct sockaddr *)&saddr, sizeof(saddr));
    if (conn < 0)
    {
        printf("Unable to connect the server\n");
        return -1;
    }

    char buffer[64];
    recv(sockfd, buffer, sizeof(buffer), 0);
    if (getCode(buffer) != 200)
        return -1;
    printf("Welcome To Caro Game\n");
    pressEnterToContinue();

    start();
    close(sockfd);
}