default: client server

client: client.cpp client_server.h
	g++ -Wall -fsanitize=address client.cpp -o client
server: server.cpp client_server.h
	g++ -Wall -fsanitize=address server.cpp -o server
