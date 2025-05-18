#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <functional>
#include <filesystem>
#include <set>
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
	//断点续传接收文件
	int Hash_Receive_Resume(SOCKET client_sock, const std::string& filename);
	//接受文件传输过程
	int Hash_Receive(SOCKET& accept_client_Socket_);
	//事件监听循环
	void EventListen();
	//获取下载目录下有哪些文件
	std::set<std::string> GetFilesInDirectory();
	//将temp转为文件
	void Change_Downlad_FileName(std::string recvfile_name);
	// 生成新文件名，避免覆盖
	std::string generateUniqueFileName(const std::string& fileName);
	//清空客户端发来的数据
	void discard_socket_recv_buffer(SOCKET sock);
};

