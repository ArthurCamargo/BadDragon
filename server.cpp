#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <unistd.h>
#include "client_server.h"
#include <exception>


int PORT = 8080;
using namespace std;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex

uint32_t Notification::count = 0; //static notification counter

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
    bzero(&(serv_addr.sin_zero), sizeof(serv_addr.sin_zero));

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

int findFollowerByNumber(int follower_number, int profile_number, vector<Profile>& profiles)
{
    vector<uint32_t> followers = profiles[follower_number].followers;
    for(uint32_t i = 0; i < followers.size(); i ++)
    {
        if(followers[i] == (uint) profile_number)
        {
            return i;
        }
    }
    return -1;
}

int createNewUser(string profile_name, vector<Profile>& profiles)
{
    bool exists = false;
    for(uint32_t i = 0; i < profiles.size(); i ++)
    {
        if(profile_name == profiles[i].profile_name)
        {
            exists = true;
        }
    }
    if(!exists)
    {
        //Create a new user in the profiles vector and return its position
        Profile new_profile;
        new_profile.profile_name = profile_name;
        new_profile.sessions_number = 0;


        //Push to the last position
        profiles.push_back(new_profile);
    }

    return profiles.size() - 1;
}

void login(ClientData &client, int socket)
{
    Packet pkt;
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

        int session = 0;
        int position = findProfileByName(profile_name, client.profiles);
        Device d;
        if(position != -1)
        {
            if(client.profiles[position].sessions_number <= 1)
            {
                cout << "Logged: " << client.profiles[position].profile_name << endl;
                session = ++ client.profiles[position].sessions_number;
                cout << "Logged: " << client.profiles[position].sessions_number << " times" << endl;
                sendConfirmation(socket, ACCEPTED);
                client.profile_number = position;
                d.socket = socket;
                d.id = session - 1;
                d.online = true;
                client.profiles[position].devices.emplace_back(d);
                createThreads(client);
            }
            else
            {
                cout << "Error: too much connections with this account" << endl;
                sendConfirmation(socket, FAILURE);
            }
        }
        else
        {
            position = createNewUser(profile_name, client.profiles);
            client.profile_number = position;
            session =  ++ client.profiles[position].sessions_number;
            client.profiles[position].devices.push_back(Device(session - 1, socket, true));

            cout << "Created new user:" << client.profiles[position].profile_name << endl;

            sendConfirmation(socket, ACCEPTED);
            createThreads(client);
        }
    }
}

int sendConfirmation(int socket, enum pkt_type type)
{
    Packet pkt(0, 0, type);
    return write(socket, &pkt, sizeof(pkt));
}

vector<uint32_t> getFollowers(ClientData *client, int profile_number)
{
    return client->profiles[profile_number].followers;
}

void queueMessage(ClientData *client, int profile_number, const char* message_data)
{
    vector<uint32_t> followers;
    followers = getFollowers(client, profile_number);

    Notification new_notification(strdup(message_data), client->profiles[profile_number].profile_name, followers.size());
    client->profiles[profile_number].notifications.push_back(new_notification);
    for (uint32_t i = 0; i < followers.size(); i ++)
    {
        //queue <sender, notification number>
        client->profiles[followers[i]].pending_notifications.push(
                pair<uint32_t, uint32_t>(profile_number, new_notification.getId()));
    }
}

int followPersonByName(vector<Profile>& profiles, int profile_number, string profile_name, bool verbose)
{
    int profile_follow = -1;
    //Follow a person right away if it exists
    if ((profile_follow = findProfileByName(profile_name, profiles)) >= 0)
    {
        if((findFollowerByNumber(profile_follow, profile_number, profiles)) >= 0)
        {
            //Your already follow this account
            if(verbose)
                cout << "Already follow this account" << endl;
            return 0;
        }
        else if(profiles[profile_number].profile_name == profile_name)
        {
            //You cannot follow yourself
            if(verbose)
                cout << "Can't follow yourself" << endl;
            return -1;
        }
        else
        {
            profiles[profile_follow].followers.push_back(profile_number);
            //There you go
            if(verbose)
                cout << "Following: " << profiles[profile_follow].profile_name << endl;
            return 1;
        }
    }
    else
    {
        //This user does not exist
        if(verbose)
            cout << "Does not exists" << endl;
        return -1;
    }
}

