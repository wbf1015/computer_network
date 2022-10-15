#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#include <winsock2.h> // winsock2��ͷ�ļ�
#include <iostream>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

// stdcall���̴߳�����
DWORD WINAPI ThreadFun(LPVOID lpThreadParameter);

int main()
{
	WSADATA wd;
	if (WSAStartup(MAKEWORD(2, 2), &wd) != 0)
	{
		cout << "WSAStartup Error:" << WSAGetLastError() << endl;
		return 0;
	}

	// 1. ������ʽ�׽���
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
	{
		cout << "socket error:" << WSAGetLastError() << endl;
		return 0;
	}

	// 2. �󶨶˿ں�ip
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

	// 3. ����
	listen(s, 5);

	// ���߳�ѭ�����տͻ��˵�����
	while (true)
	{
		sockaddr_in addrClient;
		len = sizeof(sockaddr_in);
		// 4.���ܳɹ�������clientͨѶ��Socket
		SOCKET c = accept(s, (SOCKADDR*)&addrClient, &len);
		if (c != INVALID_SOCKET)
		{
			// �����̣߳����Ҵ�����clientͨѶ���׽���
			HANDLE hThread = CreateThread(NULL, 0, ThreadFun, (LPVOID)c, 0, NULL);
			CloseHandle(hThread); // �رն��̵߳�����
		}

	}

	// 6.�رռ����׽���
	closesocket(s);

	// ����winsock2�Ļ���
	WSACleanup();

	return 0;
}

DWORD WINAPI ThreadFun(LPVOID lpThreadParameter)
{
	// 5.��ͻ���ͨѶ�����ͻ��߽�������
	SOCKET c = (SOCKET)lpThreadParameter;

	cout << "��ӭ" << c << "���������ң�" << endl;

	// ��������
	char buf[100] = { 0 };
	sprintf(buf, "��ӭ %d ���������ң�", c);
	send(c, buf, 100, 0);

	// ѭ�����տͻ�������
	int ret = 0;
	do
	{
		char buf2[100] = { 0 };
		ret = recv(c, buf2, 100, 0);

		cout << c << " ˵��" << buf2 << endl;

	} while (ret != SOCKET_ERROR && ret != 0);

	cout << c << "�뿪�������ң�";

	return 0;
}
