#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <iostream>
#include "server.h"


std::string ServerGetWirelessIP() {
    ULONG bufferSize = 15000;
    IP_ADAPTER_ADDRESSES* adapterAddresses = (IP_ADAPTER_ADDRESSES*)malloc(bufferSize);
    if (GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, nullptr, adapterAddresses, &bufferSize) != NO_ERROR) {
        free(adapterAddresses);
        return "";
    }

    std::string localIp;
    for (IP_ADAPTER_ADDRESSES* adapter = adapterAddresses; adapter; adapter = adapter->Next) {
        // 排除回环、未启用接口、虚拟网卡等
        if (adapter->OperStatus != IfOperStatusUp || adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
            continue;

        for (IP_ADAPTER_UNICAST_ADDRESS* addr = adapter->FirstUnicastAddress; addr; addr = addr->Next) {
            sockaddr_in* sa_in = (sockaddr_in*)addr->Address.lpSockaddr;
            char ipStr[INET_ADDRSTRLEN] = {};
            inet_ntop(AF_INET, &(sa_in->sin_addr), ipStr, sizeof(ipStr));

            std::string ip(ipStr);
            if (ip.find("192.") == 0 || ip.find("10.") == 0 || ip.find("172.") == 0) {
                localIp = ip;
                break;
            }
        }

        if (!localIp.empty()) {
            break;
        }
    }

    free(adapterAddresses);
    return localIp;
}

void Server::addClient(const std::string& ip, int port, bool is_online) {
    ClientInfo client_info;
    client_info.ip_address = ip;
    client_info.port = port;
    client_info.is_online = is_online;
    client_map[ip] = client_info;
}

DWORD WINAPI Server::ServerReceiverThread(LPVOID lpParam) {
    Server* server = static_cast<Server*>(lpParam);
    //设置接收地址
    struct sockaddr_in recv_address;
    recv_address.sin_family = AF_INET;
    recv_address.sin_port = htons(8888);
    if (inet_pton(AF_INET, server->Ip.c_str(), &recv_address.sin_addr) <= 0) {
        std::cerr << "Invalid IP address: " << server->Ip << std::endl;
        return -1;
    }

    //创建接收socket套接字
    SOCKET recv_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (recv_socket == INVALID_SOCKET) {
        std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
        return -1;
    }
    u_long mode = 1; // 1 for non-blocking mode, 0 for blocking
    if (ioctlsocket(recv_socket, FIONBIO, &mode) != NO_ERROR) {
        std::cerr << "Error setting socket to non-blocking mode" << std::endl;
        closesocket(recv_socket);
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

    while (!server->stop_thread) {
        int recv_len = recvfrom(recv_socket, buffer, sizeof(buffer), 0, (struct sockaddr*)&sender_address, &sender_address_len);
        if (recv_len == SOCKET_ERROR) {
            int err_code = WSAGetLastError();
            if (err_code == WSAEWOULDBLOCK) {  // No data available, just continue
                continue;
            }
            else {
                std::cerr << "Error receiving message: " << err_code << std::endl;
                break;  // Exit the loop on other errors
            }
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
        if (sender_ip == server->Ip) {
            continue;
        }
        //处理discovery信息
        if (message.find("discovery") != std::string::npos) {
            std::cerr << "Receiver message: " << message << "from " << sender_ip << std::endl;
            server->online_manager->updateClient(sender_ip, 8888);
        }
        else if (message.find("offline") != std::string::npos) {
            std::cerr << "Receiver offfline message from " << sender_ip << "message :" << message << std::endl;
            auto it = server->client_map.find(sender_ip);
            if (it != server->client_map.end()) {
                server->online_manager->removeClient(sender_ip);
            }

        }

        // 每次接收处理完一条消息后尝试进行清理
        static auto last_cleanup = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_cleanup).count() > 10) {
            server->online_manager->cleanupOfflineClients();
            last_cleanup = now;
        }
    }
    closesocket(recv_socket);
    return 0;
}

void Server::SetIP() {
    Server::Ip = ServerGetWirelessIP();
    // 打印出获取到的 IP 地址
    std::cout << "Obtained Wireless IP: " << Server::Ip << std::endl;
    if (!Ip.empty()) {
        std::cout << "Wireless IP: " << Ip << std::endl;
    }
    else {
        std::cout << "Wireless adapter not found!" << std::endl;
    }
}

Server::Server(){
    // Create a socket
    this->server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (this->server_socket == INVALID_SOCKET) {
        std::cerr << ("Error creating socket: %d\n", WSAGetLastError()) << std::endl;
    }
    SetIP();
    BOOL broad_cast = TRUE;
    online_manager = new OnlineManager(client_map);
    if (setsockopt(this->server_socket, SOL_SOCKET, SO_BROADCAST, (char*)&broad_cast, sizeof(broad_cast)) == SOCKET_ERROR)
    {
        std::cerr << ("Error setting broadcast option: %d\n", WSAGetLastError()) << std::endl;
    }
}

int Server::StartReceiverThread() {

    hReceiverThread = CreateThread(nullptr, 0, ServerReceiverThread, this, 0, nullptr);
    if (hReceiverThread == nullptr) {
        std::cerr << "Error creating thread: " << GetLastError() << std::endl;
        return -1;
    }
    std::cerr << "Thread has created " << std::endl;
    return 0;
}

int Server::StopReceiverThread() {

    if (hReceiverThread != nullptr) {
        stop_thread = true;
        // 等待线程完成
        WaitForSingleObject(hReceiverThread, INFINITE);
        // 关闭线程句柄
        CloseHandle(hReceiverThread);
        hReceiverThread = nullptr;
    }
    return 0;
}

int Server::SendOfflineMessage() {
    struct sockaddr_in broadcast_address;
    broadcast_address.sin_family = AF_INET;
    broadcast_address.sin_port = htons(8888);
    if (inet_pton(AF_INET, "255.255.255.255", &broadcast_address.sin_addr) <= 0) {
        std::cerr << "Error create offline message: " << WSAGetLastError() << std::endl;
        return -1;
    }
    std::string message = "offline " + Ip;
    const char* msg = message.c_str();
    int result = sendto(server_socket, msg, strlen(msg), 0, (struct sockaddr*)&broadcast_address, sizeof(broadcast_address));
    if (result == SOCKET_ERROR) {
        std::cerr << "Error sending offline message: " << WSAGetLastError() << std::endl;
        return -1;
    }
    return 0;
}

Server::~Server()
{
    WSACleanup();
    if (this->server_socket != INVALID_SOCKET) {
        closesocket(this->server_socket);
    }
    delete online_manager;
}