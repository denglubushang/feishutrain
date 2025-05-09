#pragma once
#include <udpbroadcast/client.h>
#include <winsock2.h>
#pragma comment(lib, "Ws2_32.lib")

int main()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

	Client client;
    client.ClientBroadcast(8888);
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();
	return 0;
}