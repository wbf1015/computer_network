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
	//----------初始化WSADATA----------
	WSADATA w;
	SOCKET s;
	int ans;
	ans = WSAStartup(MAKEWORD(2, 2), &w);
	if (ans == 0) {
		cout << "----------成功初始化WSADATA----------" << endl;
	}
	else {
		cout << "----------初始化WSADATA失败----------" << endl;
		cout << "----------具体错误可以参考: " << WSAGetLastError() << "---------" << endl;
		return 0;
	}
	//----------初始化WSADATA----------
	//----------初始化socket---------
	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET) {
		cout << "----------初始化socket错误---------" << endl;
		cout << "----------具体错误可以参考: " << WSAGetLastError()<<"---------" << endl;
		WSACleanup();
		return 0;
	}
	else {
		cout << "---------成功初始化socket----------" << endl;
	}
	//----------初始化socket---------
	//----------进行连接尝试----------
	sockaddr_in sa;
	cout << "----------请输入想要连接的主机对应的ip地址--------" << endl;
	cout << "****如果想使用本地ip输入localhost即可****" << endl;
	cin >> ipaddress;
	cout << "请输入想要连接的主机的端口号" << endl;
	cout << "****使用默认端口号1234则输入888888****" << endl;
	cin >> port;
	if ((string)ipaddress == "localhost") { strcpy(ipaddress, "127.0.0.1"); }
	if (port <= 0 || port > 65536) port = 1234;
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr(ipaddress);
	sa.sin_port = htons(port);
	ans = connect(s, (SOCKADDR*)&sa, sizeof(SOCKADDR));
	if (ans == 0) {
		cout << "---------成功连接对应主机！准备开始聊天吧！---------" << endl;
	}
	else {
		cout << "---------连接主机失败，请稍后再试---------" << endl;
		cout << "----------具体原因可以参看:" << WSAGetLastError() <<"---------" << endl;
		closesocket(s);
		WSACleanup();
		return 0;
	}
	//----------进行连接尝试----------
	recv(s, name, 100, 0);//获取用户名
	cout << "SUCCSSFULLY!----------成功进入聊天室，您的用户名是----------" << name << endl;
	cout << "想要更换自己用户名吗？输入changename yourname就可以啦！主义不要忘记加上空格哦" << endl;
	cout << "                  私聊可以使用sendto xxx xxxxxxxxxxxx的形式~" << endl;
	cout << "                      输入quit即可退出聊天室啦" << endl;
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
	cout << t.wYear << "年" << t.wMonth << "月" << t.wDay << "日" << t.wHour << "时" << t.wMinute << "分" << t.wSecond << "秒";
}

DWORD WINAPI receive(LPVOID p) {
	int rflag;
	SOCKET* s = (SOCKET*)p;
	char* receivebuffer = new char[100];
	while (true) {
		rflag = recv(*s, receivebuffer, 100, 0);
		if (flag && rflag > 0) {
			cout << "****------------------------------------------------------------****" << endl;
			cout << "在";
			outTime();
			cout << "收到消息: ";
			cout << receivebuffer;
			cout << "  说点什么吧！" << endl;
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
		//cout << "说点什么吧~" << endl;
		cin.getline(sendbuffer, 100);
		if (string(sendbuffer) == "quit") {
			sflag = send(*s, sendbuffer, 100, 0);
			flag = 0;
			closesocket(*s);
			cout << endl << "----------即将退出聊天,期待下次见面--------" << endl;
			return 1;
		}
		sflag = send(*s, sendbuffer, 100, 0);
		if (sflag == SOCKET_ERROR) {
			cout << "----------发送失败----------" << endl;
			cout << "----------具体原因可以参看：" << WSAGetLastError()<<"-----------" << endl;
			closesocket(*s);
			WSACleanup();
			return 0;
		}
		else {
			
			outTime();
			cout << "消息成功发送啦！";
			cout << "再说点什么吧!" << endl;
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
	//进行一次系统初始化
	nResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nResult != 0)
	{
		printf("WSAStartup failed: %d\n", nResult);
		return 1;
	}
	//gethostname依赖wind socket
	nResult = gethostname(szHostName, sizeof(szHostName));
	if (nResult == 0)
	{
		printf("gethostname success: %s\n", szHostName);
	}
	else
	{
		printf("gethostname fail(errcode %d)...\n", ::GetLastError());
	}
	//资源回收
	WSACleanup();
	return 0;
}