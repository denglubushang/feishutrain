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

std::string TcpServer::generateUniqueFileName(const std::string& original_filename) {
    std::string base_name = original_filename;
    std::string extension = "";

    // 分离文件名和扩展名
    size_t dot_pos = original_filename.find_last_of('.');
    if (dot_pos != std::string::npos) {
        base_name = original_filename.substr(0, dot_pos);
        extension = original_filename.substr(dot_pos);
    }

    std::string unique_name = original_filename;
    std::set<std::string> existing_files = GetFilesInDirectory(); // 这个函数已经获取download文件夹中的文件

    int counter = 1;
    while (existing_files.find(unique_name) != existing_files.end()) {
        unique_name = base_name + "(" + std::to_string(counter) + ")" + extension;
        counter++;
    }

    return unique_name;
}

std::vector<unsigned char> base64_decode(const std::string& input) {
    BIO* bio, * b64;

    int decodeLen = input.size();
    std::vector<unsigned char> buffer(decodeLen);

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_mem_buf(input.c_str(), input.length());
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

    int len = BIO_read(bio, buffer.data(), decodeLen);
    buffer.resize(len);

    BIO_free_all(bio);
    return buffer;
}

bool read_keys(std::vector<unsigned char>& aes_key, std::vector<unsigned char>& hmac_key) {
    std::ifstream keyfile("key.txt");
    if (!keyfile.is_open()) {
        std::cout << "无法打开密钥文件" << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(keyfile, line)) {
        if (line.find("AES_KEY=") == 0) {
            std::string key_b64 = line.substr(8); // 跳过"AES_KEY="
            aes_key = base64_decode(key_b64);
        }
        else if (line.find("HMAC_KEY=") == 0) {
            std::string key_b64 = line.substr(9); // 跳过"HMAC_KEY="
            hmac_key = base64_decode(key_b64);
        }
    }

    keyfile.close();
    return (!aes_key.empty() && !hmac_key.empty());
}


void TcpServer::Hash_Receive(SOCKET& accept_client_Socket_) {
    std::filesystem::path dirpath="download";
    if (!std::filesystem::exists(dirpath)) {
        std::filesystem::create_directory(dirpath);
    }
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
    std::string down_file = "download/temp.txt";
    std::ofstream file(down_file, std::ios::binary);
    int state = 0;
    if (!file) {
        std::cerr << "无法打开文件！" << std::endl;
        exit(1);
    }
    std::cout << "开始接受文件 " << filename << " 长度为： " << fileinformation.information.filesize << "\n";
    uint64_t file_size = fileinformation.information.filesize;
    int segment_num = (file_size - 1) / (1024 * 511) + 1;
    uint64_t received_total_size = 0;
    // 接收数据段
    for (int i = 0; i < segment_num; i++) {
        DataSegment data_segment;
        int data_segment_size = sizeof(DataSegment);
        int received_segment_size = 0;

        while (received_segment_size < data_segment_size) {
            int bytes_received = recv(accept_client_Socket_, reinterpret_cast<char*>(&data_segment) + received_segment_size, data_segment_size - received_segment_size, 0);
            if (bytes_received <= 0) {
                if (bytes_received == 0) {
                    std::cout << "服务器关闭了连接。\n";
                }
                else {
                    std::cout << "接收数据段失败: " << WSAGetLastError() << std::endl;
                }
                file.close();
                return;
            }
            received_segment_size += bytes_received;
        }

        // 验证数据完整性
        unsigned char calculated_hash[SHA256_DIGEST_LENGTH];
        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        const EVP_MD* md = EVP_sha256();
        unsigned int md_len = SHA256_DIGEST_LENGTH;

        // 使用HMAC-SHA256验证数据完整性
        std::vector<unsigned char> aes_key, hmac_key;
        if (read_keys(aes_key, hmac_key)) {
            EVP_MAC* mac = EVP_MAC_fetch(NULL, "HMAC", NULL);
            EVP_MAC_CTX* mac_ctx = EVP_MAC_CTX_new(mac);

            OSSL_PARAM params[2];
            params[0] = OSSL_PARAM_construct_utf8_string("digest", const_cast<char*>("SHA256"), 0);
            params[1] = OSSL_PARAM_construct_end();

            EVP_MAC_init(mac_ctx, hmac_key.data(), hmac_key.size(), params);
            EVP_MAC_update(mac_ctx, reinterpret_cast<const unsigned char*>(data_segment.data.filedata), static_cast<size_t>(data_segment.data.datasize));

            size_t out_len = SHA256_DIGEST_LENGTH;
            EVP_MAC_final(mac_ctx, calculated_hash, &out_len, SHA256_DIGEST_LENGTH);

            EVP_MAC_CTX_free(mac_ctx);
            EVP_MAC_free(mac);

            if (memcmp(calculated_hash, data_segment.data.hash, SHA256_DIGEST_LENGTH) != 0) {
                std::cout << "数据段 " << i << " 哈希验证失败，数据可能已损坏。\n";
                // 可以选择继续或中断
            }
        }

        // 如果数据是加密的，需要解密
        if (data_segment.Decrypt_data() != 0) {
            std::cout << "数据段 " << i << " 解密失败。\n";
            // 可以选择继续或中断
        }

        // 写入文件
        file.write(data_segment.data.filedata, data_segment.data.datasize);
        received_total_size += data_segment.data.datasize;
    }

    file.close();
    std::cout << "文件接收完毕\n";
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
    std::cout << "文件名为" << recvfile_name << std::endl;
    std::string unique_filename = generateUniqueFileName(recvfile_name);

    try {
        std::filesystem::rename("download/temp.txt", "download/" + unique_filename);
        std::cout << "文件已保存为: " << unique_filename << std::endl;
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "重命名失败: " << e.what() << std::endl;
    }
}


