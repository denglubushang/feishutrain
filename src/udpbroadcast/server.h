#pragma once
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h> // 添加 Windows 线程库头文件
#pragma comment(lib, "Ws2_32.lib")
#include <string>
#include <unordered_map>

struct ClientInfo {
    std::string ip_address;
    int port;
    bool is_online;
};

class Server
{
private:
    SOCKET server_socket;
    std::string Ip;
    std::string subnetMask;
    std::unordered_map<std::string, ClientInfo> client_map;
    HANDLE hReceiverThread;
    bool stop_thread = false;

    bool GetWirelessIP();
    void addClient(const std::string& ip, int port, bool is_online);
    static DWORD WINAPI ServerReceiverThread(LPVOID lpParam);

public:
    Server();
    ~Server();
    int StartReceiverThread();
    int StopReceiverThread();
    int SendOfflineMessage();
};