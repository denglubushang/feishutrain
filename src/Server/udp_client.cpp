#pragma once
#include "udp_client.h"

std::string CalculateBroadcastAddress(const std::string& ip, const std::string& subnetMask) {
    in_addr ipAddr, maskAddr;
    inet_pton(AF_INET, ip.c_str(), &ipAddr);
    inet_pton(AF_INET, subnetMask.c_str(), &maskAddr);

    in_addr broadcast;
    broadcast.S_un.S_addr = ipAddr.S_un.S_addr | ~maskAddr.S_un.S_addr;

    char broadcastStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &broadcast, broadcastStr, sizeof(broadcastStr));
    return broadcastStr;
}

void Client::ClientGetWirelessIP() {
    ULONG bufferSize = 15000;
    PIP_ADAPTER_ADDRESSES pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(bufferSize);
    if (!pAddresses) return;

    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, pAddresses, &bufferSize) != NO_ERROR) {
        free(pAddresses);
        return;
    }

    for (PIP_ADAPTER_ADDRESSES pAdapter = pAddresses; pAdapter; pAdapter = pAdapter->Next) {
        if (pAdapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        if (pAdapter->OperStatus != IfOperStatusUp) continue;
        if (pAdapter->IfType != IF_TYPE_IEEE80211) continue;

        for (PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pAdapter->FirstUnicastAddress;
            pUnicast != nullptr;
            pUnicast = pUnicast->Next) {
            if (pUnicast->Address.lpSockaddr->sa_family == AF_INET) {
                sockaddr_in* sin = (sockaddr_in*)pUnicast->Address.lpSockaddr;
                char ipStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &sin->sin_addr, ipStr, sizeof(ipStr));
                this->ip_ = ipStr;

                if (pUnicast->OnLinkPrefixLength > 0 && pUnicast->OnLinkPrefixLength <= 32) {
                    unsigned long mask = htonl(0xFFFFFFFF << (32 - pUnicast->OnLinkPrefixLength));
                    char maskStr[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &mask, maskStr, sizeof(maskStr));
                    this->subnet_mask_ = maskStr;
                }
                break;
            }
        }

        if (!this->ip_.empty() && !this->subnet_mask_.empty()) break;
    }

    free(pAddresses);
}

DWORD WINAPI Client::ClientBroadcastThread(LPVOID lpParam)
{
    SendParams* params = static_cast<SendParams*>(lpParam);
    struct sockaddr_in target_address;
    target_address.sin_family = AF_INET;
    target_address.sin_port = htons(params->target_port_);
    if (inet_pton(AF_INET, params->broadcast_ip_.c_str(), &target_address.sin_addr) <= 0) {
        std::cerr << "Error setting broadcast address" << std::endl;
        delete params;
        return -1;
    }

    const char *msg = params->message_.c_str();
    while(true)
    {
        if (sendto(params->client_->client_socket_, msg, strlen(msg), 0, (struct sockaddr *)&target_address, sizeof(target_address)) == SOCKET_ERROR)
        {
            std::cerr << "Error sending message: " << WSAGetLastError() << std::endl;
            delete params;
            return -1;
        }
        Sleep(3000);
    }
    delete params;
    return 0;
}
void Client::close() {
    if (this->client_socket_ != INVALID_SOCKET)
    {
        closesocket(this->client_socket_);
    }
}
Client::Client()
{
    // Create a socket
    this->client_socket_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (this->client_socket_ == INVALID_SOCKET)
    {
        std::cerr << ("Error creating socket: %d\n", WSAGetLastError()) << std::endl;
    }

    ClientGetWirelessIP();

    // Set socket broadcast option
    BOOL broad_cast = TRUE;
    if (setsockopt(this->client_socket_, SOL_SOCKET, SO_BROADCAST, (char *)&broad_cast, sizeof(broad_cast)) == SOCKET_ERROR)
    {
        std::cerr << ("Error setting broadcast option: %d\n", WSAGetLastError()) << std::endl;
    }
}

int Client::ClientBroadcast(int target_port)
{
    if (this->ip_.empty() || this->subnet_mask_.empty()) {
        std::cerr << "Error: Cannot get wireless IP or subnet mask." << std::endl;
        return -1;
    }

    std::string broadcast_ip = CalculateBroadcastAddress(this->ip_, this->subnet_mask_);
    std::string message = "discovery " + this->ip_;

    SendParams *params = new SendParams{this, target_port, message, broadcast_ip};
    HANDLE hThread = CreateThread(nullptr, 0, ClientBroadcastThread, params, 0, nullptr);
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
