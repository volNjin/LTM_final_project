#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#include "checkinput.h"
#include "serverHelper.h"
#define SIZE 12
#define TIMED_OUT 20

int Port;
int *client = NULL;
char **usernameList = NULL;
int *opponentList = NULL; // store index of client[]
char ***boardList = NULL;
int count = 0;
char *token = "#";

pthread_mutex_t mutex;
int *findingQueue = NULL; // store index of client[]
int queue_size = 0;
void enqueue(int index);
int dequeue();

char WELCOME[] = "200 Caro Game ready\n";
char LOGGED_IN_ACC[] = "110 This account is already in use\n";
char FIND_TIMED_OUT[] = "120 Cannot find any player\n";
char REGISTER_SUCCESS[] = "201 Register success\n";
char REGISTER_FAIL[] = "202 Register fail\n";
char LOGIN_SUCCESS[] = "211 Login success\n";
char LOGIN_FAIL[] = "212 Login fail\n";
char FIND_SUCCESS_X[64] = "220 %s - you play X\n"; // %s: opponent username
char FIND_SUCCESS_O[64] = "221 %s - you play O\n"; // %s: opponent username
char MOVE_SUCCESS[] = "230 Move success\n";
char OPP_MOVE[32] = "330 %d %d Opponent move\n"; // %d %d: opponent move's coordinate
char YOU_WON[] = "341 You won\n";
char OPP_WON[] = "342 %d %d Opponent won\n"; // %d %d: opponent O move's coordinate
char OPP_DISCONNECTED[] = "500 Opponent disconnected\n";

void enqueue(int index)
{
    findingQueue = (int *)realloc(findingQueue, (queue_size + 1) * sizeof(int));
    findingQueue[queue_size] = index;
    queue_size++;
}

int dequeue()
{
    int top = *findingQueue;
    memmove(findingQueue, findingQueue + 1, (queue_size - 1) * sizeof(int));
    findingQueue = (int *)realloc(findingQueue, (queue_size - 1) * sizeof(int));
    queue_size--;
    return top;
}

char *rtrim(char *s)
{
    char *back = s + strlen(s) - 1;
    while (*back == ' ' || *back == '\n' || *back == '\r')
    {
        back--;
    }
    *(back + 1) = '\0';
    return s;
}

int check_win(char **board, int x, int y)
{
    int count;
    int i, j;
    char role = board[x][y];
    char oppRole;
    if (role == 'X')
        oppRole = 'O';
    else
        oppRole = 'X';
    // check horizontal
    i = x - 1, j = y, count = 1;
    while (i >= 0 && board[i--][j] == role)
        count++;
    i = x + 1;
    while (i < SIZE && board[i++][j] == role)
        count++;
    if (count >= 5 && board[i-1][j] != oppRole && board[i-7][j] != oppRole)
        return 1;

    // check vertical
    i = x, j = y - 1, count = 1;
    while (j >= 0 && board[i][j--] == role)
        count++;
    j = y + 1;
    while (j < SIZE && board[i][j++] == role)
        count++;
    if (count >= 5 && board[i][j-1] != oppRole && board[i][j-7] != oppRole)
        return 1;

    // check diagonal '\'
    i = x - 1, j = y - 1, count = 1;
    while (i >= 0 && j >= 0 && board[i--][j--] == role)
        count++;
    i = x + 1, j = y + 1;
    while (i < SIZE && j < SIZE && board[i++][j++] == role)
        count++;
    if (count >= 5 && board[i-1][j-1] != oppRole && board[i-7][j-7] != oppRole)
        return 1;

    // check diagonal '/'
    i = x - 1, j = y + 1, count = 1;
    while (i >= 0 && j < SIZE && board[i--][j++] == role)
        count++;
    i = x + 1, j = y - 1;
    while (i < SIZE && j >= 0 && board[i++][j--] == role)
        count++;
    if (count >= 5 && board[i-1][j+1] != oppRole && board[i-7][j+7] != oppRole)
        return 1;

    return 0;
}

