#pragma once
#include "client.h"

// 定义一个结构体来打包参数
struct SendParams {
    Client* client;
    std::string target_ip;
    int target_port;
    std::string message;
};

DWORD WINAPI Client::ClientSendThread(LPVOID lpParam)
{
    SendParams* params = static_cast<SendParams*>(lpParam);
    struct sockaddr_in target_address;
    target_address.sin_family = AF_INET;
    target_address.sin_port = htons(params->target_port);
    if (inet_pton(AF_INET, params->target_ip.c_str(), &target_address.sin_addr) <= 0)
    {
        std::cerr << "Error setting target address" << std::endl;
        delete params;
        return -1;
    }
    const char *msg = params->message.c_str();
    if (sendto(params->client->client_socket, msg, strlen(msg), 0, (struct sockaddr *)&target_address, sizeof(target_address)) == SOCKET_ERROR)
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

int Client::ClientSendMsg(std::string target_ip, int target_port, std::string message)
{
    /*
        target_ip: 目标IP地址，例如广播地址 "255.255.255.255"
        target_port: 目标端口号
        message: 要发送的消息
    */
    SendParams* params = new SendParams{this, target_ip, target_port, message};
    HANDLE hThread = CreateThread(nullptr, 0, ClientSendThread, params, 0, nullptr);
    if (hThread == nullptr)
    {
        std::cerr << "Error creating thread: " << GetLastError() << std::endl;
        delete params;
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
