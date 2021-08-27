#include <iostream>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <process.h>
#include <string> 
#include <sstream>
#include<vector>
#include<map>
#include <fstream>

#pragma comment(lib, "Ws2_32.lib")

#define BUFF_SIZE 2048

using namespace std;

struct User {
	string username = "";
	string password = "";
	bool isLogin = false;
};

map<string, User> users;
map<int, User> socketUsers;

void showUserOnline(SOCKET sock) {
	int count = 0;
	string msg = "";
		for (auto it = socketUsers.begin(); it != socketUsers.end(); ++it) {
			if (it->second.isLogin) {
				count++;
				msg = msg + it->second.username + "\n";
			}
		}
		msg = "Danh sach " + to_string(count) + " thanh vien online\n" + msg;
		send(sock, msg.c_str(), msg.size() + 1, 0);
}

void showHelp(SOCKET sock) {
	string msg = "SERVER: Danh sach cac lenh: \n\t=Join: Vao phong\n\t=Exit: Thoat\n\t=Online: Xem DS nguoi trong phong\n\t=Help: Hien thi DS lenh\n";
	send(sock, msg.c_str(), msg.size() + 1, 0);
}

void clientExit(SOCKET sock, fd_set initfds) {
	string msg = "\\out";
	send(sock, msg.c_str(), msg.size() + 1, 0);
	cout << socketUsers[sock].username << " da dang xuat socket " << sock << "\n" << endl;
	socketUsers[sock].isLogin = false;
	socketUsers.erase(sock);
	cout << "Current socket connected: " << initfds.fd_count << endl;
}

void commandAction(string cmd, SOCKET sock, fd_set initfds) {
	cout << "Command: " << cmd << endl;
	
	if (cmd == "\\help") {
		showHelp(sock);
	}
	else if (cmd == "\\exit") {
		clientExit(sock, initfds);
	}
	else if (cmd == "\\online") {
		showUserOnline(sock);
	}
}

void loadUsers() {
	// Khai bao va mo file
	ifstream infile("user.txt");
	string username, password;
	while (infile >> username >> password)
	{
		User user = User();
		user.username = username;
		user.password = password;
		users[username] = user;
	}
}

int main(int argc, char* argv[]) {

	// Init port from command-line arguments
	int port;
	if (argc > 1) {
		port = atoi(argv[1]);
		cout << "Server will start at port " << port << "." << endl;
	}
	else {
		port = 5500;
		cout << "Server will start at default port 5500." << endl;
	}

	loadUsers();

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

				// Thêm id socket vào tập socketUsers
				socketUsers[client] = User();

				// Send a welcome message to the connected client
				string welcomeMsg = "SERVER: Chao mung ban den voi phong chat! Vui long dang nhap de bat dau.\nTai khoan:";
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
					// Kiem tra dang nhap
					User currentUser = socketUsers[sock];
					if (!currentUser.isLogin) {
						if (currentUser.username == "") {
							socketUsers[sock].username = buf;
							string msg = "Mat khau:";
							send(sock, msg.c_str(), msg.size() + 1, 0);
						}
						else {
							// check login
							string username = socketUsers[sock].username;
							string password = buf;

							if (users[username].password == password) {
								socketUsers[sock].isLogin = true;
								cout << currentUser.username << " da dang nhap" << endl;
								string msg = "Ban da dang nhap thanh cong! Bat dau chat.";
								send(sock, msg.c_str(), msg.size() + 1, 0);
							}
							else {
								socketUsers[sock].username = "";
								string msg = "Ban da nhap sai tai khoan hoac mat khau!\nTai khoan:";
								send(sock, msg.c_str(), msg.size() + 1, 0);
							}
						}
						continue;
					}

					cout << "Recieve from " << currentUser.username << ": " << buf << endl;

					// Kiểm tra xem có phải command hay không? Command bắt đầu bằng '\' 
					if (buf[0] == '\\')
					{
						string cmd = string(buf, ret);
						commandAction(cmd, sock, initfds);

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

						if (!socketUsers[outSock].isLogin) {
							continue;
						}

						ostringstream msg;

						if (outSock != sock && socketUsers[outSock].username != socketUsers[sock].username)
						{
							msg << currentUser.username << ": " << buf << "\r\n";
						}
						else
						{
							msg << "TOI: " << buf << "\r\n";
						}

						string strOut = msg.str();
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