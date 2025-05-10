#pragma once
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <functional>
#include <filesystem>
#include <fstream>
#include <string>
#include "Segment.h"
#include "ProgressBar.h"
class TcpClient
{
public:
	SOCKET client_Socket_;
	sockaddr_in server_Addr_;
public:
	TcpClient();
	~TcpClient();
	void Controller();
	void Connect(const char* tag_ip);
	std::vector<std::string> GetFilesInDirectory();
	void SendFile(std::string tag_file_name);

};

