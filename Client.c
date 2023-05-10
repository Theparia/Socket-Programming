#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>

char question[1024] = {0};
char answers[2][1024] = {0};

int connectServer(int port)
{
    int fd;
    struct sockaddr_in server_address;

    fd = socket(AF_INET, SOCK_STREAM, 0);

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) // checking for errors
    {
        printf("Error in connecting to server\n");
    }

    return fd;
}

void alarm_handler(int signal)
{
    printf("tick tock!\n");
}

char *decodeBCMessage(char buff[], char message[])
{
    char buff_copy[1024] = {0};
    strcpy(buff_copy, buff);
    if (buff[2] == 'S')
        strcat(message, "Best Answer: ");
    else if (buff[2] == 'Q')
        strcat(message, "Question: ");
    else if (buff[2] == 'A' && buff[3] == '1')
        strcat(message, "Answer 1: ");
    else if (buff[2] == 'A' && buff[3] == '2')
        strcat(message, "Answer 2: ");
    char *token = strtok(buff_copy, ":");
    token = strtok(NULL, "\n");
    strcat(message, token);
    strcat(message, "\n");
    return message;
}

/*Broadcasting Coding:   

    0)Q-0:
    0)A1-1:
    0)A2-2:
    0)S:1/2

    1)Q-1:
    1)A1-2:
    1)A2-0:
    1)S:1/2

    2)Q-2:
    2)A1-0:
    2)A2-1:
    2)S:1/2
*/

char *codeBCMessage(char buff_prev[], char buff_curr[], int id, char message[])
{
    char fd[1024];
    sprintf(fd, "%d", id);

    if (buff_prev[2] == 'Q' && ((buff_prev[0] - '0' + 1) % 3) == id)
    {
        strncat(message, &buff_prev[0], 1);
        strcat(message, ")A1-");
        strcat(message, fd);
        strcat(message, ":");
    }
    else if (buff_prev[2] == 'A' && buff_prev[3] == '1' && ((buff_prev[5] - '0' + 1) % 3) == id)
    {
        strncat(message, &buff_prev[0], 1);
        strcat(message, ")A2-");
        strcat(message, fd);
        strcat(message, ":");
    }
    else if (buff_prev[2] == 'A' && buff_prev[3] == '2' && ((buff_prev[0] - '0') % 3) == id)
    {
        strncat(message, &buff_prev[0], 1);
        strcat(message, ")S:");
    }
    else if (buff_prev[2] == 'S' && ((buff_prev[0] - '0' + 1) % 3) == id)
    {
        strcat(message, fd);
        strcat(message, ")Q-");
        strcat(message, fd);
        strcat(message, ":");
    }
    strcat(message, buff_curr);
    return message;
}

int main(int argc, char const *argv[])
{
    int fd, id, sock, broadcast = 1, opt = 1, port, server_port;
    struct sockaddr_in bc_address;
    char buff[1024] = {0}, field[1024] = {0};
    char working_buff[1024] = "0)Q-0:";
    server_port = atoi(argv[1]);
    fd = connectServer(server_port);

    read(0, field, 1024);
    send(fd, field, strlen(field), 0);
    recv(fd, buff, 1024, 0);
    id = atoi(buff);
    printf("You have entered the room!\n");
    memset(buff, 0, 1024);
    recv(fd, buff, 1024, 0);
    port = atoi(buff);
    

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    bc_address.sin_family = AF_INET;
    bc_address.sin_port = htons(port);
    bc_address.sin_addr.s_addr = inet_addr("192.168.1.255");

    bind(sock, (struct sockaddr *)&bc_address, sizeof(bc_address));

    memset(buff, 0, 1024);

    if (id == 0)
    {
        printf("Your Turn!\n");
        read(0, buff, 1024);
        strcat(working_buff, buff);
        sendto(sock, working_buff, strlen(working_buff), 0, (struct sockaddr *)&bc_address, sizeof(bc_address));
    }

    while (1)
    {
        memset(buff, 0, 1024);
        memset(working_buff, 0, 1024);
        recv(sock, buff, 1024, 0);
        printf("%s", decodeBCMessage(buff, working_buff));

        if ((buff[0] - '0') == id)
        {
            memset(working_buff, 0, 1024);

            if (buff[2] == 'Q')
                strcpy(question, decodeBCMessage(buff, working_buff));

            else if (buff[2] == 'A')
                strcpy(answers[buff[3] - '0' - 1], decodeBCMessage(buff, working_buff));

            else if (buff[2] == 'S')
            {
                char best_answer[1024] = "* ";
                strcat(best_answer, answers[buff[4] - '0' - 1]);
                strcpy(answers[buff[4] - '0' - 1], best_answer);
                char round[1024] = {0};
                strcpy(round, field);
                strcat(round, question);
                strcat(round, answers[0]);
                strcat(round, answers[1]);
                strcat(round, "\n");
                send(fd, round, strlen(round), 0);
            }
        }

        if (buff[0] == '2' && buff[2] == 'S')
        {
            close(fd);
            break;
        }

        if ((buff[2] == 'Q' && ((buff[0] - '0' + 1) % 3) == id) ||
            (buff[2] == 'A' && buff[3] == '1' && ((buff[5] - '0' + 1) % 3) == id) ||
            (buff[2] == 'A' && buff[3] == '2' && ((buff[0] - '0') % 3) == id) ||
            (buff[2] == 'S' && ((buff[0] - '0' + 1) % 3) == id))
        {
            printf("Your Turn!\n");
            char message[1024] = {0};
            memset(working_buff, 0, 1024);

            if ((buff[2] == 'Q' && (buff[0] - '0' + 1) % 3 == id) ||
                (buff[2] == 'A' && buff[3] == '1' && (buff[5] - '0' + 1) % 3 == id))
            {
                signal(SIGALRM, alarm_handler);
                siginterrupt(SIGALRM, 1);
                alarm(60);
                int read_ret = read(0, message, 1024);
                alarm(0);
                if (read_ret == -1)
                {
                    strcpy(message, "Times Up!");
                }
            }
            else
                read(0, message, 1024);

            strcpy(buff, codeBCMessage(buff, message, id, working_buff));
            sendto(sock, buff, strlen(buff), 0, (struct sockaddr *)&bc_address, sizeof(bc_address));
        }
    }
}
