#include "TcpServer.h"
TcpServer::TcpServer(std::string s):password(s) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        exit(1);
    }
    server_Socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_Socket_ == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        exit(1);
    }
    server_Addr_;
    server_Addr_.sin_family = AF_INET;
    server_Addr_.sin_addr.s_addr = INADDR_ANY; // 监听所有网卡
    server_Addr_.sin_port = htons(8080);       // 端口 8080
    if (bind(server_Socket_, (sockaddr*)&server_Addr_, sizeof(server_Addr_)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(server_Socket_);
        WSACleanup();
        exit(1);
    }

    // 开始监听
    if (listen(server_Socket_, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(server_Socket_);
        WSACleanup();
        exit(1);
    }
    std::cout << "Server is running on port 8080..." << std::endl;
}

TcpServer::~TcpServer() {
    closesocket(server_Socket_);
    WSACleanup();
}

void TcpServer::Connect(SOCKET& accept_client_Socket_) {
    memset(buffer,'\0', sizeof(buffer));
    std::string rec_password;
    int buffer_len = 0;
    int state = 0;
    while(1){
        buffer_len = recv(accept_client_Socket_, buffer, sizeof(buffer), 0);
        if (buffer_len == SOCKET_ERROR) {
            std::cerr << "Recv failed: " << WSAGetLastError() << std::endl;
        }
        else if (buffer_len == 0) {
            std::cout << "Client disconnected." << std::endl;
            exit(1);
        }
        if (password.compare(0, password.size(), buffer, buffer_len) == 0) {
            std::cout << "密码正确\n";
            send(accept_client_Socket_, "ok", strlen("ok"), 0);
            break;
        }
        else {
            send(accept_client_Socket_, "try again", strlen("try again"), 0);
        }
    }
}

void TcpServer::Receive(SOCKET& accept_client_Socket_) {
    std::ofstream file("test.txt", std::ios::binary);
    int state = 0;
    if (!file) {
        std::cerr << "无法打开文件！" << std::endl;
        exit(1);
    }
    size_t need_recve_byte = 0, received_byte=0;
    std::string file_name,date_len;
    static char buffer[1024 * 1024];
    while (1) {
        size_t need_dispose_byte_len = recv(accept_client_Socket_, buffer, sizeof(buffer), 0);
        size_t disposed_byte_len = 0;
        if (state == 0) {
            for (; disposed_byte_len < need_dispose_byte_len;disposed_byte_len++) {
                if (buffer[disposed_byte_len] != '\n') {
                    file_name += buffer[disposed_byte_len];
                }
                else {
                    if (file_name[file_name.size() - 1] == '\r') {
                        state = 1;
                        //文件名暂未处理
                        disposed_byte_len++;
                        break;
                    }
                }
            }
        }
        if (state == 1) {
            for (; disposed_byte_len < need_dispose_byte_len;disposed_byte_len++) {
                if (buffer[disposed_byte_len] != '\n') {
                    date_len += buffer[disposed_byte_len];
                }
                else {
                    if (file_name[file_name.size() - 1] == '\r') {
                        state = 2;
                        disposed_byte_len++;
                        need_recve_byte = std::stoul(date_len);
                        break;
                    }
                }
            }
        }
        if (state == 2) {
            file.write(buffer + disposed_byte_len, need_dispose_byte_len - disposed_byte_len);
            received_byte += need_dispose_byte_len - disposed_byte_len;
            disposed_byte_len = need_dispose_byte_len;
            if (!file.good()) {
                std::cerr << "写入失败";
            }
        }
        if (received_byte == need_recve_byte) {
            std::cout << "文件接收完毕";
            break;
        }

    }
}

void TcpServer::EventListen() {
    sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);
    while (1) {
        SOCKET client_Socket_ = accept(server_Socket_, (sockaddr*)&clientAddr, &clientAddrSize);
        if (client_Socket_ == INVALID_SOCKET) {
            std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
            closesocket(server_Socket_);
            WSACleanup();
            exit(1);
        }
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
        std::cout << "Client connected: " << clientIP << std::endl;
        Connect(client_Socket_);
        Receive(client_Socket_);
    }

}
