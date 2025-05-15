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
	//��ȡ��������󶨶�Ӧ�˿�
	TcpServer(std::string s);
	~TcpServer();
	//��ͻ�����֤�������
	void Connect(SOCKET& accept_client_Socket_);
	//�����ļ��������
	void Receive(SOCKET& accept_client_Socket_);
	//�ϵ����������ļ�
	int Hash_Receive_Resume(SOCKET client_sock, const std::string& filename);
	//�����ļ��������
	int Hash_Receive(SOCKET& accept_client_Socket_);
	//�¼�����ѭ��
	void EventListen();
	//��ȡ����Ŀ¼������Щ�ļ�
	std::set<std::string> GetFilesInDirectory();
	//��tempתΪ�ļ�
	void Change_Downlad_FileName(std::string recvfile_name);
	//�鿴��ǰ�ļ����Ƿ��д����ļ���
	bool fileExists(const std::string& filename);
	// �������ļ��������⸲��
	std::string generateUniqueFileName(const std::string& fileName);
};

