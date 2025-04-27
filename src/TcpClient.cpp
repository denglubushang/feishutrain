#include "TcpClient.h"
TcpClient::TcpClient() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed.\n");
        exit(1);
    }
    client_Socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_Socket_ == INVALID_SOCKET) {
        printf("socket() failed: %d\n", WSAGetLastError());
        WSACleanup();
        exit(1);
    }
}
TcpClient::~TcpClient() {
    closesocket(client_Socket_);
    WSACleanup();
}

void TcpClient::Controller() {
    std::cout << "客户端开始运行\n";
    std::cout << "以下是检测到的机器:\n";



    std::cout << "输入你要操作的机器序号：\n";
    int seq;
    std::cin >> seq;
    Connect("127.0.0.1");
    std::vector<std::string> files = GetFilesInDirectory();
    for (int i = 0;i < files.size();i++) {
        std::cout << i << ": " << files[i]<<"\n";
    }
    std::cout << "选择要发送的文件序号：";
    int seq_file;
    std::cin >> seq_file;
    SendFile("test.png");
}

void TcpClient::Connect(const char* tag_ip ) {
    server_Addr_.sin_family = AF_INET;
    server_Addr_.sin_port = htons(8080);  // 服务器端口
    inet_pton(AF_INET, tag_ip, &server_Addr_.sin_addr);  // 服务器 IP

    if (connect(client_Socket_, (sockaddr*)&server_Addr_, sizeof(server_Addr_)) == SOCKET_ERROR) {
        printf("connect() failed: %d\n", WSAGetLastError());
        closesocket(client_Socket_);
        WSACleanup();
        exit(1);
    }
    printf("Connected to server!\n");
    std::string in_password;
    while (1) {
        std::cout << "输入密码：";
        std::cin >> in_password;
        int bytesSent = send(client_Socket_, in_password.c_str(), strlen(in_password.c_str()), 0);
        if (bytesSent == SOCKET_ERROR) {
            printf("send() failed: %d\n", WSAGetLastError());
            closesocket(client_Socket_);
            WSACleanup();
            exit(1);
        }
        char buffer[1024];
        std::string tag_rev = "ok";
        int bytesReceived = recv(client_Socket_, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';  // 确保字符串以 NULL 结尾
            if (tag_rev.compare(0, tag_rev.size(), buffer, bytesReceived)==0) {
                std::cout << "可以传输以下文件：\n";

                break;
            }
        }
        else if (bytesReceived == 0) {
            printf("Server closed the connection.\n");
        }
        else {
            printf("recv() failed: %d\n", WSAGetLastError());
        }
    }

}

std::vector<std::string> TcpClient::GetFilesInDirectory() {
    std::vector<std::string> files;

    for (const auto& entry : std::filesystem::directory_iterator("./")) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path().filename().string());
        }
    }
    return files;
}

void TcpClient::SendFile(std::string tag_file_name) {
    static char buffer[1024*1014];
    int check_size = 1024 * 1024;
    std::string header = tag_file_name + "\r\n";
    std::ifstream file(tag_file_name, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cout << "文件打开失败";
    }
    std::size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    header += std::to_string(fileSize) + "\r\n";
    size_t headneed_send_len = header.size(),head_sended_len=0;
    while (head_sended_len < headneed_send_len) {
        size_t send_len=send(client_Socket_, header.data() + head_sended_len, headneed_send_len - head_sended_len, 0);
        head_sended_len += send_len;
    }
    while (true) {
        file.read(buffer, check_size);
        size_t byte_read = file.gcount();
        while (1) {
            size_t send_len = 0;
            send_len += send(client_Socket_, buffer, byte_read, 0);
            if (send_len == byte_read) {
                break;
            }
        }
        if (file.eof()) break;
    }
    std::cout << "发送完成";
}
