#pragma once
#include "client.h"
#include <winsock2.h>
#pragma comment(lib, "Ws2_32.lib")

int main()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

	Client client;
    client.ClientSendMsg("255.255.255.255", 8888, "hello world"); // 广播上线消息
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();
	return 0;
}