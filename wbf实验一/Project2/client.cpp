#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>
#include <WinSock2.h>
#include <windows.h>
#include <iostream>
#include <thread>
#include <string>
#pragma comment(lib, "ws2_32.lib")  //加载 ws2_32.dll
using namespace std;
#define BUF_SIZE 100

DWORD WINAPI Send(LPVOID sockpara) {
	SOCKET* sock = (SOCKET*)sockpara;
	char bufSend[BUF_SIZE] = { 0 };
	while (1) {
		//printf("Input a string: ");
		cin >> bufSend;
		int t = send(*sock, bufSend, strlen(bufSend), 0);
		if (strcmp(bufSend, "quit()") == 0)
		{
			SYSTEMTIME st = { 0 };
			GetLocalTime(&st);
			closesocket(*sock);
			cout << "您已于" << st.wYear << "年" << st.wMonth << "月" << st.wDay << "日" << st.wHour << "时" << st.wMinute << "分" << st.wSecond << "秒退出聊天室" << std::endl;
			return 0L;
		}
		if (t > 0) {
			SYSTEMTIME st = { 0 };
			GetLocalTime(&st);
			cout << "消息已于" << st.wYear << "年" << st.wMonth << "月" << st.wDay << "日" << st.wHour << "时" << st.wMinute << "分" << st.wSecond << "秒成功发送\n";
			cout << "-------------------------------------------------------------" << std::endl;
		}
		memset(bufSend, 0, BUF_SIZE);
	}
}


DWORD WINAPI Recv(LPVOID sock_) {
	char bufRecv[BUF_SIZE] = { 0 };
	SOCKET* sock = (SOCKET*)sock_;
	while (1) {
		int t = recv(*sock, bufRecv, BUF_SIZE, 0);
		if (strcmp(bufRecv, "quit()") == 0)
		{
			SYSTEMTIME st = { 0 };
			GetLocalTime(&st);
			closesocket(*sock);
			cout << "对方已于" <<st.wYear<<"年" <<st.wMonth<<"月" << st.wDay << "日" << st.wHour << "时" << st.wMinute << "分" << st.wSecond << "秒下线退出聊天室" << std::endl;
			return 0L;
		}
		if (t > 0) {
			SYSTEMTIME st = { 0 };
			GetLocalTime(&st);
			cout << st.wYear << "年" << st.wMonth << "月" << st.wDay << "日" << st.wHour << "时" << st.wMinute << "分" << st.wSecond << "秒收到消息:";
			printf(" %s\n", bufRecv);
			cout << "-------------------------------------------------------------" << std::endl;
		}
		memset(bufRecv, 0, BUF_SIZE);
	}
}


int main() {
	//初始化DLL
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0)
	{
		cout << "Call WSAStartup succseefully!" << std::endl;
	}
	else {
		cout << "Call WSAStartup unsuccseeful!" << std::endl;
		return 0;
	}
	sockaddr_in sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));  //每个字节都用0填充
	sockAddr.sin_family = PF_INET;
	sockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	sockAddr.sin_port = htons(1234);
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(sock, (SOCKADDR*)&sockAddr, sizeof(SOCKADDR)) == 0)
	{
		cout << "成功进入聊天室" << std::endl;
	}
	else {
		cout << "聊天室未上线" << std::endl;
		return 0;
	}
	//std::cout << WSAGetLastError() << std::endl;
	//std::cout << sock << std::endl;
	HANDLE hThread[2];
	hThread[0] = CreateThread(NULL, 0, Recv, (LPVOID)&sock, 0, NULL);
	hThread[1] = CreateThread(NULL, 0, Send, (LPVOID)&sock, 0, NULL);
	WaitForMultipleObjects(2, hThread, TRUE, INFINITE);
	CloseHandle(hThread[0]);
	CloseHandle(hThread[1]);
	closesocket(sock);
	WSACleanup();
	return 0;
}