int loadProfiles(vector<Profile> &profiles)
{
    int actual_profile = 0;
    string substring;
    string profile_name;
    ifstream file;
    file.open("profiles.txt");

     if (!file) {
        cout << "Unable to open file";
        return -1; // terminate with error
    }

    string line;
    while(getline(file, line))
    {
        int i = 0;
        stringstream names(line);
        while(names.good())
        {
            //Insert the names into the profiles
            actual_profile = -1;
            getline(names, substring, ',');
            profile_name = substring;
            if(i == 0 && !profile_name.empty())
            {
                actual_profile = createNewUser(profile_name, profiles);
            }
            i++;
        }
    }

    file.clear();  //starts from the begging of the file again
    file.seekg(0, file.beg);

    while(getline(file, line))
    {
        stringstream followers(line);
        int i = 0;
        actual_profile = -1;
        while(followers.good())
        {
            //Follow people
            getline(followers, substring, ',');
            profile_name = substring;
            if(i == 0 && !profile_name.empty())
            {
                actual_profile = findProfileByName(profile_name, profiles);
            }
            else if(!profile_name.empty())
            {
                if(actual_profile >= 0)
                {
                    followPersonByName(profiles, actual_profile, profile_name, false);
                }
            }
            i++;
        }
    }

    return 0;
}

int saveProfiles(vector<Profile> &profiles)
{
    uint32_t follower_number;

    ofstream file;
    file.open("profiles.txt", ios::trunc);

    if (!file) {
        cout << "Unable to open file";
        return -1; // terminate with error
    }
    for(uint32_t i = 0; i < profiles.size(); i ++)
    {
        file << profiles[i].profile_name << ",";
        for(uint32_t j = 0; j < profiles[i].followers.size(); j ++)
        {
            follower_number = profiles[i].followers[j];
            file << profiles[follower_number].profile_name << ",";
        }
        file << "\n";
    }

    file.close();
    return 0;
}

void processRequest(ClientData *client, int profile_number, Packet pkt)
{
    if(pkt.type == SEND)
    {
        queueMessage(client, profile_number, pkt.payload);
    }
    else if(pkt.type == FOLLOW)
    {
        if (followPersonByName(client->profiles, profile_number, pkt.payload, true))
            saveProfiles(client->profiles);
    }
}

void reduceDeviceNumber(ClientData *client, int profile_number)
{
    for(uint i = 0; i < client->profiles[profile_number].devices.size(); i ++)
    {
        client->profiles[profile_number].devices[i].id --;
    }
}

void* listenClient(void* client_ptr)
{
    ClientData *client = (ClientData *) client_ptr;
    int profile_number = client->profile_number;
    int session = client->profiles[profile_number].sessions_number;
    int device_id = client->profiles[profile_number].devices[session - 1].id;
    int socket = client->profiles[profile_number].devices[device_id].socket;
    bool online = client->profiles[profile_number].devices[device_id].online;

    int n = 0;

    while(online)
    {
        Packet pkt;

        n = read(socket, &pkt, sizeof(pkt));
        if(n == 0)
            client->profiles[profile_number].devices[device_id].online = false;

        pthread_mutex_lock(&mutex);
        if(!client->profiles[profile_number].devices.empty())
            online = client->profiles[profile_number].devices[device_id].online;
        else
            online = false;
        processRequest(client, profile_number, pkt);
        pthread_mutex_unlock(&mutex);
    }

    cout << "Logging out: " << client->profiles[profile_number].profile_name << endl;
    pthread_exit(NULL);
    return client_ptr;
}

void* messageClient(void * client_ptr)
{
    ClientData *client = (ClientData *) client_ptr;
    int profile_number = client->profile_number;
    int session = client->profiles[profile_number].sessions_number;
    int device_id = client->profiles[profile_number].devices[session - 1].id;
    int socket = client->profiles[profile_number].devices[device_id].socket;
    bool online = client->profiles[profile_number].devices[device_id].online;

    while(online)
    {
        cout << device_id << endl;
        pthread_mutex_lock(&mutex);
        online = client->profiles[profile_number].devices[device_id].online;
        sendPending(client, profile_number, socket);
        pthread_mutex_unlock(&mutex);
    }

    if(!client->profiles[profile_number].devices.empty())
    {
        pthread_mutex_lock(&mutex);
        //client->profiles[profile_number].devices.erase(client->profiles[profile_number].devices.begin() + device_id);
        client->profiles[profile_number].sessions_number --;
        reduceDeviceNumber(client, profile_number);
        pthread_mutex_unlock(&mutex);
    }

    close(socket);
    pthread_exit(NULL);
    return client_ptr;
}

