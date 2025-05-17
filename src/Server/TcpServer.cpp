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
    server_Addr_.sin_addr.s_addr = INADDR_ANY; // ������������
    server_Addr_.sin_port = htons(8080);       // �˿� 8080
    if (bind(server_Socket_, (sockaddr*)&server_Addr_, sizeof(server_Addr_)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(server_Socket_);
        WSACleanup();
        exit(1);
    }

    // ��ʼ����
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
            std::cout << "������ȷ\n";
            send(accept_client_Socket_, "ok", strlen("ok"), 0);
            break;
        }
        else {
            send(accept_client_Socket_, "try again", strlen("try again"), 0);
        }
    }
}


std::string TcpServer::generateUniqueFileName(const std::string& original_filename) {
    std::string base_name = original_filename;
    std::string extension = "";

    // �����ļ�������չ��
    size_t dot_pos = original_filename.find_last_of('.');
    if (dot_pos != std::string::npos) {
        base_name = original_filename.substr(0, dot_pos);
        extension = original_filename.substr(dot_pos);
    }

    std::string unique_name = original_filename;
    std::set<std::string> existing_files = GetFilesInDirectory(); // ��������Ѿ���ȡdownload�ļ����е��ļ�

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
        std::cout << "�޷�����Կ�ļ�" << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(keyfile, line)) {
        if (line.find("AES_KEY=") == 0) {
            std::string key_b64 = line.substr(8); // ����"AES_KEY="
            aes_key = base64_decode(key_b64);
        }
        else if (line.find("HMAC_KEY=") == 0) {
            std::string key_b64 = line.substr(9); // ����"HMAC_KEY="
            hmac_key = base64_decode(key_b64);
        }
    }

    keyfile.close();
    return (!aes_key.empty() && !hmac_key.empty());
}

//�ҵ������ļ��Ŀ���
int TcpServer::Hash_Receive_Resume(SOCKET client_sock, const std::string& filename) {
    int start_chunk = 0;
    std::string unique_name = GetUniqueNameFromMap(filename);
    const std::string temp_file = "download/" + unique_name; // �ļ�·��  -- ����ж���ļ��Ļ���Ҫ�޸�

    try {
        if (std::filesystem::exists(temp_file)) {
            // ��ȡ�Ѵ����ļ���С
            uintmax_t file_size = std::filesystem::file_size(temp_file);
            const int chunk_size = 1024 * 64; // ��ͻ���һ�µĿ��С

            // ��������ɵĿ���������ȡ����
            start_chunk = static_cast<int>((file_size + chunk_size - 1) / chunk_size);

            std::cout << "[����] �ļ�: " << filename
                << " ��ʱ�ļ���С: " << file_size
                << " ������ʼ��: " << start_chunk << "\n";
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "�ļ����ʴ���: " << e.what() << std::endl;
        start_chunk = 0;
    }
    // ������ʼ��ţ�����ԭ�����߼���
    int sent_bytes = 0;
    while (sent_bytes < sizeof(int)) {
        int bytes = send(client_sock,
            reinterpret_cast<const char*>(&start_chunk) + sent_bytes,
            sizeof(int) - sent_bytes, 0);
        if (bytes <= 0) {
            std::cerr << "��ʼ�鷢��ʧ��\n";
            return -1;
        }
        sent_bytes += bytes;
    }
    return start_chunk;
}

void discard_socket_recv_buffer(SOCKET sock) {
    char buf[4096]; // ��ʱ���ջ�����
    fd_set readfds;
    timeval timeout;

    while (true) {
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);

        timeout.tv_sec = 0;
        timeout.tv_usec = 100000; // ���� 100ms ÿ��

        int ret = select(0, &readfds, NULL, NULL, &timeout);
        if (ret <= 0) {
            // ��ʱ�����˵�� socket ��ǰû�и���ɶ�������
            break;
        }

        int received = recv(sock, buf, sizeof(buf), 0);
        if (received <= 0) {
            break; // �Է��ر����ӻ����
        }
    }
}

