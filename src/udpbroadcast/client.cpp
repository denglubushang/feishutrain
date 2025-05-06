#pragma once
#include "client.h"

DWORD WINAPI Client::ClientBroadcastThread(LPVOID lpParam)
{
    Client* client = static_cast<Client*>(lpParam);
    struct sockaddr_in target_address;
    target_address.sin_family = AF_INET;
    target_address.sin_port = htons(8080);
    target_address.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    const char *msg = "discovery";
    while(true)
    {
        if (sendto(client->client_socket_, msg, strlen(msg), 0, (struct sockaddr *)&target_address, sizeof(target_address)) == SOCKET_ERROR)
        {
            std::cerr << "Error sending message: " << WSAGetLastError() << std::endl;
            return -1;
        }
        Sleep(3000);
    }
    return 0;
}

Client::Client()
{
    // Create a socket
    this->client_socket_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (this->client_socket_ == INVALID_SOCKET)
    {
        std::cerr << ("Error creating socket: %d\n", WSAGetLastError()) << std::endl;
    }

    // Set socket broadcast option
    BOOL broad_cast = TRUE;
    if (setsockopt(this->client_socket_, SOL_SOCKET, SO_BROADCAST, (char *)&broad_cast, sizeof(broad_cast)) == SOCKET_ERROR)
    {
        std::cerr << ("Error setting broadcast option: %d\n", WSAGetLastError()) << std::endl;
    }
}

int Client::ClientBroadcast()
{
    HANDLE hThread = CreateThread(nullptr, 0, ClientBroadcastThread, this, 0, nullptr);
    if (hThread == nullptr)
    {
        std::cerr << "Error creating thread: " << GetLastError() << std::endl;
        return -1;
    }
    CloseHandle(hThread);
    return 0;
}

Client::~Client()
{
    WSACleanup(); 
    if (this->client_socket_ != INVALID_SOCKET)
    {
        closesocket(this->client_socket_); 
    }
}
