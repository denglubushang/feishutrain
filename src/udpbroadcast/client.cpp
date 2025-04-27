#pragma once
#include "client.h"

void Client::addClient(const std::string& ip, int port, bool is_online) {
    ClientInfo client_info;
    client_info.ip_address = ip;
    client_info.port = port;
    client_info.is_online = is_online;
    client_map[ip] = client_info;
}

int Client::SetClientIP() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR)
    {
        std::cerr << ("Error getting hostname: %d\n", WSAGetLastError()) << std::endl;
        return -1;
    }

    // Get the IP address of the host
    struct addrinfo hints = {};
    struct addrinfo* result = nullptr;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(hostname, nullptr, &hints, &result) != 0)
    {
        std::cerr << ("Error getting IP address: %d\n", WSAGetLastError()) << std::endl;
        return -1;
    }

    for (struct addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next)
    {
        struct sockaddr_in* sockaddr = (struct sockaddr_in*)ptr->ai_addr;
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sockaddr->sin_addr, ip, sizeof(ip));
        this->client_ip = ip;
    }
    freeaddrinfo(result);
    return 1;
}

DWORD WINAPI Client::BroadcastThread(LPVOID lpParam) {
    Client* client = static_cast<Client*>(lpParam);
    struct sockaddr_in broadcast_address;
    broadcast_address.sin_family = AF_INET;
    broadcast_address.sin_port = htons(8888);
    if (inet_pton(AF_INET, "255.255.255.255", &broadcast_address.sin_addr) <= 0)
    {
        std::cerr << "Error setting broadcast address" << std::endl;
        return -1;
    }
    std::string message = "discovery " + client->client_ip;
    const char* msg = message.c_str();

    while (true)
    {
        sendto(client->client_socket, msg, strlen(msg), 0, (struct sockaddr*)&broadcast_address, sizeof(broadcast_address));
        Sleep(1000 * 60);
    }

    return 0;
}

DWORD WINAPI Client::ClientResponseThread(LPVOID lpParam) {
    std::pair<Client*, std::string>* params = static_cast<std::pair<Client*, std::string>*>(lpParam);
    Client* client = params->first;
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
    const char* msg = message.c_str();
    if (sendto(client->client_socket, msg, strlen(msg), 0, (struct sockaddr*)&target_address, sizeof(target_address)) == SOCKET_ERROR)
    {
        std::cerr << "Error sending message: " << WSAGetLastError() << std::endl;
        delete params;
        return -1;
    }
    delete params;
    return 0;
}

DWORD WINAPI Client::ReceiverThread(LPVOID lpParam) {
    Client* client = static_cast<Client*>(lpParam);
    //设置接收地址
    struct sockaddr_in recv_address;
    recv_address.sin_family = AF_INET;
    recv_address.sin_port = htons(8888);
    recv_address.sin_addr.s_addr = INADDR_ANY;
    //创建接收socket套接字
    SOCKET recv_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (recv_socket == INVALID_SOCKET) {
        std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
        return -1;
    }
    //绑定接收地址
    if (bind(recv_socket, (struct sockaddr*)&recv_address, sizeof(recv_address)) == SOCKET_ERROR) {
        std::cerr << "Error binding socket: " << WSAGetLastError() << std::endl;
        closesocket(recv_socket);
        return -1;
    }

    char buffer[1024];
    struct sockaddr_in sender_address;
    int sender_address_len = sizeof(sender_address);

    while (true) {
        int recv_len = recvfrom(recv_socket, buffer, sizeof(buffer), 0, (struct sockaddr*)&sender_address, &sender_address_len);
        if (recv_len == SOCKET_ERROR) {
            std::cerr << "Error receiving message: " << WSAGetLastError() << std::endl;
            continue;
        }
        //解析收到的信息
        buffer[recv_len] = '\0';
        std::string message(buffer);

        char ip_str[INET_ADDRSTRLEN] = { 0 };
        if (inet_ntop(AF_INET, &sender_address.sin_addr, ip_str, sizeof(ip_str)) == nullptr) {
            std::cerr << "Error converting IP address: " << WSAGetLastError() << std::endl;
            continue;
        }
        std::string sender_ip(ip_str);
        if (sender_ip == client->client_ip) {
            continue;
        }
        //处理discovery信息
        if (message.find("discovery") != std::string::npos) {
            std::cerr << "Receiver message: " << message << "from " << sender_ip << std::endl;
            client->addClient(sender_ip, 8888, true);
        }
        else if (message.find("offline") != std::string::npos) {
            std::cerr << "Receiver offfline message from " << sender_ip << "message :" << message << std::endl;
            auto it = client->client_map.find(sender_ip);
            if (it != client->client_map.end()) {
                client->client_map.erase(it);
            }
        }
    }
    closesocket(recv_socket);
    return 0;
}

Client::Client() {
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
    if (setsockopt(this->client_socket, SOL_SOCKET, SO_BROADCAST, (char*)&broad_cast, sizeof(broad_cast)) == SOCKET_ERROR)
    {
        std::cerr << ("Error setting broadcast option: %d\n", WSAGetLastError()) << std::endl;
    }
}

int Client::ClientResponse(std::string target_ip)
{
    std::pair<Client*, std::string>* params = new std::pair<Client*, std::string>(this, target_ip);
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

int  Client::StartReceiverThread() {
    HANDLE hThread = CreateThread(nullptr, 0, ReceiverThread, this, 0, nullptr);
    if (hThread == nullptr)
{
        std::cerr << "Error creating thread: " << GetLastError() << std::endl;
        return -1;
    }
    CloseHandle(hThread);
    return 0;
}

int Client::SendOfflineMessage() {
    struct sockaddr_in broadcast_address;
    broadcast_address.sin_family = AF_INET;
    broadcast_address.sin_port = htons(8888);
    if (inet_pton(AF_INET, "255.255.255.255", &broadcast_address.sin_addr) <= 0) {
        std::cerr << "Error create offline message: " << WSAGetLastError() << std::endl;
        return -1;
    }
    std::string message = "offline " + client_ip;
    const char* msg = message.c_str();
    int result = sendto(client_socket, msg, strlen(msg), 0, (struct sockaddr*)&broadcast_address, sizeof(broadcast_address));
    if (result == SOCKET_ERROR) {
        std::cerr << "Error sending offline message: " << WSAGetLastError() << std::endl;
        return -1;
    }
    return 0;
}

Client::~Client() {
    WSACleanup(); 
    if (this->client_socket != INVALID_SOCKET)
    {
        closesocket(this->client_socket); 
    }
}
