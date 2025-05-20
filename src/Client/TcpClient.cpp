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
    ProgressBar progress_bar;
}
TcpClient::~TcpClient() {
    closesocket(client_Socket_);
    WSACleanup();
}

void TcpClient::Controller() {
    std::cout << "客户端开始运行\n";
    std::cout << "以下是检测到的机器:\n";
    //显示从广播模块检测到的机器
    Server server;
    Client client;
    server.StartReceiverThread();
    client.ClientBroadcast(8888);

    /*std::cout << "输入你要操作的机器序号：\n";
    std::string server_ip;
    std::cin >> server_ip;*/

	server.StartCheckThread();
    server.WaitForCheckThread();
    if (!server.GetCheckSuccess()) {
        std::cerr << "检测客户端失败或未选择有效IP，程序退出" << std::endl;
        return ;
    }
    std::string target_ip = server.SelectClientIP();

    server.StopReceiverThread();

    Connect(target_ip.c_str());
    std::cout << "请输入要查看的目录路径：";
    std::string directory_path;
    std::cin >> directory_path;
    std::vector<std::string> files = GetFilesInDirectory(directory_path);
    std::cout << "可以传输以下文件：\n";

    for (int i = 0;i < files.size();i++) {
        std::cout << i << ": " << files[i]<<"\n";
    }
    std::cout << "选择要发送的文件序号：";
    int seq_file;
    std::cin >> seq_file;
    if (seq_file < 0 || seq_file >= files.size()) {
        std::cerr << "输入的文件序号非法，程序终止。\n";
        return;
    }
    std::cout << "是否是续传：y/n" << std::endl;
    char tag_continue;
    std::cin >> tag_continue;
    progress_bar.start();

    std::string selected_file = directory_path + "/" + files[seq_file];
    if (tag_continue == 'y') {
        
        int flag = SendContinue(selected_file);
        while (flag != 0) {
            std::cout << "\n文件传输失败\n";
            std::cout << "续传：x,重传：c,退出：q\n";
            char ch;
            std::cin >> ch;
            progress_bar.start();
            if (ch == 'x') {
                flag = SendContinue(selected_file);
            }
            else if (ch == 'c') {
                flag = SendFile(selected_file);
            }
            else {
                std::cout  << "传输结束，程序终止。\n";
                return;
            }
        }

    }
    else
    {   
        int flag = SendFile(selected_file);
        while (flag != 0) {
            std::cout << "\n文件传输失败\n";
            std::cout << "续传：x,重传：c,退出：q\n";
            char ch;
            std::cin >> ch;
            progress_bar.start();
            if (ch == 'x') {
                flag = SendContinue(selected_file);
            }
            else if (ch == 'c') {
                flag = SendFile(selected_file);
            }
            else {
                std::cout << "传输结束，程序终止。\n";
                return;
            }
        }
    }
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
    logSeg in_password;
    while (1) {
        in_password.init();
        std::cout << "请输入密码不超过 " << PassMaxlen - 1 << " 个字符: ";
        if (fgets(in_password.password, PassMaxlen, stdin) == nullptr) {
            // 输入错误
            std::cerr << "输入错误，请重新输入。" << std::endl;
            continue;
        }
        size_t len = strlen(in_password.password);

        // 检查是否包含换行符（表示未被截断）
        if (len > 0 && in_password.password[len - 1] == '\n') {
            in_password.password[len - 1] = '\0';  // 去除换行符
            len--;
        }
        else {
            // 输入被截断，提示错误并清空缓冲区
            std::cout << "输入过长，请不要超过 " << PassMaxlen - 1 << " 个字符。\n";
            // 清除缓冲区中剩余字符
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            continue;
        }

        // 检查输入内容长度是否超过限制
        if (len > PassMaxlen - 1) {
            std::cout << "输入内容过长，请重新输入。\n";
            continue;
        }

        int bytesSent = send(client_Socket_, in_password.password, PassMaxlen, 0);
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
                //std::cout << "可以传输以下文件：\n";

                break;
            }
        }
        else if (bytesReceived == 0) {
            printf("Server closed the connection.\n");
            break;
        }
        else {
            printf("recv() failed: %d\n", WSAGetLastError());
            break;
        }
    }

}

std::vector<std::string> TcpClient::GetFilesInDirectory(const std::string& directory_path) {
    std::vector<std::string> files;

    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory_path)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path().filename().string()); // 只使用文件名
            }
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "访问目录失败: " << e.what() << std::endl;
    }
    return files;
}


