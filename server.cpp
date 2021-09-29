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
 *      [X] - Escutar as mensagens do servidor
 * SERVIDOR:
 * [ ] - Implementar funcionalidades do servidor (follow e send)
 *      [X] - Verificar se existe request do usuário
 *      [X] - Decodificar qual o request:
 *          [X] - Enfileirar requests para que tenham uma ordem
 *      [X] - Implementar Follow:
 *          [X] - Atualizar o registro de followers do usuário citado no request
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
    for(uint32_t i = 0; i < profiles.size(); ++i)
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
    packet pkt;
    string profile_name;
    int n = read(socket, &pkt, sizeof(pkt));
    if (n < 0)
    {
        perror("Error reading from socket");
        exit(1);
    }

    if(pkt.type == PROFILE)
    {
        profile_name = pkt.payload;

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
                cout << "Error: too much connections with this account" << endl;
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
    packet pkt(0, 0, ACCEPTED);
    return write(socket, &pkt, sizeof(pkt));
}

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


vector<uint32_t> getFollowers(ClientData *client, int profile_number)
{
    return client->profile_ptr->at(profile_number).followers;
}

void queueMessage(ClientData *client, char* message_data)
{
    //Queue the message that will be sent by the sender
    int profile_number = client->profile_number;
    Profile* actual_profile = &client->profile_ptr->at(profile_number);

    vector<uint32_t> followers;
    followers = getFollowers(client, client->profile_number);
    Notification new_notification(message_data, actual_profile->profile_name.c_str(), followers.size());
    client->notifications.push_back(new_notification);
    for (uint32_t i = 0; i < followers.size(); i ++)
    {
        client->pending_notifications.push_back(pair<uint32_t, uint32_t>(profile_number, new_notification.getId()));
    }
}

void followPerson(ClientData *client, char* profile_data)
{
    int profile_number = client->profile_number; // current profile number
    int profile_follow = -1;
    //Follow a person right away if it exists
    if(profile_number == findProfileByName(profile_data, *client->profile_ptr))
    {
        cout << "You cannot follow yourself!" << endl;
    }
    else if ((profile_follow = findProfileByName(profile_data, *client->profile_ptr)) >= 0)
    {
        //There it goes
        cout << "Following: " << profile_data << endl;
        client->profile_ptr->at(profile_follow).followers.push_back(profile_number);
    }
    else
    {
        //It does not exist, stop trying please
        cout << "It does not exist sorry :(" << endl;
    }
}

int loadProfiles(vector<Profile> &profiles)
{
    ifstream file;
    file.open("profiles.txt");

     if (!file) {
        cout << "Unable to open file";
        return -1; // terminate with error
    }

    string line;
    while(getline(file, line))
    {
        stringstream s_stream(line);
        while(s_stream.good())
        {
            Profile actual_profile;
            int i = 0;
            string substring;
            getline(s_stream, substring, ',');
            string profile_name = substring;
            if(i == 0)
            {
                actual_profile = Profile(profile_name);
            }
            actual_profile.followers.push_back(atoi(substring.c_str()));
            i++;
        }
    }

    return 0;
}


int saveProfiles(vector<Profile> profiles)
{
    ofstream file;
    file.open("profiles.txt");

     if (!file) {
        cout << "Unable to open file";
        return -1; // terminate with error
    }
    for(int i = 0; i < profiles.size(); i ++)
    {
        file << profiles[i].profile_name << ",";
        for(int j = 0; j < profiles[i].followers.size(); j ++)
        {
            file << profiles[i].followers[j] << ",";
        }
        file << "\n";
    }

    file.close();
    return 0;
}


void processRequest(ClientData *client, packet pkt)
{
    bool connected = true;
    int n = 1;
        if(n < 0)
        {
            perror("Error:");
        }
        if(n == 0)
        {
            connected = false;
        }

        if(pkt.type == SEND)
        {
            queueMessage(client, pkt.payload);
        }
        else if(pkt.type == FOLLOW)
        {
            followPerson(client, pkt.payload);
        }

    close(client->connection.socket);
}

void* listenClient(void* client_ptr)
{
    ClientData *client = (ClientData *) client_ptr;
    bool connected = true;

    while(connected)
    {
        packet pkt;
        int n = read(client->connection.socket, &pkt, sizeof(pkt));
        if(n == 0)
            connected = false;

        pthread_mutex_lock(&mutex);
        processRequest(client, pkt);
        pthread_mutex_unlock(&mutex);
    }
    pthread_exit(client->consumer);
    close(client->connection.socket);

    return client_ptr;
}

void sendPending(ClientData *client)
{
    vector<pair<uint32_t, uint32_t>> pending  = client->pending_notifications;
    char* payload;
    for (int i = pending.size() - 1; i >= 0; i --)
    {
        uint32_t pos = pending[i].second;
        payload = client->notifications[pos].getBody();
        packet pkt(0, payload, 256, DATA);
        int n = write(client->connection.socket, &pkt, 256);
        if(n < 0)
        {
            cout << "Error sending pending messages" << endl;
        }
        pending.pop_back();
        i--;
    }
}

void* messageClient(void * client_ptr)
{
    ClientData *client = (ClientData *) client_ptr;

    pthread_mutex_lock(&mutex); //Critical section
    sendPending(client);
    pthread_mutex_unlock(&mutex); //Critical section

    pthread_exit(client->producer);
    close(client->connection.socket);
    return client_ptr;
}

void createThreads(ClientData c)
{
    ClientData *client_ptr = &c;
    pthread_t producer_ptr, consumer_ptr;

    c.producer = &producer_ptr;
    c.consumer = &consumer_ptr;

    cout << "Creating threads ... " << endl;
    pthread_create(&consumer_ptr, NULL, listenClient, (void *) client_ptr);
    pthread_create(&producer_ptr, NULL, messageClient,(void *) client_ptr);
};

void printUsers(const ClientData &client)
{
    for(int i = 0; i < client.profile_ptr->size(); i++)
    {
        cout << "Online: " << client.profile_ptr->at(i).profile_name << endl;
    }
}


uint32_t Notification::count = 0;

int main (int argc, char* argv [])
{
    vector<Profile> profiles;
    Connection c;

    int new_sock_fd;
    pair<int,socklen_t> socket;
    c = startServer(PORT);

    while(1)
    {
        loadProfiles(profiles);
        if ((new_sock_fd = accept(c.socket ,(struct sockaddr *) &c.addr, &c.socklen)) == -1)
        {
            perror("Accept ERROR");
            exit(EXIT_FAILURE);
        }
        else
        {
            ClientData actual_client;
            actual_client.connection.socket = new_sock_fd;
            actual_client.profile_number = requestProfile(new_sock_fd, profiles);
            actual_client.profile_ptr = &profiles;
            printUsers(actual_client);

            if(actual_client.profile_number >= 0)
            {
                sendConfirmation(new_sock_fd);
                createThreads(actual_client);
            }
        }
        saveProfiles(profiles);
    }
    close(new_sock_fd);

    return 0;
}
