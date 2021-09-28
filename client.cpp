#include "client_server.h"

using namespace std;


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
    time_t now = time(NULL);
    tm * ptm = localtime(&now);
    // Send the profile name trought to the server
    packet pkt;
    bzero(&pkt, sizeof (pkt));
    pkt.type = PROFILE;
    strcpy(pkt.payload, c.profile_name);
    strftime(pkt.timestamp, sizeof(pkt.payload), "%H:%M:%S", ptm);
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
        perror("Error reading from socket");
        exit(1);
    }

    if (pkt.type == ACCEPTED)
    {
        cout << "You're Successfully logged in :)" << endl;
        return 1;
    }
    return -1;
}

int post(char* menssage, Connection c)
{
    time_t now = time(NULL);
    tm * ptm = localtime(&now);
    // Send the profile name trought to the server
    packet pkt;
    bzero(&pkt, sizeof (pkt));
    pkt.type = SEND;
    strcpy(pkt.payload, menssage);
    strftime(pkt.timestamp, sizeof(pkt.payload), "%H:%M:%S", ptm);
    pkt.length = sizeof(pkt.payload);
    pkt.seqn = 1;

    return write(c.socket, &pkt, sizeof(pkt));
}

int follow(char* profile_name, Connection c)
{
    time_t now = time(NULL);
    tm * ptm = localtime(&now);
    // Send the profile name trought to the server
    packet pkt;
    bzero(&pkt, sizeof (pkt));
    pkt.type = SEND;
    strcpy(pkt.payload, profile_name);
    strftime(pkt.timestamp, sizeof(pkt.payload), "%H:%M:%S", ptm);
    pkt.length = sizeof(pkt.payload);
    pkt.seqn = 1;

    return write(c.socket, &pkt, sizeof(pkt));
}

void sendCommand(Connection c)
{
    string command;
    char data[256];

    cin >> command;
    fgets(data, 256, stdin);

    if(command == "SEND")
    {
        post(data, c);
    }
    else if (command == "FOLLOW")
    {
        follow(data, c);
    }
    else
    {
        cout << "Invalid command:" << command << endl << "Usage: SEND <message>, FOLLOW <profile>" << endl;
    }
}

int listenServer(char* buffer)
{
    int response = 1;

    return 1;
}

int main (int argc, char *argv[])
{
    Connection connection;
    char data_buffer[2048];

    connection = parseOptions(argc, argv);
    connection.socket = connectServer(connection);

    int send_result = sendProfile(connection);
    int confirmation = receiveConfirmation(connection.socket);

    if(confirmation)
    {
        connection.is_connected = true;
        while(connection.is_connected)
        {
            cout << "Usage: SEND <message>, FOLLOW <profile>" << endl;
            int response = listenServer(data_buffer);
            sendCommand(connection);
        }
    }

    close(connection.socket);

    return 0;
}
