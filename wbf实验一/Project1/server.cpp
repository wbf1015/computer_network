#include <stdio.h>
#include <winsock2.h>//���뽻����Ҫ��ͷ�ļ�
#include <iostream>
using namespace std;
#pragma comment (lib, "ws2_32.lib")  //���� ws2_32.dll


#define BUF_SIZE 100//��Ϣ���ͻ������Ĵ�СΪ100

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
			std::cout << "������" << st.wYear << "��" << st.wMonth << "��" << st.wDay << "��" << st.wHour << "ʱ" << st.wMinute << "��" << st.wSecond << "���˳�������" << std::endl;
			return 0L;
		}
		if (t > 0) {
			SYSTEMTIME st = { 0 };
			GetLocalTime(&st);
			std::cout << "��Ϣ����" << st.wYear << "��" << st.wMonth << "��" << st.wDay << "��" << st.wHour << "ʱ" << st.wMinute << "��" << st.wSecond << "��ɹ�����\n";
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
			cout << "�Է�����" << st.wYear << "��" << st.wMonth << "��" << st.wDay << "��" << st.wHour << "ʱ" << st.wMinute << "��" << st.wSecond << "�������˳�������" << std::endl;
			return 0L;
		}
		if (t > 0) {
			SYSTEMTIME st = { 0 };
			GetLocalTime(&st);
			cout << st.wYear << "��" << st.wMonth << "��" << st.wDay<<st.wYear<<"��" <<st.wMonth<<"��" << "��" << st.wHour << "ʱ" << st.wMinute << "��" << st.wSecond << "���յ���Ϣ:";
			printf(" %s\n", bufRecv);
			std::cout << "-------------------------------------------------------------" << std::endl;
		}
		memset(bufRecv, 0, BUF_SIZE);
	}
}




int main() {
	WSADATA wsaData;//�����洢startup�������ú󷵻ص�socket
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0)//ʹ���첽�׽������������һ������ָ��ʹ�õİ汾Ϊ2.2������ɹ�
	{
		cout << "Call WSAStartup succseefully!" << endl;
	}
	else {
		cout << "Call WSAStartup failed!" << endl;
		return 0;
	}

	//�����׽���
	SOCKET servSock = socket(AF_INET, SOCK_STREAM, 0);//ָ����ַ����ΪIPV-4����������Ϊ�����Ͳ��Զ�ѡ��ProtocolЭ��

	//���׽��֣���һ�����ص�ַ�󶨵�ָ����Socket
	sockaddr_in sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));  //ÿ���ֽڶ���0���
	sockAddr.sin_family = PF_INET;  //ʹ��IPv4��ַ
	sockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  //�����IP��ַ
	sockAddr.sin_port = htons(1234);  //�˿�
	bind(servSock, (SOCKADDR*)&sockAddr, sizeof(SOCKADDR));//ִ�а�

	//�������״̬
	if (listen(servSock, 20) == 0) {//�������ȴ����г���Ϊ20
		std::cout << "�ѽ������״̬" << std::endl;
	}
	else {
		std::cout << "����״̬����" << std::endl;
		return 0;
	}

	//���տͻ�������
	SOCKADDR clntAddr;
	int nSize = sizeof(SOCKADDR);
	SOCKET clntSock = accept(servSock, (SOCKADDR*)&clntAddr, &nSize);//����һ������������Ŷӵ���������
	if (clntSock > 0) {
		cout << "�ͻ�������" << std::endl;
	}
	//�������߳�
	HANDLE hThread[2];
	hThread[0] = CreateThread(NULL, 0, Recv, (LPVOID)&clntSock, 0, NULL);
	hThread[1] = CreateThread(NULL, 0, Send, (LPVOID)&clntSock, 0, NULL);
	WaitForMultipleObjects(2, hThread, TRUE, INFINITE);
	CloseHandle(hThread[0]);
	CloseHandle(hThread[1]);


	closesocket(clntSock);  //�ر��׽���

	//�ر��׽���
	closesocket(servSock);

	//��ֹ DLL ��ʹ��
	WSACleanup();

	return 0;
}
