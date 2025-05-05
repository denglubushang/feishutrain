#pragma once
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h> // 添加 Windows 线程库头文件
#pragma comment(lib, "Ws2_32.lib")
#include <string>

class Client
{
private:
    SOCKET client_socket;
    static DWORD WINAPI ClientSendThread(LPVOID lpParam);

public:
    Client();
    ~Client();
    int ClientSendMsg(std::string target_ip, int target_port, std::string message);
};

struct SendParams {
    Client* client;
    std::string target_ip;
    int target_port;
    std::string message;
};