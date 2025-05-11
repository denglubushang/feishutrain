#include "online_manager.h"
#include <iostream>

OnlineManager::OnlineManager(std::unordered_map<std::string, ClientInfo>& map) : client_map(map) {}

void OnlineManager::addOrUpdateClient(const std::string& ip, int port, bool is_online) {
    std::lock_guard<std::mutex> lock(map_mutex);
    auto now = std::chrono::steady_clock::now();
    auto it = client_map.find(ip);
    if (it != client_map.end()) {
        it->second.last_seen = now;
        it->second.port = port;
        it->second.is_online = is_online;
    } else {
        client_map[ip] = ClientInfo{ ip, port, is_online, now };
    }
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
    auto now = std::chrono::steady_clock::now();
    for (const auto& [ip, info] : client_map) {
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now - info.last_seen).count();
        std::cout << "Client IP: " << ip << ", Port: " << info.port
                  << ", Last seen: " << seconds << " seconds ago" << std::endl;
    }
}
