#include <stdio.h>
#include <winsock2.h>//引入交互需要的头文件
#include <iostream>
using namespace std;
#pragma comment (lib, "ws2_32.lib")  //加载 ws2_32.dll


#define BUF_SIZE 100//消息发送缓冲区的大小为100

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
			std::cout << "您已于" << st.wYear << "年" << st.wMonth << "月" << st.wDay << "日" << st.wHour << "时" << st.wMinute << "分" << st.wSecond << "秒退出聊天室" << std::endl;
			return 0L;
		}
		if (t > 0) {
			SYSTEMTIME st = { 0 };
			GetLocalTime(&st);
			std::cout << "消息已于" << st.wYear << "年" << st.wMonth << "月" << st.wDay << "日" << st.wHour << "时" << st.wMinute << "分" << st.wSecond << "秒成功发送\n";
			std::cout << "-------------------------------------------------------------" << endl;
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
			cout << "对方已于" << st.wYear << "年" << st.wMonth << "月" << st.wDay << "日" << st.wHour << "时" << st.wMinute << "分" << st.wSecond << "秒下线退出聊天室" << std::endl;
			return 0L;
		}
		if (t > 0) {
			SYSTEMTIME st = { 0 };
			GetLocalTime(&st);
			cout << st.wYear << "年" << st.wMonth << "月" << st.wDay<<st.wYear<<"年" <<st.wMonth<<"月" << "日" << st.wHour << "时" << st.wMinute << "分" << st.wSecond << "秒收到消息:";
			printf(" %s\n", bufRecv);
			std::cout << "-------------------------------------------------------------" << std::endl;
		}
		memset(bufRecv, 0, BUF_SIZE);
	}
}




int main() {
	WSADATA wsaData;//用来存储startup函数调用后返回的socket
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0)//使用异步套接字启动命令，第一个参数指定使用的版本为2.2，如果成功
	{
		cout << "Call WSAStartup succseefully!" << endl;
	}
	else {
		cout << "Call WSAStartup failed!" << endl;
		return 0;
	}

	//创建套接字
	SOCKET servSock = socket(AF_INET, SOCK_STREAM, 0);//指定地址类型为IPV-4，服务类型为流类型并自动选择Protocol协议

	//绑定套接字，将一个本地地址绑定到指定的Socket
	sockaddr_in sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));  //每个字节都用0填充
	sockAddr.sin_family = PF_INET;  //使用IPv4地址
	sockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  //具体的IP地址
	sockAddr.sin_port = htons(1234);  //端口
	bind(servSock, (SOCKADDR*)&sockAddr, sizeof(SOCKADDR));//执行绑定

	//进入监听状态
	if (listen(servSock, 20) == 0) {//设置最大等待队列长度为20
		std::cout << "已进入监听状态" << std::endl;
	}
	else {
		std::cout << "监听状态出错" << std::endl;
		return 0;
	}

	//接收客户端请求
	SOCKADDR clntAddr;
	int nSize = sizeof(SOCKADDR);
	SOCKET clntSock = accept(servSock, (SOCKADDR*)&clntAddr, &nSize);//接受一个服务端正在排队的连接请求
	if (clntSock > 0) {
		cout << "客户端上线" << std::endl;
	}
	//开启多线程
	HANDLE hThread[2];
	hThread[0] = CreateThread(NULL, 0, Recv, (LPVOID)&clntSock, 0, NULL);
	hThread[1] = CreateThread(NULL, 0, Send, (LPVOID)&clntSock, 0, NULL);
	WaitForMultipleObjects(2, hThread, TRUE, INFINITE);
	CloseHandle(hThread[0]);
	CloseHandle(hThread[1]);


	closesocket(clntSock);  //关闭套接字

	//关闭套接字
	closesocket(servSock);

	//终止 DLL 的使用
	WSACleanup();

	return 0;
}
