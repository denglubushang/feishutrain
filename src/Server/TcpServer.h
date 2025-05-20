#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <functional>
#include <filesystem>
#include <sstream>
#include <set>
#include <winsock2.h>
#include <windows.h>
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
	int HashReceiveResume(SOCKET client_sock, const std::string& filename);
	//�����ļ��������
	int HashReceive(SOCKET& accept_client_Socket_);
	//�¼�����ѭ��
	void EventListen();
	//��ȡ����Ŀ¼������Щ�ļ�
	std::set<std::string> GetFilesInDirectory();
	//��tempתΪ�ļ�
	void ChangeDownladFileName(std::string recvfile_name, const std::string& temp_file_path);
	//mapӳ���ļ���
	std::string GetUniqueNameFromMap(const std::string& filename);
	//ɾ��ӳ���ļ���
	void RemoveFileMapping(const std::string& filename, const std::string& unique_name, const std::string& map_path);
	//�鿴��ǰ�ļ����Ƿ��д����ļ���
	bool FileExists(const std::string& filename);
	// �������ļ��������⸲��
	std::string GenerateUniqueFileName(const std::string& fileName);
};

