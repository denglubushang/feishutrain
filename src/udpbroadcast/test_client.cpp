#pragma once
#include "client.h"
#include <winsock2.h>
#pragma comment(lib, "Ws2_32.lib")

int main()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

	Client client;
	client.StartBroadcastThread(); // 启动广播线程
	client.StartReceiverThread();//启动接收线程
    client.ClientResponse("255.255.255.255"); // 启动客户端响应线程
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();
	client.SendOfflineMessage();
	return 0;
}