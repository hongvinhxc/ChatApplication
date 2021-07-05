#include <iostream>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <process.h>
#include <string> 


#pragma comment(lib, "Ws2_32.lib")

#define BUFF_SIZE 2048

using namespace std;

int main(int argc, char* argv[]) {

	// Init port from command-line arguments
	int port = 5500;

	// Init winsock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		cout << "Version is not supported." << endl;
	}

	// Construct socket
	SOCKET listenSock;
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Bind address to socket
	sockaddr_in	serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
	if (bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr))) {
		cout << "Error! Cannot bind this address." << endl;
		return 0;
	}

	if (listen(listenSock, 10)) {
		cout << "Error!" << endl;
		return 0;
	}
	cout << "Server started." << endl;

	// Communicate with client
	sockaddr_in clientAddr;
	char buff[BUFF_SIZE], clientIP[INET_ADDRSTRLEN];
	int ret, clientAddrLen = sizeof(clientAddr), clientPort;

	SOCKET connSock;
	//accept request
	connSock = accept(listenSock, (sockaddr*)&clientAddr, &clientAddrLen);
	inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
	clientPort = ntohs(clientAddr.sin_port);

	while (true) {
		//receive message from client
		ret = recv(connSock, buff, BUFF_SIZE, 0);
		if (ret == 0) {
			cout << "Connection was closed." << endl;
			break;
		}
		if (ret == SOCKET_ERROR) {
			cout << "Error " << WSAGetLastError() << endl;
			break;
		}
		else {
			buff[ret] = '\0';
			cout << "Receive from client " << clientIP << ":" << clientPort << " " << buff << endl;
			
			//Echo to client
			ret = send(connSock, buff, ret, 0);
			if (ret == SOCKET_ERROR) {
				cout << "Error " << WSAGetLastError() << endl;
				break;
			}
		}
	}

	// Close socket
	closesocket(connSock);
	closesocket(listenSock);

	// Free winsock
	WSACleanup();

	return 0;
}