#include<iostream>
#include<fstream>
#include"TcpServer.h"
#include "udp_client.h"
#include "udp_server.h"
#pragma comment(lib, "Ws2_32.lib")
int main() {
	std::ifstream passfile("password.txt");
	if (!passfile) {
		std::cout << "文件打开时失败";
		exit(1);
	}
	std::string password;
	passfile >> password;
	TcpServer myServer(password);
	Server server;
	server.StartReceiverThread();
	Client client;
	client.ClientBroadcast(8888);
	//std::cout << "press enter to exit" << std::endl;
	//std::cin.get();
	server.close();
	//client.close();
	server.StopReceiverThread();
	myServer.EventListen();
	
	server.SendOfflineMessage();
}