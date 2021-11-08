default: client server

debug: c_debug s_debug

c_debug : client.cpp client_server.h
	g++ -g -pthread -Wall  client.cpp -o client

s_debug : client.cpp client_server.h
	g++ -g -pthread -Wall  server.cpp -o server
	
client: client.cpp client_server.h
	g++ -pthread  -Wall -O3 client.cpp -o client

server: server.cpp client_server.h
	g++ -pthread -Wall -O3 server.cpp -o server
