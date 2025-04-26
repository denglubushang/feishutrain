#pragma once
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h> 
#pragma comment(lib, "Ws2_32.lib")
#include <string>

class Client
{
private:
    SOCKET client_socket;
    std::string client_ip;

    int SetClientIP();

    static DWORD WINAPI BroadcastThread(LPVOID lpParam);
    static DWORD WINAPI ClientResponseThread(LPVOID lpParam);

public:
    Client();
    int ClientResponse(std::string target_ip);
    int StartBroadcastThread();
    ~Client();
};