#pragma once
#include "client.h"

int Client::SetClientIP()
{
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR)
    {
        std::cerr << ("Error getting hostname: %d\n", WSAGetLastError()) << std::endl;
        return -1;
    }

    // Get the IP address of the host
    struct addrinfo hints = {};
    struct addrinfo *result = nullptr;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(hostname, nullptr, &hints, &result) != 0)
    {
        std::cerr << ("Error getting IP address: %d\n", WSAGetLastError()) << std::endl;
        return -1;
    }

    for (struct addrinfo *ptr = result; ptr != nullptr; ptr = ptr->ai_next)
    {
        struct sockaddr_in *sockaddr = (struct sockaddr_in *)ptr->ai_addr;
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sockaddr->sin_addr, ip, sizeof(ip));
        this->client_ip = ip;
    }
    freeaddrinfo(result);
    return 1;
}

DWORD WINAPI Client::BroadcastThread(LPVOID lpParam)
{
    Client *client = static_cast<Client *>(lpParam);
    struct sockaddr_in broadcast_address;
    broadcast_address.sin_family = AF_INET;
    broadcast_address.sin_port = htons(8888);
    if (inet_pton(AF_INET, "255.255.255.255", &broadcast_address.sin_addr) <= 0)
    {
        std::cerr << "Error setting broadcast address" << std::endl;
        return -1;
    }
    std::string message = "discovery " + client->client_ip;
    const char *msg = message.c_str();

    while (true)
    {
        sendto(client->client_socket, msg, strlen(msg), 0, (struct sockaddr *)&broadcast_address, sizeof(broadcast_address));
        Sleep(1000 * 60);
    }

    return 0;
}

DWORD WINAPI Client::ClientResponseThread(LPVOID lpParam)
{
    std::pair<Client *, std::string> *params = static_cast<std::pair<Client *, std::string> *>(lpParam);
    Client *client = params->first;
    std::string target_ip = params->second;

    struct sockaddr_in target_address;
    target_address.sin_family = AF_INET;
    target_address.sin_port = htons(8888);
    if (inet_pton(AF_INET, target_ip.c_str(), &target_address.sin_addr) <= 0)
    {
        std::cerr << "Error setting target address" << std::endl;
        delete params;
        return -1;
    }
    std::string message = "ack " + client->client_ip;
    const char *msg = message.c_str();
    if (sendto(client->client_socket, msg, strlen(msg), 0, (struct sockaddr *)&target_address, sizeof(target_address)) == SOCKET_ERROR)
    {
        std::cerr << "Error sending message: " << WSAGetLastError() << std::endl;
        delete params;
        return -1;
    }
    delete params;
    return 0;
}

Client::Client()
{
    // Get client IP address
    this->SetClientIP();

    // Create a socket
    this->client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (this->client_socket == INVALID_SOCKET)
    {
        std::cerr << ("Error creating socket: %d\n", WSAGetLastError()) << std::endl;
    }

    // Set socket broadcast option
    BOOL broad_cast = TRUE;
    if (setsockopt(this->client_socket, SOL_SOCKET, SO_BROADCAST, (char *)&broad_cast, sizeof(broad_cast)) == SOCKET_ERROR)
    {
        std::cerr << ("Error setting broadcast option: %d\n", WSAGetLastError()) << std::endl;
    }
}

int Client::ClientResponse(std::string target_ip)
{
    std::pair<Client *, std::string> *params = new std::pair<Client *, std::string>(this, target_ip);
    HANDLE hThread = CreateThread(nullptr, 0, ClientResponseThread, params, 0, nullptr);
    if (hThread == nullptr)
    {
        std::cerr << "Error creating thread: " << GetLastError() << std::endl;
        delete params;
        return -1;
    }
    CloseHandle(hThread);
    return 0;
}

int Client::StartBroadcastThread()
{
    HANDLE hThread = CreateThread(nullptr, 0, BroadcastThread, this, 0, nullptr);
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
    if (this->client_socket != INVALID_SOCKET)
    {
        closesocket(this->client_socket); 
    }
}
