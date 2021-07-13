#include <iostream>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <process.h>
#include <string> 
#include <sstream>


#pragma comment(lib, "Ws2_32.lib")

#define BUFF_SIZE 2048

using namespace std;

void commandAction(string cmd, SOCKET sock) {
	cout << "Command: " << cmd << endl;
	
	if (cmd == "\help") {

	}
	else if (cmd == "\online") {

	}
}

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

	if (listen(listenSock, SOMAXCONN)) {
		cout << "Error!" << endl;
		return 0;
	}
	cout << "Server started." << endl;

	//Assign initial value for the fd_set
	fd_set initfds;
	FD_ZERO(&initfds);
	FD_SET(listenSock, &initfds);

	while (true) {
		fd_set readfds = initfds;

		int socketCount = select(0, &readfds, nullptr, nullptr, nullptr);

		// Lặp qua các socket đang sẵn sàng
		for (int i = 0; i < socketCount; i++)
		{
			SOCKET sock = readfds.fd_array[i];

			// Kiểm tra socket chưa đc thiết lập kết nối
			if (sock == listenSock)
			{
				// Accept connection mới
				SOCKET client = accept(listenSock, nullptr, nullptr);

				// Thêm connection mới vào tập FD_SET
				FD_SET(client, &initfds);

				// Send a welcome message to the connected client
				string welcomeMsg = "SERVER: Chao mung ban den voi phong chat!";
				send(client, welcomeMsg.c_str(), welcomeMsg.size() + 1, 0);
			}
			else
			{
				char buf[4096];
				// Khởi tạo giá trị 0 cho buff
				ZeroMemory(buf, 4096);

				// Nhận thông điệp
				int ret = recv(sock, buf, 4096, 0);

				// Nếu có lỗi
				if (ret <= 0)
				{
					// Đóng socket
					closesocket(sock);
					FD_CLR(sock, &initfds);
				}
				else
				{
					cout << "Recieve from " << sock << ": " << buf << endl;

					// Kiểm tra xem có phải command hay không? Command bắt đầu bằng '\' 
					if (buf[0] == '\\')
					{
						string cmd = string(buf, ret);
						commandAction(cmd, sock);

						continue;
					}

					// Gửi tin nhắn đến các client khác

					for (int i = 0; i < initfds.fd_count; i++)
					{
						SOCKET outSock = initfds.fd_array[i];
						if (outSock == listenSock)
						{
							continue;
						}

						ostringstream ss;

						if (outSock != sock)
						{
							ss << "NGUOI DUNG #" << sock << ":" << buf << "\r\n";
						}
						else
						{
							ss << "TOI:" << buf << "\r\n";
						}

						string strOut = ss.str();
						send(outSock, strOut.c_str(), strOut.size() + 1, 0);
					}
				}
			}
		}
	}

	// Gửi thông báo đóng kết nối đến các client
	string msg = "SERVER: Server da dong.\r\n";

	// Đóng listenSock để không nhận thêm kết nối mới
	FD_CLR(listenSock, &initfds);
	closesocket(listenSock);

	// Đóng các kết nối còn lại
	while (initfds.fd_count > 0)
	{
		SOCKET sock = initfds.fd_array[0];
		send(sock, msg.c_str(), msg.size() + 1, 0);
		FD_CLR(sock, &initfds);
		closesocket(sock);
	}

	// Free winsock
	WSACleanup();

	return 0;
}