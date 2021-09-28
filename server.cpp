#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <unistd.h>
#include "client_server.h"


/*
 * TODO:
 * CLIENTE:
 *      [X] - Arrumar a Interface do servidor
 *      [X] - Implementar loop principal do servidor
 *      [X] - Implementar funcionalidades do cliente (follow e send)
 *           [x] - Follow: Pede ao servidor para seguir o usuário
 *           [X] - Send: Mandar mensagem pra todos os followers do usuário atual
 *                  [X] - Manda uma mensagem para o servidor
 *      [ ] - Escutar as mensagens do servidor
 * SERVIDOR:
 * [ ] - Implementar funcionalidades do servidor (follow e send)
 *      [X] - Verificar se existe request do usuário
 *      [ ] - Decodificar qual o request:
 *          [ ] - Enfileirar requests para que tenham uma ordem
 *      [ ] - Implementar Follow:
 *          [ ] - Atualizar o registro de followers do usuário citado no request
 *                  Com o usuário atual
 *      [ ] - Implementar Send:
 *          [ ] - Pegar todos os Followers do usuário que esta fazendo o request
 *          [ ] - Colocar as informações na notificação -> quem tem que receber
 *          [ ] - Adicionar essas notificações como não recebidas pelos followers <usuario id>
 *          [ ] - Enviar a mensagem para os followers
 *          [ ] - Remover das não recebidas
 *      [ ] - Guardar informações em um arquivo
 */


const int PORT = 8080;
using namespace std;


Connection startServer(int port)
{
    //Start the server using the port especified, and return the server socket
    int server_sk = socket(AF_INET, SOCK_STREAM, 0);
    Connection c;

    socklen_t clilen;

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

    c.socklen = sizeof(struct sockaddr_in);
    c.socket = server_sk;
    c.addr = cli_addr;

    return c;
}


int findProfileByName(string profile_name, vector<Profile>& profiles)
{
    //Return the position of a profile in the profile vector
    for(int i = 0; i < profiles.size(); ++i)
    {
        if(profiles[i].profile_name == profile_name)
            return i;
    }
    return -1;
}


int createNewUser(string profile_name, vector<Profile>& profiles)
{
    //Create a new user in the profiles vector and return its position
    Profile new_profile;
    new_profile.profile_name = profile_name;
    new_profile.sessions_number = 1;

    //Push to the last position
    profiles.push_back(new_profile);

    return profiles.size() - 1;
}

int requestProfile(int socket, vector<Profile>& profiles)
{
    packet pkc;
    string profile_name;
    bzero(&pkc, sizeof(pkc));
    int n = read(socket, &pkc, sizeof(pkc));
    if (n < 0)
    {
        perror("Error reading from socket");
        exit(1);
    }

    if(pkc.type == PROFILE)
    {
        profile_name = pkc.payload;

        int position = findProfileByName(profile_name, profiles);
        if(position != -1)
        {
            if(profiles[position].sessions_number <= 1)
            {
                cout << "Logged: " << profiles[position].profile_name << endl;
                profiles[position].sessions_number ++;
                return position;
            }
            else
            {
                cout << "Error no login: Já possui muitas sessões ativas" << endl;
                return -1;
            }
        }
        else
        {
            position = createNewUser(profile_name, profiles);
            if(position > -1)
            {
                cout << "Created new user:" << profiles[position].profile_name << endl;
                return position;
            }
            else
            {
                cout << "Error: Failed to create new user" << endl;
                return -1;
            }
        }
    }
    return -1;
}

int sendConfirmation(int socket)
{
    time_t now = time(NULL);
    tm * ptm = localtime(&now);
    // Send the profile name trought to the server
    packet pkt;
    bzero(&pkt, sizeof (pkt));
    pkt.type = PROFILE;
    strcpy(pkt.payload, "You are ok");
    strftime(pkt.timestamp, sizeof(pkt.payload), "H%:M%:S%",ptm);
    pkt.length = sizeof(pkt.payload);
    pkt.seqn = 1;

    return write(socket, &pkt, sizeof(pkt));
}


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


void* listenClient(void * client_ptr)
{
    Client *client = (Client *) client_ptr;
    int position = client->profile_number;

    bool piped = true;

    while(piped)
    {
        packet pkc;
        string profile_name;
        bzero(&pkc, sizeof(pkc));
        int n = read(client->socket, &pkc, sizeof(pkc));
        cout << pkc.payload;

        if(n == 0)
            piped = false;
    }

    close(client->connection.socket);

    return client_ptr;
}

void* messageClient(void * client_ptr)
{
    Client *client = (Client *) client_ptr;
    while(1)
    {
    }

    close(client->connection.socket);
    return client_ptr;
}

void createThreads(Client *c)
{
    pthread_t *producer_ptr, *consumer_ptr;

    c->consumer = (pthread_t*) malloc(sizeof(pthread_t));
    c->producer = (pthread_t*) malloc(sizeof(pthread_t));

    consumer_ptr= (pthread_t*) malloc(sizeof(pthread_t));
    producer_ptr= (pthread_t*) malloc(sizeof(pthread_t));

    c->consumer = consumer_ptr;
    c->producer = producer_ptr;

    cout << "Creating threads ... " << endl;
    pthread_create(producer_ptr, NULL, listenClient, c);
    pthread_create(consumer_ptr, NULL, messageClient, c);
};

int main (int argc, char* argv [])
{
    vector<Profile> profiles;
    Connection c;

    //int server_socket = startServer(port);

    int new_sock_fd;
    pair<int,socklen_t> socket;
    c = startServer(PORT);


    while(1)
    {
        if ((new_sock_fd = accept(c.socket , (struct sockaddr *) &c.addr, &c.socklen)) == -1)
        {
            perror("Accept ERROR");
            exit(EXIT_FAILURE);
        }
        Client actual_client;
        actual_client.connection.socket = new_sock_fd;
        actual_client.profile_number = requestProfile(new_sock_fd, profiles);
        if(actual_client.profile_number >= 0)
        {
            sendConfirmation(new_sock_fd);
            actual_client.profile_ptr = &profiles;
            actual_client.socket = new_sock_fd;
            createThreads(&actual_client);
        }
    }
    close(new_sock_fd);

    return 0;
}