int TcpClient::SendContinue(std::string tag_file_name) {
    HeadSegment head_segment;
    std::ifstream file(tag_file_name, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "文件打开失败\n";
        return -1;
    }
    std::uint64_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);  // 初始化到文件头

    // 设置续传请求头
    head_segment.init();
    head_segment.Set_header(tag_file_name);
    head_segment.Set_filesize(fileSize);
    head_segment.Set_Boolean(true);

    // 发送请求头（确保完整发送）
    int headseg_size = sizeof(head_segment);
    int haved_send = 0;
    while (haved_send < headseg_size) {
        int temp = send(client_Socket_, reinterpret_cast<const char*>(&head_segment) + haved_send, headseg_size - haved_send, 0);
        if (temp == SOCKET_ERROR) {
            std::cerr << "send() failed: " << WSAGetLastError() << std::endl;
            closesocket(client_Socket_);
            WSACleanup();
            exit(1);
        }
        haved_send += temp;
    }

    // 接收起始块号（确保完整接收）
    int start_chunk = 0;
    int received = 0;
    while (received < sizeof(int)) {
        int bytes = recv(client_Socket_, reinterpret_cast<char*>(&start_chunk) + received, sizeof(int) - received, 0);
        if (bytes <= 0) {
            std::cerr << "接收块号失败\n";
            closesocket(client_Socket_);
            exit(1);
        }
        received += bytes;
    }
    //std::cout << "续传文件块号：" << start_chunk << "\n";

    // 调整文件读取位置
    file.seekg(start_chunk * File_segdata_size, std::ios::beg);
    int segment_num = (fileSize - 1) / File_segdata_size + 1;
    int segment_seq = start_chunk ;  // 直接使用服务端返回的块号

    
    std::uint64_t sended_total_size = start_chunk * File_segdata_size;
    progress_bar.reset(sended_total_size);
    while (segment_seq < segment_num && file) {
        DataSegment* datasegment = new DataSegment();
        datasegment->init();
        datasegment->Set_segid(segment_seq++);
        file.read(datasegment->data.filedata, File_segdata_size);
        size_t byte_read = file.gcount();
        if (byte_read == 0) break;

        datasegment->Set_datasize(byte_read);
        datasegment->Encrypt_data();
        datasegment->Set_hash();

        int data_segment_size = sizeof(DataSegment);
        int sended_size = 0;

        while (sended_size < data_segment_size) {
            fd_set writefds, readfds;
            FD_ZERO(&writefds);
            FD_ZERO(&readfds);
            FD_SET(client_Socket_, &writefds);
            FD_SET(client_Socket_, &readfds);

            timeval timeout;
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;

            int ret = select(client_Socket_ + 1, &readfds, &writefds, NULL, &timeout); 
            if (ret < 0) {
                std::cerr << "select() failed: " << WSAGetLastError() << std::endl;
                closesocket(client_Socket_);
                WSACleanup();
                exit(1);
            }
            // 可读，接收服务端消息
            if (FD_ISSET(client_Socket_, &readfds)) {
                char buf[64] = { 0 }; // 足够大
                int received_bytes = 0;
                int total_len = 2; // 例如"ok"长度
                while (received_bytes < total_len) {
                    int bytes = recv(client_Socket_, buf + received_bytes, total_len - received_bytes, 0);
                    if (bytes <= 0) {
                        std::cerr << "接收服务端确认块失败\n";
                        break;
                    }
                    received_bytes += bytes;
                }
                std::string TransConfirm_recv(buf, received_bytes);
                head_segment.init();
                delete datasegment;
                datasegment = nullptr;
                file.close();
                return -1;

            }
            // 可写，继续发送
            if (FD_ISSET(client_Socket_, &writefds)) {
                int temp = send(client_Socket_, reinterpret_cast<const char*>(datasegment) + sended_size, data_segment_size - sended_size, 0);
                //std::cout << "文件传了" << temp << std::endl;
                if (temp == SOCKET_ERROR) {
                    std::cerr << "send() failed: " << WSAGetLastError() << std::endl;
                    closesocket(client_Socket_);
                    WSACleanup();
                    exit(1);
                }
                sended_total_size += temp;
                progress_bar.update(sended_total_size, fileSize);
                sended_size += temp;
            }

            
        }
        if (file.eof()) {
            break;
        }      
    }
    char buf[64] = { 0 }; // 足够大
    int received_bytes = 0;
    int total_len = 2; // 例如"ok"长度
    while (received_bytes < total_len) {
        int bytes = recv(client_Socket_, buf + received_bytes, total_len - received_bytes, 0);
        if (bytes <= 0) {
            std::cerr << "接收服务端确认块失败\n";
            break;
        }
        received_bytes += bytes;
    }
    std::string TransConfirm_recv(buf, received_bytes);
    if (TransConfirm_recv != "ok")
    {
        return -1;
    }
    // 最终状态更新
    progress_bar.finish(fileSize);
    std::cout << "文件发送完毕\n";
    return 0;
}


