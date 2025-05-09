#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <iostream>
#include "server.h"

std::string BroadcastAddress(const std::string& ip, const std::string& subnetMask) {
    in_addr ipAddr, maskAddr;
    inet_pton(AF_INET, ip.c_str(), &ipAddr);
    inet_pton(AF_INET, subnetMask.c_str(), &maskAddr);

    in_addr broadcast;
    broadcast.S_un.S_addr = ipAddr.S_un.S_addr | ~maskAddr.S_un.S_addr;

    char broadcastStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &broadcast, broadcastStr, sizeof(broadcastStr));
    return std::string(broadcastStr);
}

bool Server::GetWirelessIP() {
    ULONG bufferSize = 15000;
    PIP_ADAPTER_ADDRESSES pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(bufferSize);
    if (!pAddresses) return false;  // 内存分配失败

    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, pAddresses, &bufferSize) != NO_ERROR) {
        free(pAddresses);
        return false;  // 获取地址信息失败
    }

    for (PIP_ADAPTER_ADDRESSES pAdapter = pAddresses; pAdapter; pAdapter = pAdapter->Next) {
        if (pAdapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        if (pAdapter->OperStatus != IfOperStatusUp) continue;
        if (pAdapter->IfType != IF_TYPE_IEEE80211) continue;  // 找到无线网络接口

        for (PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pAdapter->FirstUnicastAddress;
            pUnicast != nullptr;
            pUnicast = pUnicast->Next) {
            if (pUnicast->Address.lpSockaddr->sa_family == AF_INET) {
                sockaddr_in* sin = (sockaddr_in*)pUnicast->Address.lpSockaddr;
                char ipStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &sin->sin_addr, ipStr, sizeof(ipStr));
                this->Ip = ipStr;

                // 通过前缀长度计算掩码
                if (pUnicast->OnLinkPrefixLength > 0 && pUnicast->OnLinkPrefixLength <= 32) {
                    unsigned long mask = htonl(0xFFFFFFFF << (32 - pUnicast->OnLinkPrefixLength));
                    char maskStr[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &mask, maskStr, sizeof(maskStr));
                    this->subnetMask = maskStr;
                }
                break;
            }
        }

        // 检查是否成功获取了 IP 和掩码
        if (!this->Ip.empty() && !this->subnetMask.empty()) {
            free(pAddresses);
            return true;  // 成功获取到 IP 和掩码
        }
    }

    // 如果未能获取到 IP 或子网掩码
    std::cerr << "Failed to obtain IP or subnet mask. Aborting Server initialization." << std::endl;
    free(pAddresses);
    return false;  // 获取失败
}

void Server::addClient(const std::string& ip, int port, bool is_online) {
    auto it = client_map.find(ip);
    if (it != client_map.end()) {
        // 客户端已存在，仅更新状态
        it->second.is_online = is_online;
        it->second.port = port; // 如果端口可能变化也可以更新
    }
    else {
        // 新客户端
        ClientInfo client_info;
        client_info.ip_address = ip;
        client_info.port = port;
        client_info.is_online = is_online;
        client_map[ip] = client_info;
    }
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
            if (err_code == WSAEWOULDBLOCK) { 
                Sleep(10);
                continue;
            }
            else {
                std::cerr << "Error receiving message: " << err_code << std::endl;
                closesocket(recv_socket);
                break; 
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
        // 输出接收到的信息和发送者的 IP 地址
        std::cout << "Received message: '" << message << "' from Client: " << sender_ip << std::endl;
        if (sender_ip == server->Ip) {
            continue;
        }
        //处理discovery信息
        if (message.find("discovery") != std::string::npos) {
            std::cerr << "Receiver message: " << message << "from " << sender_ip << std::endl;
            server->addClient(sender_ip, 8888, true);
            std::cout << "Client " << sender_ip << " is online" << std::endl;  // 这里输出测试信息
        }
        else if (message.find("offline") != std::string::npos) {
            std::cerr << "Receiver offfline message from " << sender_ip << "message :" << message << std::endl;
            auto it = server->client_map.find(sender_ip);
            if (it != server->client_map.end()) {
                server->client_map.erase(it);
            }
        }
    }
    closesocket(recv_socket);
    return 0;
}

Server::Server() {
    // Create a socket
    this->server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (this->server_socket == INVALID_SOCKET) {
        std::cerr << ("Error creating socket: %d\n", WSAGetLastError()) << std::endl;
    }
    GetWirelessIP();
    BOOL broad_cast = TRUE;
    if (setsockopt(this->server_socket, SOL_SOCKET, SO_BROADCAST, (char*)&broad_cast, sizeof(broad_cast)) == SOCKET_ERROR) {
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
    if (Ip.empty() || subnetMask.empty()) {
        std::cerr << "Cannot send offline message: IP or subnet mask is empty." << std::endl;
        return -1;
    }

    std::string broadcast_ip = BroadcastAddress(Ip, subnetMask);

    struct sockaddr_in broadcast_address;
    broadcast_address.sin_family = AF_INET;
    broadcast_address.sin_port = htons(8888);
    if (inet_pton(AF_INET, broadcast_ip.c_str(), &broadcast_address.sin_addr) <= 0) {
        std::cerr << "Error setting broadcast address: " << WSAGetLastError() << std::endl;
        return -1;
    }

    std::string message = "offline " + Ip;
    const char* msg = message.c_str();
    int result = sendto(server_socket, msg, strlen(msg), 0,
        (struct sockaddr*)&broadcast_address, sizeof(broadcast_address));
    if (result == SOCKET_ERROR) {
        std::cerr << "Error sending offline message: " << WSAGetLastError() << std::endl;
        return -1;
    }

    std::cout << "Sent offline message to " << broadcast_ip << std::endl;
    return 0;
}

Server::~Server()
{
    WSACleanup();
    if (this->server_socket != INVALID_SOCKET) {
        closesocket(this->server_socket);
    }
}
