#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <functional>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "Segment.h"
class TcpServer
{
public:
	SOCKET server_Socket_;
	sockaddr_in server_Addr_;
	std::string password;
private:
	char buffer[1024];
public:
	//��ȡ��������󶨶�Ӧ�˿�
	TcpServer(std::string s);
	~TcpServer();
	//��ͻ�����֤�������
	void Connect(SOCKET& accept_client_Socket_);
	//�����ļ��������
	void Receive(SOCKET& accept_client_Socket_);
	//�����ļ��������
	void Hash_Receive(SOCKET& accept_client_Socket_);
	//�¼�����ѭ��
	void EventListen();
};

