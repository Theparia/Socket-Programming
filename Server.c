#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <string.h>

struct Room
{
    int client_fds[3];
    int size;
};

int port_maker = 8080;

struct Room rooms[4]; //Room[0] ---> Computer
                      //Room[1] ---> Electric
                      //Room[2] ---> Civil
                      //Room[3] ---> Mechanic

void addClientToRoom(int client_fd, int field)
{
    char port[1024];
    rooms[field].client_fds[rooms[field].size++] = client_fd;

    if (rooms[field].size == 3)
    {
        sprintf(port, "%d", port_maker++);
        send(rooms[field].client_fds[0], "0", 1024, 0);
        send(rooms[field].client_fds[0], port, 1024, 0);
        send(rooms[field].client_fds[1], "1", 1024, 0);
        send(rooms[field].client_fds[1], port, 1024, 0);
        send(rooms[field].client_fds[2], "2", 1024, 0);
        send(rooms[field].client_fds[2], port, 1024, 0);
        rooms[field].size = 0;
        memset(port, 0, 1024);
    }
}

int setupServer(int port)
{
    struct sockaddr_in address;
    int server_fd;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));

    listen(server_fd, 2);

    return server_fd;
}

int acceptClient(int server_fd)
{
    char buffer[1024] = {0};
    int client_fd;
    struct sockaddr_in client_address;
    int address_len = sizeof(client_address);
    client_fd = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t *)&address_len);
    recv(client_fd, buffer, 1024, 0);
    addClientToRoom(client_fd, atoi(buffer));
    return client_fd;
}

void parse(char buffer[], char token[])
{
    for (int i = 2; i < strlen(buffer); i++)
        token[i - 2] = buffer[i];
}

int main(int argc, char const *argv[])
{
    int server_fd, new_socket, max_sd, file_fd, server_port;
    char buffer[1024] = {0};
    fd_set master_set, working_set;
    server_port = atoi(argv[1]);
    server_fd = setupServer(server_port);

    FD_ZERO(&master_set);
    max_sd = server_fd;
    FD_SET(server_fd, &master_set);

    write(1, "Server is running\n", 18);

    while (1)
    {
        working_set = master_set;
        select(max_sd + 1, &working_set, NULL, NULL, NULL);

        for (int i = 0; i <= max_sd; i++)
        {
            if (FD_ISSET(i, &working_set))
            {

                if (i == server_fd) // new clinet
                {
                    new_socket = acceptClient(server_fd);
                    FD_SET(new_socket, &master_set);
                    if (new_socket > max_sd)
                        max_sd = new_socket;
                    printf("New client connected. fd = %d\n", new_socket);
                }

                else // client sending msg
                {
                    int bytes_received = recv(i, buffer, 1024, 0);

                    if (bytes_received == 0) // EOF
                    {
                        printf("client fd = %d closed\n", i);
                        close(i);
                        FD_CLR(i, &master_set);
                        continue;
                    }

                    if (buffer[0] == '0')
                        file_fd = open("computer.txt", O_CREAT | O_APPEND | O_RDWR, 0644);
                    else if (buffer[0] == '1')
                        file_fd = open("electric.txt", O_CREAT | O_APPEND | O_RDWR, 0644);
                    else if (buffer[0] == '2')
                        file_fd = open("civil.txt", O_CREAT | O_APPEND | O_RDWR, 0644);
                    else if (buffer[0] == '3')
                        file_fd = open("mechanic.txt", O_CREAT | O_APPEND | O_RDWR, 0644);

                    char token[1024] = {0};
                    parse(buffer, token);
                    write(file_fd, token, strlen(token));
                    memset(buffer, 0, 1024);
                }
            }
        }
    }

    return 0;
}