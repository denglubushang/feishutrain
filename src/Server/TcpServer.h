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
	//读取自身密码绑定对应端口
	TcpServer(std::string s);
	~TcpServer();
	//与客户端验证密码过程
	void Connect(SOCKET& accept_client_Socket_);
	//接受文件传输过程
	void Receive(SOCKET& accept_client_Socket_);
	//接受文件传输过程
	void Hash_Receive(SOCKET& accept_client_Socket_);
	//事件监听循环
	void EventListen();
};

