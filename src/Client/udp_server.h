#pragma once
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h> // ?í?? Windows ???????·????
#pragma comment(lib, "Ws2_32.lib")
#include <string>
#include <unordered_map>
#include "online_manager.h"

//struct ClientInfo {
//    std::string ip_address;
//    int port;
//    bool is_online;
//};

class Server
{
private:
    SOCKET server_socket;
    std::string Ip;
    std::string subnetMask;
    std::unordered_map<std::string, ClientInfo> client_map;
    HANDLE hReceiverThread;
    bool stop_thread = false;
    OnlineManager* online_manager;  // 添加指针成员
    std::mutex client_map_mutex;
    std::thread check_thread;
    std::atomic<bool> check_done;
    std::atomic<bool> check_success;
    std::string selected_ip;

    bool GetWirelessIP();
    void addClient(const std::string& ip, int port, bool is_online);
    static DWORD WINAPI ServerReceiverThread(LPVOID lpParam);
    void CheckClientsAndSelect();

public:
    Server();
    ~Server();
    void close();
    int StartReceiverThread();
    int StopReceiverThread();
    int SendOfflineMessage();
    std::string SelectClientIP();
    bool HasClients();
    void StartCheckThread();
    void WaitForCheckThread();
    bool GetCheckSuccess() const;
};