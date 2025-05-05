#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include<fstream>
#include <iostream>
#include<string>
#include"TcpClient.h"
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

int main() {
	TcpClient myclient;
	myclient.Controller();
    return 0;
}