void *thread_proc(void *arg)
{
    int index = *((int *)arg);
    int cfd = client[index];
    char buffer[1024];
    int opp_index;

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        int r = recv(cfd, buffer, sizeof(buffer), 0);
        if (r <= 0)
            break;

        strcpy(buffer, rtrim(buffer));
        printf("%s\n", buffer);

        if (strncmp(buffer, "REGISTER ", 9) == 0)
        {
            char *request = strtok(buffer, " ");
            char *username = strtok(NULL, token);
            char *password = strtok(NULL, token);

            if (isValid(username, NULL))
            {
                printf("%s", REGISTER_FAIL);
                send(cfd, REGISTER_FAIL, sizeof(REGISTER_FAIL), 0);
            }
            else
            {
                registerUser(username, password);
                printf("%s", REGISTER_SUCCESS);
                send(cfd, REGISTER_SUCCESS, sizeof(REGISTER_SUCCESS), 0);
            }
        }
        else if (strncmp(buffer, "LOGIN ", 6) == 0)
        {
            char *request = strtok(buffer, " ");
            char *username = strtok(NULL, token);
            char *password = strtok(NULL, token);
            if (isValid(username, password))
            {
                int isNameDuplicate = 0;
                for (int i = 0; i < count; i++)
                {
                    if (usernameList[i] == NULL)
                        continue;
                    if (strcmp(username, usernameList[i]) == 0)
                    {
                        printf("%s", LOGGED_IN_ACC);
                        send(cfd, LOGGED_IN_ACC, strlen(LOGGED_IN_ACC), 0);
                        isNameDuplicate = 1;
                        break;
                    }
                }
                if (isNameDuplicate)
                    continue;
                usernameList[index] = (char *)realloc(usernameList[index], strlen(username) + 1);
                strcpy(usernameList[index], username);
                send(cfd, LOGIN_SUCCESS, sizeof(LOGIN_SUCCESS), 0);
            }
            else
            {
                send(cfd, LOGIN_FAIL, sizeof(LOGIN_FAIL), 0);
            }
        }
        else if (strncmp(buffer, "FIND", 4) == 0)
        {
            pthread_mutex_lock(&mutex);
            enqueue(index);
            pthread_mutex_unlock(&mutex);

            time_t start_time = time(NULL);
            while (time(NULL) - start_time < TIMED_OUT)
            {
                if (opponentList[index] != -1)
                    break;
                sleep(1);
            }

            // timed out on finding
            if (opponentList[index] == -1)
            {
                int recheck_after_lock_flag = 0;
                pthread_mutex_lock(&mutex);
                // check again in case player found right before lock, otherwise the current client is still top of queue
                if (opponentList[index] == -1)
                {
                    recheck_after_lock_flag = 1;
                    dequeue();
                    send(cfd, FIND_TIMED_OUT, strlen(FIND_TIMED_OUT), 0);
                }
                pthread_mutex_unlock(&mutex);
                if (recheck_after_lock_flag)
                    continue;
            }

            // opponent found
            opp_index = opponentList[index];

            char find_resp[64];
            // Create the board

            char **board = (char **)calloc(SIZE, sizeof(char *));
            for (int i = 0; i < SIZE; i++)
            {
                board[i] = (char *)calloc(SIZE, sizeof(char));
            }

            for (int i = 0; i < SIZE; i++)
            {
                for (int j = 0; j < SIZE; j++)
                {
                    board[i][j] = ' ';
                }
            }

            boardList[index] = boardList[opp_index] = board;
            srand(time(0));
            int coin;
            if (index < opp_index)
            {
                sprintf(find_resp, FIND_SUCCESS_X, usernameList[opp_index]);
                send(cfd, find_resp, strlen(find_resp), 0);
            }
            else
            {
                sprintf(find_resp, FIND_SUCCESS_O, usernameList[opp_index]);
                send(cfd, find_resp, strlen(find_resp), 0);
            }
        }
        else if (strncmp(buffer, "MOVE ", 5) == 0)
        {
            if (opponentList[index] == -1)
                continue; // bugfix: opponent dc before sending MOVE

            int x, y;
            char role; // 'X' or 'O'
            char opp_move_resp[32];
            sscanf(buffer, "%*s%d%d", &x, &y);
            x--;
            y--;
            sprintf(opp_move_resp, OPP_MOVE, x, y);

            // Smaller index will be 'X
            if (index < opp_index)
                role = 'X';
            else
                role = 'O';
            boardList[index][x][y] = role;

            // check win
            if (check_win(boardList[index], x, y))
            {
                printf("WIN\n");
                char opp_won_resp[32];
                sprintf(opp_won_resp, OPP_WON, x, y);
                send(cfd, YOU_WON, strlen(YOU_WON), 0);
                send(client[opp_index], opp_won_resp, strlen(opp_won_resp), 0);

                // free memory
                opponentList[index] = opponentList[opp_index] = -1;
                free(boardList[index]);
                boardList[index] = boardList[opp_index] = NULL;
                continue;
            }

            send(client[opp_index], opp_move_resp, strlen(opp_move_resp), 0);
            send(cfd, MOVE_SUCCESS, strlen(MOVE_SUCCESS), 0);
        }
        else if (strncmp(buffer, "EXIT", 4) == 0)
        {
            break;
        }
    }

    free(arg);
    close(client[index]);
    client[index] = -1;
    free(usernameList[index]);
    usernameList[index] = NULL;

    if (opponentList[index] != -1)
    {
        send(client[opp_index], OPP_DISCONNECTED, strlen(OPP_DISCONNECTED), 0);
        opponentList[index] = opponentList[opp_index] = -1;
        free(boardList[index]); // boardList[opp_index] & boardList[index] point to the same location
    }

    return NULL;
}

