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

const int TIMESTAMP_SIZE = 128;
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

class Notification
{
    static uint32_t count; // Number of Notifications
    const char* author;     // Author
    char* timestamp; // TimeStamp
    uint32_t length; // Body lenght
    uint32_t not_received; // Number of people pending
    const char* body; // Body of the message
    uint32_t id; //Unique id

    public:
        Notification(char* _body, const char* _author, uint32_t _not_received)
        {
            body = _body;
            author = _author;
            not_received = _not_received;

            id = count;
            length = strlen(body);

            //time_t now = time(NULL);
            //tm * ptm = localtime(&now);
            //strftime(timestamp, sizeof(timestamp), "%H:%M:%S",ptm);

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

        char* getBody()
        {
            char* buffer;
            strcpy(buffer,body);
            return buffer;
        }
};

class Profile{
    // Dados do usuário
    public:
        std::string profile_name; //Nome do usuario na aplicacao @exemplo
        std::vector <uint32_t> followers; // vetor de ponteiro para os followers
        std::vector <uint32_t> pending_notifications; // Notificações pendentes
        int sessions_number; // Numero de sessões já incializadas pelo usuário
    Profile(){};
    Profile(std::string name)
    {
        profile_name = name;
        sessions_number = 0;
    }
};

class ClientData{
    //wrapper do cliente
    public:
        std::vector<std::pair<uint32_t, uint32_t>> pending_notifications; // <profile, notification>
        std::vector<Notification> notifications;
        connection_data connection;
        pthread_t* consumer;
        pthread_t* producer;
        int profile_number;
        std::vector<Profile> *profile_ptr;
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

class packet{
    public:
        enum pkt_type type;      //Tipo do pacote (p.ex. DATA | CMD)
        int32_t seqn;            //Número de sequência
        int32_t length;          //Comprimento do payload
        char* timestamp;         // Timestamp do dado
        char payload[256];       //Dados da mensagem

        packet()
        {
            seqn = 0;
            length = 0;
            type = NONE;
            bzero((char*)payload,length);
        }

        packet(int32_t _seqn, char* _payload, int32_t _length, enum pkt_type _type)
        {
            type = _type;
            length = _length;
            seqn = _seqn;

            bzero((char*)payload,length);
            strcpy(payload, _payload);
        }

        packet(int32_t _seqn, int32_t _length, enum pkt_type _type)
        {
            type = _type;
            length = _length;
            seqn = _seqn;

            bzero((char*)payload,length);
        }
};

