#pragma once
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#include <windows.h> // 添加 Windows 线程库头文件
#pragma comment(lib, "Ws2_32.lib")
#include <string>

class Client
{
private:
    SOCKET client_socket_;
    std::string ip_;
    std::string subnet_mask_;
    void ClientGetWirelessIP();
    static DWORD WINAPI ClientBroadcastThread(LPVOID lpParam);

public:
    Client();
    ~Client();
    void close();
    int ClientBroadcast(int target_port);
};

struct SendParams {
    Client* client_;
    int target_port_;
    std::string message_;
    std::string broadcast_ip_;
};