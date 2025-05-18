#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <iostream>
#include "udp_server.h"

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
    if (!pAddresses) return false;  // �ڴ����ʧ��

    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, pAddresses, &bufferSize) != NO_ERROR) {
        free(pAddresses);
        return false;  // ��ȡ��ַ��Ϣʧ��
    }

    for (PIP_ADAPTER_ADDRESSES pAdapter = pAddresses; pAdapter; pAdapter = pAdapter->Next) {
        if (pAdapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        if (pAdapter->OperStatus != IfOperStatusUp) continue;
        if (pAdapter->IfType != IF_TYPE_IEEE80211) continue;  // �ҵ���������ӿ�

        for (PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pAdapter->FirstUnicastAddress;
            pUnicast != nullptr;
            pUnicast = pUnicast->Next) {
            if (pUnicast->Address.lpSockaddr->sa_family == AF_INET) {
                sockaddr_in* sin = (sockaddr_in*)pUnicast->Address.lpSockaddr;
                char ipStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &sin->sin_addr, ipStr, sizeof(ipStr));
                this->Ip = ipStr;

                // ͨ��ǰ׺���ȼ�������
                if (pUnicast->OnLinkPrefixLength > 0 && pUnicast->OnLinkPrefixLength <= 32) {
                    unsigned long mask = htonl(0xFFFFFFFF << (32 - pUnicast->OnLinkPrefixLength));
                    char maskStr[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &mask, maskStr, sizeof(maskStr));
                    this->subnetMask = maskStr;
                }
                break;
            }
        }

        // ����Ƿ�ɹ���ȡ�� IP ������
        if (!this->Ip.empty() && !this->subnetMask.empty()) {
            free(pAddresses);
            return true;  // �ɹ���ȡ�� IP ������
        }
    }

    // ���δ�ܻ�ȡ�� IP ����������
    std::cerr << "Failed to obtain IP or subnet mask. Aborting Server initialization." << std::endl;
    free(pAddresses);
    return false;  // ��ȡʧ��
}
void Server::close() {
    if (this->server_socket != INVALID_SOCKET) {
        closesocket(this->server_socket);
    }
}
void Server::addClient(const std::string& ip, int port, bool is_online) {
    auto it = client_map.find(ip);
    if (it != client_map.end()) {
        // �ͻ����Ѵ��ڣ�������״̬
        it->second.is_online = is_online;
        it->second.port = port; // ����˿ڿ��ܱ仯Ҳ���Ը���
    }
    else {
        // �¿ͻ���
        ClientInfo client_info;
        client_info.ip_address = ip;
        client_info.port = port;
        client_info.is_online = is_online;
        client_map[ip] = client_info;
    }
}

DWORD WINAPI Server::ServerReceiverThread(LPVOID lpParam) {
    Server* server = static_cast<Server*>(lpParam);
    //���ý��յ�ַ
    struct sockaddr_in recv_address;
    recv_address.sin_family = AF_INET;
    recv_address.sin_port = htons(8888);
    if (inet_pton(AF_INET, server->Ip.c_str(), &recv_address.sin_addr) <= 0) {
        std::cerr << "Invalid IP address: " << server->Ip << std::endl;
        return -1;
    }

    //��������socket�׽���
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
    //�󶨽��յ�ַ
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
        //�����յ�����Ϣ
        buffer[recv_len] = '\0';
        std::string message(buffer);

        char ip_str[INET_ADDRSTRLEN] = { 0 };
        if (inet_ntop(AF_INET, &sender_address.sin_addr, ip_str, sizeof(ip_str)) == nullptr) {
            std::cerr << "Error converting IP address: " << WSAGetLastError() << std::endl;
            continue;
        }
        std::string sender_ip(ip_str);
        // ������յ�����Ϣ�ͷ����ߵ� IP ��ַ
        //std::cout << "Received message: '" << message << "' from Client: " << sender_ip << std::endl;
        if (sender_ip == server->Ip) {
            continue;
        }
        //����discovery��Ϣ
        if (message.find("discovery") != std::string::npos) {
            //std::cerr << "Receiver message: " << message << "from " << sender_ip << std::endl;
            server->addClient(sender_ip, 8888, true);
            //std::cout << "Client " << sender_ip << " is online" << std::endl;  // �������������Ϣ
        }
        else if (message.find("offline") != std::string::npos) {
            std::cerr << "Receiver offfline message from " << sender_ip << "message :" << message << std::endl;
            auto it = server->client_map.find(sender_ip);
            if (it != server->client_map.end()) {
                server->online_manager->removeClient(sender_ip);
            }

        }

        // ÿ�ν��մ�����һ����Ϣ���Խ�������
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


Server::Server() {
    // Create a socket
    this->server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (this->server_socket == INVALID_SOCKET) {
        std::cerr << ("Error creating socket: %d\n", WSAGetLastError()) << std::endl;
    }
    GetWirelessIP();
    std::cout<<"Server IP: "<<Ip<<std::endl;
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
        // �ȴ��߳����
        WaitForSingleObject(hReceiverThread, INFINITE);
        // �ر��߳̾��
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
    delete online_manager;
}