#pragma once
#include <unordered_map>
#include <string>
#include <chrono>
#include <mutex>

struct ClientInfo {
    std::string ip_address;
    int port;
    bool is_online;
    std::chrono::steady_clock::time_point last_seen;
};

class OnlineManager {
private:
    std::unordered_map<std::string, ClientInfo>& client_map;
    std::mutex map_mutex;
    int timeout_seconds = 60;

public:
    OnlineManager(std::unordered_map<std::string, ClientInfo>& map);

    void addOrUpdateClient(const std::string& ip, int port, bool is_online);
    void removeClient(const std::string& ip);
    void cleanupOfflineClients();
    void printOnlineClients();
};