void sendPending(ClientData *client, int profile_number, int socket)
{
    char* payload;

    uint32_t sender = -1;
    uint32_t noti_number = -1;
    uint32_t id = -1;
    string message = "";
    int session = client->profiles[profile_number].sessions_number;
    vector<Device> devices = client->profiles[profile_number].devices;
    int n = -1;

    if(!(client->profiles[profile_number].pending_notifications.empty()))
    {
        sender = client->profiles[profile_number].pending_notifications.front().first;
        noti_number = client->profiles[profile_number].pending_notifications.front().second;

        id = findNotificationById(client, sender, noti_number);

        // Payload = "sender: message"
        message = client->profiles[sender].profile_name + ": ";
        message += client->profiles[sender].notifications[id].getBody();
        payload = strdup(message.c_str());
        Packet pkt(0, payload, 256, DATA);
        //Duplicate for all the devices
        for(int i = 0; i < session; i ++)
        {
            if(client->profiles[profile_number].devices[i].online)
                n = write(devices[i].socket, &pkt, sizeof(pkt));
        }
        if(n < 0)
        {
            cout << "Error sending pending messages" << endl;
        }
        else if(n == 0)
        {
            client->profiles[profile_number].devices[session - 1].online = false;
        }
        else
        {
            client->profiles[profile_number].pending_notifications.pop();
        }
    }
}

int findNotificationById(ClientData *client, int profile_number, int id)
{
    uint32_t size = client->profiles[profile_number].notifications.size();
    for(uint32_t i = 0; i < size; i ++)
    {
        if(client->profiles[profile_number].notifications[i].getId() == (uint) id)
            return i;
    }
    return -1;
}

void createThreads(ClientData &c)
{
    ClientData *client_ptr;
    client_ptr  = (ClientData*) malloc(sizeof(ClientData));

    client_ptr = &c;

    pthread_t* producer_ptr;
    pthread_t* consumer_ptr;

    producer_ptr = (pthread_t*) malloc(sizeof(pthread_t));
    consumer_ptr = (pthread_t*) malloc(sizeof(pthread_t));

    client_ptr->consumer = consumer_ptr;
    client_ptr->producer = producer_ptr;

    cout << "Creating threads ... " << endl;
    pthread_create(producer_ptr, NULL, listenClient, (void *) client_ptr);
    pthread_create(consumer_ptr, NULL, messageClient,(void *) client_ptr);
};

//1o servidor
//2o master ou backup
//3o PORT
//3o nro backups

int argChooseServer(int argc, char* argv[]){
    int ret;
    
    if (argc < 3 && argc > 4){
        cout << "Wrong number of parameters." << endl;
        exit(1);
    }

    PORT = atoi(argv[2]);

    if (!strcmp(argv[1],"master")){
        if(argc == 4){
            PORT = atoi(argv[3]);
            ret  = atoi(argv[2]);
        }
        else{
            ret = 0;
        }
    }

    else if (!strcmp(argv[1], "backup"))
        ret = -1;

    return ret;
}


void connectClient(ClientData actual_client){
    int new_sock_fd;
    Connection c;

    c = startServer(PORT);

    while(1){
        loadProfiles(actual_client.profiles);
        if ((new_sock_fd = accept(c.socket, (struct sockaddr *) &c.addr, &c.socklen)) == -1)
            {
                perror("Accept ERROR");
                exit(EXIT_FAILURE);
            }
        else
            {
                actual_client.connection.socket = new_sock_fd;
                login(actual_client, new_sock_fd);
            }
        saveProfiles(actual_client.profiles);
    }
}

Connection parseOptions(int argc, char* argv[]){
    //Parse the options and put the data inside the connection
    int opt;
    Connection c;
    while((opt = getopt(argc, argv, "u:i:p:")) != -1)
    {
        switch(opt)
        {
            case 'u':
                if (strlen(optarg) > 20 || strlen(optarg) < 4)
                {
                    cout << "Error: profile out of bounds (4 - 20 characters)" << endl;
                    exit(1);
                    break;
                }
                c.profile_name = optarg;
                break;
            case 'i':
                c.ip = optarg;
                break;
            case 'p':
                c.port = atoi(optarg);
                break;
            case '?':
                cout << "Usage: client -u <profile> -i <ip> -p <port>" << endl;
                exit(1);
                break;
            default:
                cout << "Usage: client -u <profile> -i <ip> -p <port>" << endl;
                exit(1);
                break;
        }
    }

    return c;
}

int main (int argc, char* argv [])
{
    Connection s;
    int num_backup;
    pair<int,socklen_t> socket;
    ClientData actual_client;
    Server new_server;


    num_backup = argChooseServer(argc, argv);

    if (num_backup >= 0){

        new_server.master = true;
        new_server.backup_count = num_backup;
        
    }

    
    s = startServer(PORT+1);

    connectServer();

    if (new_server.master){

        connectClient(actual_client);

    }

    return 0;
}