int TcpClient::SendFile(std::string tag_file_name) {
    HeadSegment head_segment;
    std::ifstream file(tag_file_name, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cout << "文件打开失败";
    }
    std::uint64_t fileSize = file.tellg(),needsend_size=fileSize;
    file.seekg(0, std::ios::beg);
    int segment_num = (fileSize - 1) / (1024 * 64) + 1,segment_seq=0;
    head_segment.Set_header(tag_file_name);
    head_segment.Set_filesize(fileSize);
    int headseg_size = sizeof(head_segment),haved_send=0;
    while (haved_send<headseg_size) {
        int temp= send(client_Socket_, reinterpret_cast<const char*>(&head_segment) + haved_send, headseg_size - haved_send, 0);
        if (temp == SOCKET_ERROR) {
            printf("send() failed: %d\n", WSAGetLastError());
            closesocket(client_Socket_);
            WSACleanup();
            exit(1);
        }
        haved_send += temp;
    }
    std::uint64_t sended_total_size = 0;
    while (segment_seq < segment_num) {
        DataSegment* datasegment = new DataSegment();
        datasegment->init();
        datasegment->Set_segid(segment_seq++);
        file.read(datasegment->data.filedata, File_segdata_size);
        size_t byte_read = file.gcount();
        datasegment->Set_datasize(byte_read);
        datasegment->Encrypt_data();  // 加密数据
        datasegment->Set_hash();      // 计算HMAC-SHA256哈希
        int data_segment_size = sizeof(DataSegment), sended_size = 0;

        while (sended_size < data_segment_size) {
            fd_set writefds, readfds;
            FD_ZERO(&readfds);
            FD_ZERO(&writefds);
            FD_SET(client_Socket_, &readfds);
            FD_SET(client_Socket_, &writefds);
            

            timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 10000;  // 10ms 微小等待，让CPU有机会响应

            int ret = select(client_Socket_ + 1, &readfds, &writefds, NULL, &timeout);
            if (ret < 0) {
                printf("select() failed: %d\n", WSAGetLastError());
                closesocket(client_Socket_);
                WSACleanup();
                exit(1);
            }
            

            // 可读，接收服务端消息
            if (FD_ISSET(client_Socket_, &readfds)) {
                char buf[64] = { 0 }; // 足够大
                int received_bytes = 0;
                int total_len = 2; // 例如"ok"长度
                while (received_bytes < total_len) {
                    int bytes = recv(client_Socket_, buf + received_bytes, total_len - received_bytes, 0);
                    if (bytes <= 0) {
                        std::cerr << "接收服务端确认块失败\n";
                        break;
                    }
                    received_bytes += bytes;
                }
                std::string TransConfirm_recv(buf, received_bytes);
                head_segment.init();
                delete datasegment;
                datasegment = nullptr;
                file.close();
                return -1;

            }

            // 可写，继续发送
            if (FD_ISSET(client_Socket_, &writefds)) {
                int temp = send(client_Socket_, reinterpret_cast<const char*>(datasegment) + sended_size, data_segment_size - sended_size, 0);
                /*std::cout << "data_segment_size: " << data_segment_size << std::endl;
                std::cout << "sended_size: " << sended_size << std::endl;
                std::cout << "temp: " << temp << std::endl;*/
                if (temp == SOCKET_ERROR) {
                    printf("send() failed: %d\n", WSAGetLastError());
                    closesocket(client_Socket_);
                    WSACleanup();
                    exit(1);
                }
                //std::cout << "发送第" << segment_seq << "块" << std::endl;
                sended_total_size += temp;
                progress_bar.update(sended_total_size, fileSize);
                sended_size += temp;
            }           
            
        }
        if (file.eof()) {
            break;
        }
    }
    char buf[64] = { 0 }; // 足够大
    int received_bytes = 0;
    int total_len = 2; // 例如"ok"长度
    while (received_bytes < total_len) {
        int bytes = recv(client_Socket_, buf + received_bytes, total_len - received_bytes, 0);
        if (bytes <= 0) {
            std::cerr << "接收服务端确认块失败\n";
            break;
        }
        received_bytes += bytes;
    }
    std::string TransConfirm_recv(buf, received_bytes);

    if (TransConfirm_recv != "ok")
    {
        return -1;
        
    }
    progress_bar.finish(fileSize);
    return 0;
}