#include "client_server.h"

using namespace std;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

Connection parseOptions(int argc, char* argv[])
{
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

int connectServer(Connection connection)
{
    //Connect to a server and return the socket
    int client_sk;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    server = gethostbyname(connection.ip);

    if(server == NULL)
    {
        errno = ENXIO;
        perror("Error");
        exit(EXIT_FAILURE);
    }

    if ((client_sk = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Error opening socket\n");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(connection.port);
    serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
    bzero(&(serv_addr.sin_zero), 8);

    if (connect(client_sk, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Error connecting\n");
        exit(EXIT_FAILURE);
    }

    return client_sk;
}

int sendProfile(Connection c)
{
    packet pkt(0, c.profile_name, 20, PROFILE);
    pkt.length = sizeof(pkt.payload);
    pkt.seqn = 1;

    return write(c.socket, &pkt, sizeof(pkt));
}

int receiveConfirmation(int sk_fd)
{
    time_t now = time(NULL);
    tm * ptm = localtime(&now);
    // Send the profile name trought to the server
    packet pkt;
    bzero(&pkt, sizeof (pkt));

    int n = read(sk_fd, &pkt, sizeof(pkt));
    if(n < 0)
    {
        perror("Fail to receive confirmation");
        exit(1);
    }

    if (pkt.type == ACCEPTED)
    {
        cout << "You're Successfully logged in :)" << endl;
        return 1;
    }
    return -1;
}

int post(char* message, Connection c)
{
    packet pkt(0, message, MAX_MESSAGE_SIZE, SEND);
    return write(c.socket, &pkt, sizeof(pkt));
}

int follow(char* profile_name, Connection c)
{
    packet pkt(0, profile_name, MAX_PROFILE_SIZE, FOLLOW);
    return write(c.socket, &pkt, sizeof(pkt));
}

void *sendServer(void *connection)
{
    Connection *connection_info = (Connection *) connection; // Connection info
    bool connected = true;
    string command;
    char data[128];


    while(connected)
    {
        cin >> command;
        cin >> data;

        if(command == "SEND")
        {
            post(data, *connection_info);
        }
        else if (command == "FOLLOW")
        {
            follow(data, *connection_info);
        }
        else
        {
            cout << "Invalid command:" << command << endl << "Usage: SEND <message>, FOLLOW <profile>" << endl;
        }
    }

    return connection;
}

void *listenServer(void *connection)
{
    Connection *connection_info = (Connection *) connection; // Connection info
    packet pkt;
    bool connected = true;

    while(connected)
    {
        bzero(&pkt, sizeof (pkt));
        int n = read(connection_info->socket, &pkt, sizeof(pkt));
        if(n < 0)
        {
            cout << "Error reading socket!" << endl;
        }
        if(n == 0)
            connected = false;

        if (pkt.type == DATA)
        {
            cout << "New message!" << endl;
        }
        cout << pkt.payload << endl;
    }

    return connection;
}


int main (int argc, char *argv[])
{
    Connection connection;

    pthread_t producer, consumer;

    connection = parseOptions(argc, argv);
    connection.socket = connectServer(connection);

    int send_result = sendProfile(connection);
    int confirmation = receiveConfirmation(connection.socket);

    if(confirmation)
    {
        cout << "Usage: SEND <message>, FOLLOW <profile>" << endl;

        pthread_create(&consumer, NULL, listenServer, &connection);
        pthread_create(&producer, NULL, sendServer, &connection);
        pthread_join(producer, NULL);
        pthread_join(consumer, NULL);
    }

    close(connection.socket);

    return 0;
}
