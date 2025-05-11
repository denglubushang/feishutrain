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
    std::cout << "�ͻ��˿�ʼ����\n";
    std::cout << "�����Ǽ�⵽�Ļ���:\n";
    //��ʾ�ӹ㲥ģ���⵽�Ļ���
    Server server;
    Client client;
    server.StartReceiverThread();
    client.ClientBroadcast(8888);

    std::cout << "������Ҫ�����Ļ�����ţ�\n";
    std::string server_ip;
    std::cin >> server_ip;
    server.StopReceiverThread();

    Connect(server_ip.c_str());
    std::vector<std::string> files = GetFilesInDirectory();
    for (int i = 0;i < files.size();i++) {
        std::cout << i << ": " << files[i]<<"\n";
    }
    std::cout << "ѡ��Ҫ���͵��ļ���ţ�";
    int seq_file;
    std::cin >> seq_file;
    SendFile(files[seq_file]);
}

void TcpClient::Connect(const char* tag_ip ) {
    server_Addr_.sin_family = AF_INET;
    server_Addr_.sin_port = htons(8080);  // �������˿�
    inet_pton(AF_INET, tag_ip, &server_Addr_.sin_addr);  // ������ IP

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
        std::cout << "���������벻���� " << PassMaxlen - 1 << " ���ַ�: ";
        if (fgets(in_password.password, PassMaxlen, stdin) == nullptr) {
            // �������
            std::cerr << "����������������롣" << std::endl;
            continue;
        }
        size_t len = strlen(in_password.password);

        // ����Ƿ�������з�����ʾδ���ضϣ�
        if (len > 0 && in_password.password[len - 1] == '\n') {
            in_password.password[len - 1] = '\0';  // ȥ�����з�
            len--;
        }
        else {
            // ���뱻�ضϣ���ʾ������ջ�����
            std::cout << "����������벻Ҫ���� " << PassMaxlen - 1 << " ���ַ���\n";
            // �����������ʣ���ַ�
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            continue;
        }

        // ����������ݳ����Ƿ񳬹�����
        if (len > PassMaxlen - 1) {
            std::cout << "�������ݹ��������������롣\n";
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
            buffer[bytesReceived] = '\0';  // ȷ���ַ����� NULL ��β
            if (tag_rev.compare(0, tag_rev.size(), buffer, bytesReceived)==0) {
                std::cout << "���Դ��������ļ���\n";

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
    ProgressBar progress_bar;
    std::ifstream file(tag_file_name, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cout << "�ļ���ʧ��";
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
    std::uint64_t sended_total_size = 0;
    while (segment_seq<segment_num) {
        datasegment.init();
        datasegment.Set_segid(segment_seq++);
        file.read(datasegment.data.filedata, File_segdata_size);
        size_t byte_read = file.gcount();
        datasegment.Set_datasize(byte_read);
        datasegment.Encrypt_data();  // ��������
        datasegment.Set_hash();      // ����HMAC-SHA256��ϣ
        int data_segment_size = sizeof(DataSegment), sended_size = 0;
        while (sended_size < data_segment_size) {
            int temp = send(client_Socket_, reinterpret_cast<const char*>(&datasegment) + sended_size, data_segment_size - sended_size, 0);
            if (temp == SOCKET_ERROR) {
                printf("send() failed: %d\n", WSAGetLastError());
                closesocket(client_Socket_);
                WSACleanup();
                exit(1);
            }
            sended_total_size += temp;
            progress_bar.update(sended_total_size, fileSize);
            sended_size += temp;
        }
        if (file.eof()) 
        {
            Sleep(100);
            break;
        }
    }
    //progress_bar.update(fileSize, fileSize);
    progress_bar.finish(fileSize);
    std::cout << "�ļ��������\n";
}