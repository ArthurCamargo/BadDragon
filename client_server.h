#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <vector>
#include <pthread.h>
#include <time.h>


typedef struct Connection
{
    char* ip;
    char* profile_name;
    int port;
    int socket;
    bool is_connected;
    socklen_t socklen;
    sockaddr_in addr;
} connection_data;

class Notifications
{
    uint16_t id; //Identificador unico do post
    std::string author;     // Autor do post
    std::string timestamp; //tempo
    uint16_t length; //Quantidade de chars
    uint16_t not_received; // Numero de pessoas que nao receberam a mensagem
    std::string body; // Corpo da mensagem
};

class Profile{
    // Dados do usuário
    public:
        std::string profile_name; //Nome do usuario na aplicacao @exemplo
        std::vector <uint16_t> followers; // vetor de ponteiro para os followers
        std::vector <Notifications> pending_notifications; // Notificações pendentes
        int sessions_number; // Numero de sessões já incializadas pelo usuário
};

class Client{
    //wrapper do cliente
    public:
        connection_data connection;
        pthread_t* consumer;
        pthread_t* producer;
        int profile_number;
        int socket;
        std::vector<Profile> *profile_ptr;
};

enum pkt_type
{
    PROFILE = 1,
    DATA,
    ACCEPTED,
    FAILURE,
    SEND,
    FOLLOW

};

typedef struct _packet{
    enum pkt_type type;       //Tipo do pacote (p.ex. DATA | CMD)
    uint16_t seqn;            //Número de sequência
    uint16_t length;          //Comprimento do payload
    char* timestamp;          // Timestamp do dado
    char payload[256];            //Dados da mensagem
} packet;

