#include "TcpServer.h"
TcpServer::TcpServer(std::string s) :password(s) {
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
    logSeg recpassword;
    while (1) {
        recpassword.init();
        int recved_len = 0;
        while (recved_len < PassMaxlen) {
            int temp = recv(accept_client_Socket_, recpassword.password,PassMaxlen-recved_len , 0);
            recved_len += temp;
            if (temp == SOCKET_ERROR) {
                std::cerr << "Recv failed: " << WSAGetLastError() << std::endl;
            }
            else if (temp == 0) {
                std::cout << "Client disconnected." << std::endl;
                exit(1);
            }
        }
        
        if (password.compare(0, password.size(), recpassword.password, password.size()) == 0) {
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
    size_t need_recve_byte = 0, received_byte = 0;
    std::string file_name, date_len;
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

void TcpServer::Hash_Receive(SOCKET& accept_client_Socket_) {
    std::filesystem::path dirpath="download";
    std::filesystem::create_directory(dirpath);
    std::ofstream file("download/temp.txt", std::ios::binary);
    int state = 0;
    if (!file) {
        std::cerr << "无法打开文件！" << std::endl;
        exit(1);
    }
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_md5();
    unsigned int md_len = MD5_DIGEST_LENGTH;
    size_t need_recve_byte = sizeof(HeadSegment), received_byte = 0;
    HeadSegment fileinformation;
    while (received_byte < need_recve_byte) {
        int temp = recv(accept_client_Socket_, reinterpret_cast<char*>(&fileinformation) + received_byte, need_recve_byte - received_byte, 0);
        if (temp == SOCKET_ERROR) {
            std::cerr << "Recv failed: " << WSAGetLastError() << std::endl;
            exit(1);
        }
        received_byte += temp;
    }
    std::string filename(fileinformation.information.header);
    std::cout << "开始接受文件 " << fileinformation.information.header << " 长度为： " << fileinformation.information.filesize << "\n";
    need_recve_byte = fileinformation.information.filesize,received_byte=0;
    DataSegment datasegment;
    size_t dataseg_len = sizeof(DataSegment);
    while (received_byte < need_recve_byte) {
        size_t recved_seglen = 0;
        while (recved_seglen < dataseg_len) {
            int temp = recv(accept_client_Socket_, reinterpret_cast<char*>(&datasegment) + recved_seglen, dataseg_len - recved_seglen, 0);
            if (temp == SOCKET_ERROR) {
                std::cerr << "Recv failed: " << WSAGetLastError() << std::endl;
                exit(1);
            }
            recved_seglen +=temp;
        }
        unsigned char output_hash[MD5_DIGEST_LENGTH];
        // 初始化 MD5 计算
        EVP_DigestInit_ex(ctx, md, NULL);
        EVP_DigestUpdate(ctx, datasegment.data.filedata, static_cast<size_t>(datasegment.data.datasize));
        EVP_DigestFinal_ex(ctx, output_hash, &md_len);

        if (memcmp(output_hash, datasegment.data.hash, MD5_DIGEST_LENGTH) == 0) {
            file.write(datasegment.data.filedata, datasegment.data.datasize);
            received_byte += datasegment.data.datasize;
        }
        else {
            std::cout << "数据损坏";
            exit(1);
        }
    }
    EVP_MD_CTX_free(ctx);
    std::cout << "文件接收完毕\n";
    file.close();
    Change_Downlad_FileName(filename);
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
        Hash_Receive(client_Socket_);
    }

}

std::set<std::string> TcpServer::GetFilesInDirectory()
{
    std::set<std::string> files;

    for (const auto& entry : std::filesystem::directory_iterator("download")) {
        if (entry.is_regular_file()) {
            files.insert(entry.path().filename().string());
        }
    }
    return files;
}

void TcpServer::Change_Downlad_FileName(std::string recvfile_name)
{
    std::filesystem::rename("download/temp.txt", "download/" + recvfile_name);
}


