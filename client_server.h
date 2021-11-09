#pragma once

#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <queue>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <vector>
#include <pthread.h>
#include <time.h>
#include <fstream>
#include <sstream>
#include <vector>


const int TIMESTAMP_SIZE = 128;
const int MAX_SESSION = 2;
const int MAX_PROFILE_SIZE = 20;
const int MAX_MESSAGE_SIZE = 128;


typedef struct Connection
{
    char* ip;
    char* profile_name;
    int port;
    int socket;
    socklen_t socklen;
    sockaddr_in addr;
} connection_data;

class Server{

    public:
        int server_id;
        Server* neighbor;
        ClientData client_data;

        Server();
        Server(int new_server_id, Server* new_neighbor, ClientData new_client_data){
            server_id = new_server_id;
            neighbor = new_neighbor;
            client_data = new_client_data;
        }
};

class Notification
{
    static uint32_t count; // Number of Notifications
    std::string author;     // Author
    char* timestamp; // TimeStamp
    uint32_t length; // Body lenght
    uint32_t not_received; // Number of people pending
    const char*  body; // Body of the message
    uint32_t id; //Unique id

    public:
        Notification(const char* _body, std::string _author, uint32_t _not_received)
        {
            body = _body;
            author = _author;
            not_received = _not_received;

            id = count;
            length = strlen(body);

            count ++;
        }

        void reduceNotReceived()
        {
            not_received --;
        }

        uint32_t getId()
        {
            return id;
        }

        const char* getBody()
        {
            return body;
        }
};

class Device {
    public:
        int id;
        int socket;
        bool online; // If the device is current online

    Device() {};
    Device(const int &_id, const int& _socket, const int& _online)
    {
        id = _id;
        socket = _socket;
        online = _online;
    }
};

class Profile{
    // Dados do usuário
    public:
        std::string profile_name; //Nome do usuario na aplicacao @exemplo
        std::vector <uint32_t> followers; // vetor de ponteiro para os followers
        std::vector <Notification> notifications; //Vector of notifications of the user
        std::queue <std::pair<uint32_t, uint32_t>> pending_notifications; // Notificações pendentes
        std::vector<Device> devices; // devices connected to the user
        int sessions_number; // Numero de sessões já incializadas pelo usuário
};


class ClientData{
    //wrapper do cliente
    public:
        std::vector<Profile> profiles;
        connection_data connection;
        pthread_t* consumer;
        pthread_t* producer;
        int profile_number;
};

enum pkt_type
{
    NONE = 0,
    PROFILE = 1,
    DATA = 2,
    ACCEPTED = 3,
    FAILURE = 4,
    SEND = 5,
    FOLLOW = 6,
};

class Packet{
    public:
        enum pkt_type type;      //Tipo do pacote (p.ex. DATA | CMD)
        int32_t seqn;            //Número de sequência
        int32_t length;          //Comprimento do payload
        char payload[256];       //Dados da mensagem

        Packet()
        {
            seqn = 0;
            length = 0;
            type = NONE;
            bzero((char*)payload,length);
        }

        Packet(int32_t _seqn, char* _payload, int32_t _length, enum pkt_type _type)
        {
            //struct tm* time_manager;
            //time_t rawtime;
            //time(&rawtime);

            //time_manager = localtime(&rawtime);
            //std::cout << "Tempo" << std::endl;

            type = _type;
            length = _length;
            seqn = _seqn;

            //strftime(timestamp, 25, "%H:%M%p", time_manager);

            bzero((char*)payload,length);
            strcpy(payload, _payload);
        }

        Packet(int32_t _seqn, int32_t _length, enum pkt_type _type)
        {
            type = _type;
            length = _length;
            seqn = _seqn;

            bzero((char*)payload,length);
        }
};


int sendConfirmation(int socket, enum pkt_type type);
int saveProfiles(std::vector<Profile> &profiles);
void *listenServer(void *connection);
void *sendServer(void *connection);
int follow(char* profile_name, Connection c);
int post(char* message, Connection c);
int receiveConfirmation(int sk_fd);
void createServers(int server_number);
void* runServer(void* actual_client);
int sendProfile(Connection c);
int connectServer(Connection connection);
Connection parseOptions(int argc, char* argv[]);
Connection startConnection(int port);
std::vector<uint32_t> getFollowers(ClientData *client, int profile_number);
void queueMessage(ClientData *client, int profile_number, const char* message_data);
int loadProfiles(std::vector<Profile> &profiles);
int saveProfiles(std::vector<Profile> &profiles);
int followPersonByName(std::vector<Profile> &profiles, int profile_number, std::string profile_name);
void processRequest(ClientData *client, int profile_number, Packet pkt);
void* listenClient(void* client_ptr);
void sendPending(ClientData *client, int profile_number, int socket);
void* messageClient(void * client_ptr);
void createThreads(ClientData &c);

int findNotificationById(ClientData *client, int profile_number, int id);

void printUsers(ClientData *client);
void printPendingNotifications(ClientData *client);
