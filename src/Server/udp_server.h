#pragma once
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h> // ?��?? Windows ???????��????
#pragma comment(lib, "Ws2_32.lib")
#include <string>
#include <unordered_map>
#include "online_manager.h"

//struct ClientInfo {
//    std::string ip_address;
//    int port;
//    bool is_online;
//};

class Server
{
private:
    SOCKET server_socket;
    std::string Ip;
    std::string subnetMask;
    std::unordered_map<std::string, ClientInfo> client_map;
    HANDLE hReceiverThread;
    bool stop_thread = false;
    OnlineManager* online_manager;  // ���ָ���Ա

    bool GetWirelessIP();
    void addClient(const std::string& ip, int port, bool is_online);
    static DWORD WINAPI ServerReceiverThread(LPVOID lpParam);

public:
    Server();
    ~Server();
    void close();
    int StartReceiverThread();
    int StopReceiverThread();
    int SendOfflineMessage();

};