int TcpServer::Hash_Receive(SOCKET& accept_client_Socket_) {
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
    uint64_t file_size = fileinformation.information.filesize;
    int segment_num = (file_size - 1) / (1024 * 64) + 1;
    uint64_t received_total_size = 0;
    int state = 0;
    //��ʼ���ļ���
    std::string unique_name = filename;
    // ����Ψһ����ʱ�ļ���--ֻ��ȡ�ļ�������
    if (!fileinformation.information.is_continue) //����Ĭ�ϵ�is_continueΪfalse , ������ʱ�򲻻����������ļ���
    {
        char temp_path[MAX_PATH];
        char temp_filename[MAX_PATH];
        GetTempPathA(MAX_PATH, temp_path);
        GetTempFileNameA(temp_path, "FTS_", 0, temp_filename);
        unique_name = std::filesystem::path(temp_filename).filename().string();
        DeleteFileA(temp_filename);  // ɾ��ϵͳ��������ʱ�ļ�
        //����Ψһ��ʱ�ļ�����ԭ���ļ�����ӳ��
        std::ofstream mapfile("download/map.txt", std::ios::app);
        if (mapfile) {
            mapfile << filename << " " << unique_name << std::endl;
            mapfile.close();
        }
        else {
            std::cerr << "�޷��� map.txt д���ļ�ӳ����Ϣ" << std::endl;
        }
    }
    // ʹ�����ɵ�Ψһ�ļ�������������downloadĿ¼��
    std::string down_file = "download/" + unique_name;
    std::ios::openmode file_mode = std::ios::binary;

    if (fileinformation.information.is_continue) {
        int start_chunk = Hash_Receive_Resume(accept_client_Socket_, fileinformation.information.header);
        received_total_size = start_chunk * (1024 * 64);
        state = start_chunk;
        unique_name = GetUniqueNameFromMap(filename);
        down_file = "download/" + unique_name;
        file_mode |= std::ios::app;  // ����ʱʹ��׷��ģʽ
        std::cout << "����ģʽ���ļ�·��: " << down_file << std::endl;
    }

    std::ofstream file(down_file, file_mode);  // ʹ��ͳһ�ļ�������ȷģʽ
    if (!file) {
        std::cerr << "�޷������ļ�" << std::endl;
        exit(1);
    }
    std::cout << "��ʼ�����ļ� " << filename << " ����Ϊ�� " << fileinformation.information.filesize << "\n";
    std::string TransConfirm = "ok";
    static int k1 = 0;
    static int k2 = 100;
    // �������ݶ�
    for (int i = state; i < segment_num; i++) {
        DataSegment* data_segment = new DataSegment();
        int data_segment_size = sizeof(DataSegment);
        int received_segment_size = 0;

        while (received_segment_size < data_segment_size) {
            int bytes_received = recv(accept_client_Socket_, reinterpret_cast<char*>(data_segment) + received_segment_size, data_segment_size - received_segment_size, 0);
            if (bytes_received <= 0) {
                if (bytes_received == 0) {
                    
                    std::cout << "�������ر������ӡ�\n";
                }
                else {
                    std::cout << "�ڵ�" << i << "�����ݶ�ʱ���������ر������ӡ�\n";
                    std::cout << "�������ݶ�ʧ��: " << WSAGetLastError() << std::endl;
                }
                file.close();
                return -1;
            }
            received_segment_size += bytes_received;
        }

        // ��֤����������
        unsigned char calculated_hash[SHA256_DIGEST_LENGTH];
        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        const EVP_MD* md = EVP_sha256();
        unsigned int md_len = SHA256_DIGEST_LENGTH;

        // ʹ��HMAC-SHA256��֤����������
        std::vector<unsigned char> aes_key, hmac_key;
        if (read_keys(aes_key, hmac_key)) {
            EVP_MAC* mac = EVP_MAC_fetch(NULL, "HMAC", NULL);
            EVP_MAC_CTX* mac_ctx = EVP_MAC_CTX_new(mac);
            //ģ���ļ���
            if (k1 == i||k2==i) {
                hmac_key[0] = '1';
                k1--;
                k2--;
            }
            OSSL_PARAM params[2];
            params[0] = OSSL_PARAM_construct_utf8_string("digest", const_cast<char*>("SHA256"), 0);
            params[1] = OSSL_PARAM_construct_end();

            EVP_MAC_init(mac_ctx, hmac_key.data(), hmac_key.size(), params);
            EVP_MAC_update(mac_ctx, reinterpret_cast<const unsigned char*>(data_segment->data.filedata), static_cast<size_t>(data_segment->data.datasize));

            size_t out_len = SHA256_DIGEST_LENGTH;
            EVP_MAC_final(mac_ctx, calculated_hash, &out_len, SHA256_DIGEST_LENGTH);

            EVP_MAC_CTX_free(mac_ctx);
            EVP_MAC_free(mac);


            if (memcmp(calculated_hash, data_segment->data.hash, SHA256_DIGEST_LENGTH) != 0) {
                std::cout << "���ݶ� " << i << " ��ϣ��֤ʧ�ܣ����ݿ������𻵡�\n";
                // ����ѡ��������ж�
                delete data_segment;
                TransConfirm = "no";
                break;
            }
        }

        // ��������Ǽ��ܵģ���Ҫ����
        if (data_segment->Decrypt_data() != 0) {
            std::cout << "���ݶ� " << i << " ����ʧ�ܡ�\n";
            delete data_segment;
            TransConfirm = "no";
            break;
        }

        // д���ļ�
        file.write(data_segment->data.filedata, data_segment->data.datasize);
        received_total_size += data_segment->data.datasize;
    }

    file.close();

    //������Ҫ���ͻ��˷���һ����ɵ���Ϣ
    int sent_bytes = 0;
    int total_len = TransConfirm.size();
    if (TransConfirm != "ok") {
        std::cout << "�ļ�����ʧ�ܣ���Ҫ�ش�" << std::endl;
        
    }
    while (sent_bytes < total_len) {
        int bytes = send(accept_client_Socket_,
            TransConfirm.c_str() + sent_bytes,
            total_len - sent_bytes, 0);
        std::cout << bytes << std::endl;
        if (bytes <= 0) {
            std::cerr << "��ʼ�鷢��ʧ��\n";
            return -1;
        }
        sent_bytes += bytes;
    }
    if (TransConfirm != "ok") {
        
        discard_socket_recv_buffer(accept_client_Socket_);
        return -1;
    }
    else {
        Change_Downlad_FileName(filename, down_file);
        //
        RemoveFileMapping(filename, unique_name, "download/map.txt"); // ɾ��ӳ��
        std::cout << "�ļ��������" << std::endl;
        return 0;
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
        int flag = Hash_Receive(client_Socket_);
        while(flag != 0) {
            flag = Hash_Receive(client_Socket_);
        }

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

void TcpServer::Change_Downlad_FileName(std::string recvfile_name, const std::string& temp_file_path)
{
    std::cout << "�ļ���Ϊ" << recvfile_name << std::endl;

    // �������ļ�������չ��
    std::string extension;
    std::string base_name = recvfile_name;
    size_t dot_pos = recvfile_name.find_last_of('.');
    if (dot_pos != std::string::npos) {
        extension = recvfile_name.substr(dot_pos);           // ������ĺ�׺���� ".txt"
        base_name = recvfile_name.substr(0, dot_pos);        // ��������չ���Ĳ���
    }

    std::string final_name = base_name + extension;
    std::string unique_path = "download/" + final_name;
    int counter = 1;

    // ����ļ��Ѵ��ڣ�������ֺ�׺����չ��ǰ
    while (std::filesystem::exists(unique_path)) {
        final_name = base_name + "(" + std::to_string(counter) + ")" + extension;
        unique_path = "download/" + final_name;
        counter++;
    }

    try {
        std::filesystem::rename(temp_file_path, unique_path);
        std::cout << "�ļ�����������Ϊ: " << final_name << std::endl;
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "������ʧ��: " << e.what() << std::endl;
    }
}


std::string TcpServer::GetUniqueNameFromMap(const std::string& filename) {
    std::ifstream mapfile("download/map.txt");
    if (!mapfile) {
        std::cerr << "�޷��� map.txt �ļ�" << std::endl;
        return "";
    }

    std::string line, name, unique;
    while (std::getline(mapfile, line)) {
        std::istringstream iss(line);
        if (iss >> name >> unique) {
            if (name == filename) {
                return unique;
            }
        }
    }
    return "";
}

// ɾ�� map.txt �� filename ��ӳ����
void TcpServer::RemoveFileMapping(const std::string& filename,const std::string& unique_name, const std::string& map_path ) {
    std::ifstream infile(map_path);
    if (!infile.is_open()) {
        std::cerr << "�޷���ӳ���ļ�: " << map_path << std::endl;
        return;
    }

    std::vector<std::string> new_lines;
    std::string origin, temp;

    while (infile >> origin >> temp) {
        if (origin != filename || temp != unique_name) {
            new_lines.push_back(origin + " " + temp);
        }
    }
    infile.close();

    // ����д�� map.txt������ԭ�ļ���
    std::ofstream outfile(map_path, std::ios::trunc);
    if (!outfile.is_open()) {
        std::cerr << "�޷�д��ӳ���ļ�: " << map_path << std::endl;
        return;
    }

    for (const auto& line : new_lines) {
        outfile << line << std::endl;
    }
    outfile.close();
}