#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#include<winsock2.h>//winsock2��ͷ�ļ�
#include<iostream>
using  namespace std;

//����������dll��lib
#pragma comment(lib, "ws2_32.lib")

int  main()
{

	//����winsock2�Ļ���
	WSADATA  wd;
	if (WSAStartup(MAKEWORD(2, 2), &wd) != 0)
	{
		cout << "WSAStartup  error��" << GetLastError() << endl;
		return 0;
	}

	//1.������ʽ�׽���
	SOCKET  s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
	{
		cout << "socket  error��" << GetLastError() << endl;
		return 0;
	}

	//2.���ӷ�����
	sockaddr_in   addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8000);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	int len = sizeof(sockaddr_in);
	if (connect(s, (SOCKADDR*)&addr, len) == SOCKET_ERROR)
	{
		cout << "connect  error��" << GetLastError() << endl;
		return 0;
	}

	//3���շ���˵���Ϣ
	char buf[100] = { 0 };
	recv(s, buf, 100, 0);
	cout << buf << endl;

	//3��ʱ������˷���Ϣ
	int  ret = 0;
	do
	{
		char buf[100] = { 0 };
		cout << "��������������:";
		cin >> buf;
		ret = send(s, buf, 100, 0);
		recv(s, buf, 100, 0);
		cout << buf << endl;
	} while (ret != SOCKET_ERROR && ret != 0);


	//4.�رռ����׽���
	closesocket(s);

	//����winsock2�Ļ���
	WSACleanup();



	return 0;
}
