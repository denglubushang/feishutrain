#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
class TcpServer
{
public:
	SOCKET server_Socket_;
	sockaddr_in server_Addr_;
	std::string password;
private:
	char buffer[1024];
public:
	TcpServer(std::string s);
	~TcpServer();
	void Connect(SOCKET& accept_client_Socket_);
	void Receive(SOCKET& accept_client_Socket_);
	void EventListen();
};

