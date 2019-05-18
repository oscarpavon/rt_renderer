#ifndef SERVER_H
#define SERVER_H

#include <vector>
#include "client.hpp"
#include <iostream>
#include <thread>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h> 
#include <arpa/inet.h>

class Server{
public:
	bool quit = false;
	std::vector<Client*> clients;
	void send_data();
	void get_ip_client();
	static void wait_connections(Server* server);
	static void recive_data(Client*);
};
#endif