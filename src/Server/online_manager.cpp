#include "online_manager.h"
#include <iostream>

OnlineManager::OnlineManager(std::unordered_map<std::string, ClientInfo>& map) : client_map(map) {}

void OnlineManager::updateClient(const std::string& ip, int port) {
    std::lock_guard<std::mutex> lock(map_mutex);
    auto now = std::chrono::steady_clock::now();
    client_map[ip] = ClientInfo{ ip, port, true, now };
}

void OnlineManager::removeClient(const std::string& ip) {
    std::lock_guard<std::mutex> lock(map_mutex);
    client_map.erase(ip);
}

void OnlineManager::cleanupOfflineClients() {
    std::lock_guard<std::mutex> lock(map_mutex);
    auto now = std::chrono::steady_clock::now();
    for (auto it = client_map.begin(); it != client_map.end(); ) {
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.last_seen).count();
        if (duration > timeout_seconds) {
            std::cout << "Client timeout, removing: " << it->first << std::endl;
            it = client_map.erase(it);
        } else {
            ++it;
        }
    }
}

void OnlineManager::printOnlineClients() {
    std::lock_guard<std::mutex> lock(map_mutex);
    for (std::unordered_map<std::string, ClientInfo>::const_iterator it = client_map.begin(); it != client_map.end(); ++it) {
        std::cout << "Client IP: " << it->first << ", Port: " << it->second.port << std::endl;
    }
}
