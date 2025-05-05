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
    //显示从广播模块检测到的机器


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
    SendFile(files[seq_file]);
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
    HeadSegment head_segment;
    std::ifstream file(tag_file_name, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cout << "文件打开失败";
    }
    std::uint64_t fileSize = file.tellg(),needsend_size=fileSize;
    file.seekg(0, std::ios::beg);
    int segment_num = (fileSize - 1) / (1024 * 511) + 1,segment_seq=0;
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
    DataSegment datasegment;
    
    while (segment_seq<segment_num) {
        datasegment.init();
        datasegment.Set_segid(segment_seq++);
        file.read(datasegment.data.filedata, File_segdata_size);
        size_t byte_read = file.gcount();
        datasegment.Set_datasize(byte_read);
        datasegment.Set_hash();
        int data_segment_size = sizeof(DataSegment), sended_size = 0;
        while (sended_size < data_segment_size) {
            int temp = send(client_Socket_, reinterpret_cast<const char*>(&datasegment) + sended_size, data_segment_size - sended_size, 0);
            if (temp == SOCKET_ERROR) {
                printf("send() failed: %d\n", WSAGetLastError());
                closesocket(client_Socket_);
                WSACleanup();
                exit(1);
            }
            sended_size += temp;
        }
        if (file.eof()) break;
    }
    std::cout << "文件发送完毕\n";
}