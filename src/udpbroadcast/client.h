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
    SOCKET client_socket_;
    static DWORD WINAPI ClientBroadcastThread(LPVOID lpParam);

public:
    Client();
    ~Client();
    int ClientBroadcast();
};