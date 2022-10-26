#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>
#include <WinSock2.h>
#include <windows.h>
#include <iostream>
#include <thread>
#include <string>
#pragma comment(lib, "ws2_32.lib")  //���� ws2_32.dll
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
			cout << "������" << st.wYear << "��" << st.wMonth << "��" << st.wDay << "��" << st.wHour << "ʱ" << st.wMinute << "��" << st.wSecond << "���˳�������" << std::endl;
			return 0L;
		}
		if (t > 0) {
			SYSTEMTIME st = { 0 };
			GetLocalTime(&st);
			cout << "��Ϣ����" << st.wYear << "��" << st.wMonth << "��" << st.wDay << "��" << st.wHour << "ʱ" << st.wMinute << "��" << st.wSecond << "��ɹ�����\n";
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
			cout << "�Է�����" <<st.wYear<<"��" <<st.wMonth<<"��" << st.wDay << "��" << st.wHour << "ʱ" << st.wMinute << "��" << st.wSecond << "�������˳�������" << std::endl;
			return 0L;
		}
		if (t > 0) {
			SYSTEMTIME st = { 0 };
			GetLocalTime(&st);
			cout << st.wYear << "��" << st.wMonth << "��" << st.wDay << "��" << st.wHour << "ʱ" << st.wMinute << "��" << st.wSecond << "���յ���Ϣ:";
			printf(" %s\n", bufRecv);
			cout << "-------------------------------------------------------------" << std::endl;
		}
		memset(bufRecv, 0, BUF_SIZE);
	}
}


int main() {
	//��ʼ��DLL
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
	memset(&sockAddr, 0, sizeof(sockAddr));  //ÿ���ֽڶ���0���
	sockAddr.sin_family = PF_INET;
	sockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	sockAddr.sin_port = htons(1234);
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(sock, (SOCKADDR*)&sockAddr, sizeof(SOCKADDR)) == 0)
	{
		cout << "�ɹ�����������" << std::endl;
	}
	else {
		cout << "������δ����" << std::endl;
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
