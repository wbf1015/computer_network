#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#include<iostream>
#include<string>
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<thread>
#include<map>
using namespace std;
#pragma comment(lib,"ws2_32.lib")
char* ipaddress = new char[100];
int port;
int flag = 1;
char* name = new char[100];
DWORD WINAPI receive(LPVOID P);
DWORD WINAPI mysend(LPVOID P);
int initial();
int main() {
	initial();
	//----------��ʼ��WSADATA----------
	WSADATA w;
	SOCKET s;
	int ans;
	ans = WSAStartup(MAKEWORD(2, 2), &w);
	if (ans == 0) {
		cout << "----------�ɹ���ʼ��WSADATA----------" << endl;
	}
	else {
		cout << "----------��ʼ��WSADATAʧ��----------" << endl;
		cout << "----------���������Բο�: " << WSAGetLastError() << "---------" << endl;
		return 0;
	}
	//----------��ʼ��WSADATA----------
	//----------��ʼ��socket---------
	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET) {
		cout << "----------��ʼ��socket����---------" << endl;
		cout << "----------���������Բο�: " << WSAGetLastError()<<"---------" << endl;
		WSACleanup();
		return 0;
	}
	else {
		cout << "---------�ɹ���ʼ��socket----------" << endl;
	}
	//----------��ʼ��socket---------
	//----------�������ӳ���----------
	sockaddr_in sa;
	cout << "----------��������Ҫ���ӵ�������Ӧ��ip��ַ--------" << endl;
	cout << "****�����ʹ�ñ���ip����localhost����****" << endl;
	cin >> ipaddress;
	cout << "��������Ҫ���ӵ������Ķ˿ں�" << endl;
	cout << "****ʹ��Ĭ�϶˿ں�1234������888888****" << endl;
	cin >> port;
	if ((string)ipaddress == "localhost") { strcpy(ipaddress, "127.0.0.1"); }
	if (port <= 0 || port > 65536) port = 1234;
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr(ipaddress);
	sa.sin_port = htons(port);
	ans = connect(s, (SOCKADDR*)&sa, sizeof(SOCKADDR));
	if (ans == 0) {
		cout << "---------�ɹ����Ӷ�Ӧ������׼����ʼ����ɣ�---------" << endl;
	}
	else {
		cout << "---------��������ʧ�ܣ����Ժ�����---------" << endl;
		cout << "----------����ԭ����Բο�:" << WSAGetLastError() <<"---------" << endl;
		closesocket(s);
		WSACleanup();
		return 0;
	}
	//----------�������ӳ���----------
	recv(s, name, 100, 0);//��ȡ�û���
	cout << "SUCCSSFULLY!----------�ɹ����������ң������û�����----------" << name << endl;
	cout << "��Ҫ�����Լ��û���������changename yourname�Ϳ����������岻Ҫ���Ǽ��Ͽո�Ŷ" << endl;
	cout << "                  ˽�Ŀ���ʹ��sendto xxx xxxxxxxxxxxx����ʽ~" << endl;
	cout << "                      ����quit�����˳���������" << endl;
	HANDLE t[2];
	t[0] = CreateThread(NULL, 0, receive, (LPVOID)&s, 0, NULL);
	t[1] = CreateThread(NULL, 0, mysend, (LPVOID)&s, 0, NULL);
	WaitForMultipleObjects(2, t, TRUE, INFINITE);
	CloseHandle(t[1]); CloseHandle(t[0]);
	closesocket(s);
	WSACleanup();
}
void outTime() {
	SYSTEMTIME t;
	GetLocalTime(&t);
	cout << t.wYear << "��" << t.wMonth << "��" << t.wDay << "��" << t.wHour << "ʱ" << t.wMinute << "��" << t.wSecond << "��";
}

DWORD WINAPI receive(LPVOID p) {
	int rflag;
	SOCKET* s = (SOCKET*)p;
	char* receivebuffer = new char[100];
	while (true) {
		rflag = recv(*s, receivebuffer, 100, 0);
		if (flag && rflag > 0) {
			cout << "****------------------------------------------------------------****" << endl;
			cout << "��";
			outTime();
			cout << "�յ���Ϣ: ";
			cout << receivebuffer;
			cout << "  ˵��ʲô�ɣ�" << endl;
			cout << "****------------------------------------------------------------****" << endl;
		}
		else {
			closesocket(*s);
			return 0;
		}
	}
}

DWORD WINAPI mysend(LPVOID p) {
	int sflag;
	SOCKET* s = (SOCKET*)p;
	char* sendbuffer = new char[100];
	while (true) {
		//cout << "˵��ʲô��~" << endl;
		cin.getline(sendbuffer, 100);
		if (string(sendbuffer) == "quit") {
			sflag = send(*s, sendbuffer, 100, 0);
			flag = 0;
			closesocket(*s);
			cout << endl << "----------�����˳�����,�ڴ��´μ���--------" << endl;
			return 1;
		}
		sflag = send(*s, sendbuffer, 100, 0);
		if (sflag == SOCKET_ERROR) {
			cout << "----------����ʧ��----------" << endl;
			cout << "----------����ԭ����Բο���" << WSAGetLastError()<<"-----------" << endl;
			closesocket(*s);
			WSACleanup();
			return 0;
		}
		else {
			
			outTime();
			cout << "��Ϣ�ɹ���������";
			cout << "��˵��ʲô��!" << endl;
			cout << "****------------------------------------------------------------****" << endl;
		}
	}
}
int initial() {
	// Declare and initialize variables
	WSADATA wsaData;
	struct hostent* remoteHost = NULL;
	char szHostName[128] = {};
	int nResult = -1;

	// Initialize Winsock
	//����һ��ϵͳ��ʼ��
	nResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nResult != 0)
	{
		printf("WSAStartup failed: %d\n", nResult);
		return 1;
	}
	//gethostname����wind socket
	nResult = gethostname(szHostName, sizeof(szHostName));
	if (nResult == 0)
	{
		printf("gethostname success: %s\n", szHostName);
	}
	else
	{
		printf("gethostname fail(errcode %d)...\n", ::GetLastError());
	}
	//��Դ����
	WSACleanup();
	return 0;
}