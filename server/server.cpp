#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <unistd.h>
#include "server.h"

const int port = 8080;


using namespace std;

int main (int argc, char* argv [])
{
    int server_sk = socket(AF_INET, SOCK_STREAM, 0);
    int opt;

    socklen_t clilen;
    int new_sock_fd;

    char buffer[256];

    struct sockaddr_in serv_addr, cli_addr;

    if(server_sk == -1)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(serv_addr.sin_zero), 8);

    if (bind(server_sk, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Binding ERROR");
        exit(EXIT_FAILURE);
    }

    listen(server_sk, 5);


    clilen = sizeof(struct sockaddr_in);

    if ((new_sock_fd = accept(server_sk, (struct sockaddr *) &cli_addr, &clilen)) == -1)
    {
        perror("Accept ERROR");
        exit(EXIT_FAILURE);
    }

    bzero(buffer, 256);


    int n = read(new_sock_fd, buffer, 256);
    if (n < 0)
    {
        perror("ERROR reading from socket");
    }

    cout << "Here is the message:" << buffer << std::endl;

    n = write(new_sock_fd, "I got your message", 18);

    if (n < 0)
    {
        perror("ERROR writing from socket");
    }

    close(new_sock_fd);
    close(server_sk);

    return 0;
}
