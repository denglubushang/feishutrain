#pragma once
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h> 
#pragma comment(lib, "Ws2_32.lib")
#include <string>
#include <unordered_map>

struct ClientInfo {
    std::string ip_address;
    int port;
    bool is_online;
};

class Client
{
private:
    SOCKET client_socket;
    std::string client_ip;
    std::unordered_map<std::string, ClientInfo> client_map;

    int SetClientIP();
    void addClient(const std::string& ip, int port, bool is_online);

    static DWORD WINAPI BroadcastThread(LPVOID lpParam);
    static DWORD WINAPI ClientResponseThread(LPVOID lpParam);
    static DWORD WINAPI ReceiverThread(LPVOID lpParam);

public:
    Client();
    int ClientResponse(std::string target_ip);
    int StartBroadcastThread();
    int StartReceiverThread();
    int SendOfflineMessage();
    ~Client();
};