void *pair_player_proc()
{
    while (1)
    {
        if (queue_size >= 2)
        { // reading queue_size dont need to lock since it can only increase - which is still correct
            pthread_mutex_lock(&mutex);
            int player1 = dequeue();
            int player2 = dequeue();
            pthread_mutex_unlock(&mutex);

            opponentList[player1] = player2;
            opponentList[player2] = player1;
        }
        sleep(1);
    }
}

void signal_handler(int sig)
{
    pthread_mutex_destroy(&mutex);
    free(client);
    client = NULL;
    count = 0;
    exit(0);
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Syntax Error.\n");
        printf("Syntax: ./server PortNumber\n");
        return 0;
    }
    if (check_port(argv[1]) == 0)
    {
        printf("Port invalid\n");
        return 0;
    }
    Port = atoi(argv[1]);
    signal(SIGINT, signal_handler);
    pthread_mutex_init(&mutex, NULL);

    pthread_t pair_tid;
    pthread_create(&pair_tid, NULL, &pair_player_proc, NULL);

    int sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in saddr;
    struct sockaddr caddr;
    int clen = sizeof(caddr);
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(Port);
    saddr.sin_addr.s_addr = 0;
    if (bind(sfd, (struct sockaddr *)&saddr, sizeof(struct sockaddr)) == -1)
    {
        perror("Unable to bind\n");
        exit(-3);
    }
    if (listen(sfd, 10) == -1)
    {
        perror("Listen error\n");
        exit(-4);
    }
    printf("Caro Game server waiting for client on port %d\n", Port);
    fflush(stdout);
    while (1)
    {
        int cfd = accept(sfd, (struct sockaddr *)&caddr, &clen);
        if (cfd >= 0)
        {
            send(cfd, WELCOME, strlen(WELCOME), 0);

            client = (int *)realloc(client, (count + 1) * sizeof(int));
            client[count] = cfd;
            usernameList = (char **)realloc(usernameList, (count + 1) * sizeof(char *));
            usernameList[count] = NULL;
            opponentList = (int *)realloc(opponentList, (count + 1) * sizeof(char *));
            opponentList[count] = -1;
            boardList = (char ***)realloc(boardList, (count + 1) * sizeof(char **));
            boardList[count] = NULL;

            pthread_t tid;
            int *arg = (int *)calloc(1, sizeof(int));
            *arg = count;
            pthread_create(&tid, NULL, &thread_proc, arg);
            count++;
        }
    }
}