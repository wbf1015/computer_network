#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#include <winsock2.h> // winsock2的头文件
#include <iostream>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

// stdcall的线程处理函数
DWORD WINAPI ThreadFun(LPVOID lpThreadParameter);

int main()
{
	WSADATA wd;
	if (WSAStartup(MAKEWORD(2, 2), &wd) != 0)
	{
		cout << "WSAStartup Error:" << WSAGetLastError() << endl;
		return 0;
	}

	// 1. 创建流式套接字
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
	{
		cout << "socket error:" << WSAGetLastError() << endl;
		return 0;
	}

	// 2. 绑定端口和ip
	sockaddr_in addr;
	memset(&addr, 0, sizeof(sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8000);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	int len = sizeof(sockaddr_in);
	if (bind(s, (SOCKADDR*)&addr, len) == SOCKET_ERROR)
	{
		cout << "bind Error:" << WSAGetLastError() << endl;
		return 0;
	}

	// 3. 监听
	listen(s, 5);

	// 主线程循环接收客户端的连接
	while (true)
	{
		sockaddr_in addrClient;
		len = sizeof(sockaddr_in);
		// 4.接受成功返回与client通讯的Socket
		SOCKET c = accept(s, (SOCKADDR*)&addrClient, &len);
		if (c != INVALID_SOCKET)
		{
			// 创建线程，并且传入与client通讯的套接字
			HANDLE hThread = CreateThread(NULL, 0, ThreadFun, (LPVOID)c, 0, NULL);
			CloseHandle(hThread); // 关闭对线程的引用
		}

	}

	// 6.关闭监听套接字
	closesocket(s);

	// 清理winsock2的环境
	WSACleanup();

	return 0;
}

DWORD WINAPI ThreadFun(LPVOID lpThreadParameter)
{
	// 5.与客户端通讯，发送或者接受数据
	SOCKET c = (SOCKET)lpThreadParameter;

	cout << "欢迎" << c << "进入聊天室！" << endl;

	// 发送数据
	char buf[100] = { 0 };
	sprintf(buf, "欢迎 %d 进入聊天室！", c);
	send(c, buf, 100, 0);

	// 循环接收客户端数据
	int ret = 0;
	do
	{
		char buf2[100] = { 0 };
		ret = recv(c, buf2, 100, 0);

		cout << c << " 说：" << buf2 << endl;

	} while (ret != SOCKET_ERROR && ret != 0);

	cout << c << "离开了聊天室！";

	return 0;
}
