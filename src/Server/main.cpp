#include<iostream>
#include<fstream>
#include"TcpServer.h"
#pragma comment(lib, "Ws2_32.lib")
int main() {
	std::ifstream passfile("password.txt");
	if (!passfile) {
		std::cout << "文件打开时失败";
		exit(1);
	}
	std::string password;
	passfile >> password;
	TcpServer myServer(password);
	myServer.EventListen();
}