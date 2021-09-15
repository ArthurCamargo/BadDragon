#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 


const int port = 8080;


int main (int argc, char *argv[])
{
    int client_sk;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];

    if (argc < 2) {
        perror("Error, no such host\n");
        exit(EXIT_FAILURE);
    }

    server = gethostbyname(argv[1]);

    if(server == NULL)
    {
        perror("Error, no such host");
        exit(EXIT_FAILURE);
    }


    if ((client_sk = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Error opening socket\n");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
    bzero(&(serv_addr.sin_zero), 8);

    if (connect(client_sk, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Error connecting\n");
        exit(EXIT_FAILURE);
    }

    std::cout << "Enter the message: ";
    bzero(buffer, 256);
    fgets(buffer, 256, stdin);

    int n = write(client_sk, buffer, strlen(buffer));

    if(n < 0)
    {
        perror("Error reading from socket\n");
    }

    fgets(buffer, 256, stdin);

    close(client_sk);

    return 0;
}
