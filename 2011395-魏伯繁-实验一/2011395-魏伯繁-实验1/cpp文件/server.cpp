#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#include<iostream>
#include<string>
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<thread>
#include<map>
#include<vector>
using namespace std;
#pragma comment(lib,"ws2_32.lib")
char* ipaddress = new char[100];
char message[100];
map<string, string>allname;
int port;
DWORD WINAPI process(LPVOID p);
int initial();
int main() {
	initial();
	cout << "-----------���ڳ�ʼ�������ң����Ժ�...-----------" << endl;
	//----------��ʼ��WSADATA--------------
	WSADATA w;
	int ans;
	ans = WSAStartup(MAKEWORD(2, 2), &w);//��ʼ��socket dll
	if (ans == 0) {
		cout << "----------�ɹ���ʼ��WSADATA----------" << endl;
	}
	else {
		cout << "----------��ʼ��WSADATAʧ��----------" << endl;
		cout << "----------����ɲο�: " << WSAGetLastError()<<"-----------" << endl;
		return 0;
	}
	//----------��ʼ��WSADATA--------------
	//----------����һ��socket-------------
	SOCKET s;
	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//��ʼ�� IPV-4���²�Э��ѡ��������������Ҫ��ѡ��TCPЭ��
	if (s == INVALID_SOCKET) {
		cout << "----------socket����ʧ��----------" << endl;
		cout << "----------����ɲο�: " << WSAGetLastError() << "-----------" << endl;
		WSACleanup();//�ͷ�dll��Դ
		return 0;
	}
	else {
		cout << "-----------socket�����ɹ�-----------" << endl;
	}
	//���׽��֣���һ��ָ����ַ�󶨵�ָ����Socket
	sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = PF_INET;
	cout << "----------�������ṩ��ip����------------" << endl;
	cout << "****�����ʹ�ñ���ip����localhost����****" << endl;
	cin >> ipaddress;
	cout << "----------��������ʹ�õĶ˿ں�----------" << endl;
	cout << "****ʹ��Ĭ�϶˿ں�1234������888888****" << endl;
	cin >> port;
	if ((string)ipaddress == "localhost") { strcpy(ipaddress, "127.0.0.1"); }
	if (port <= 0 || port > 65536) port = 1234;
	sa.sin_addr.s_addr = inet_addr(ipaddress);  //�鿴Դ�룬���Կ���������ǰ����Ƕ�S_un.S_addr���и�ֵ
	sa.sin_port = htons(port);  //�˿�
	cout << ipaddress << endl;
	cout << port << endl;

	ans = bind(s, (SOCKADDR*)&sa, sizeof(SOCKADDR));//ִ�а�
	if (ans == 0) {
		cout << "**----------ip��ַ�󶨳ɹ�----------**" << endl;
	}
	else {
		closesocket(s);//�ͷ�socket��Դ
		WSACleanup();//�ͷ�dll��Դ
		cout << WSAGetLastError();
		cout << "-----------ip��ַ��ʧ��-----------" << endl;
		cout << "----------����ɲο�: " << WSAGetLastError() << "-----------" << endl;
		return 0;
	}
	//----------����һ��socket-------------
	//--------��ʼ�������״̬----------
	//�������״̬
	if (listen(s, 10) == 0) {//������Ϊ10
		cout << "----------��ʼ�����������ҳɹ�����������10��----------" << endl;
		cout << "----------�ɹ��������״̬----------" << endl;
	}
	else {
		cout << "----------����״̬����-----------" << endl;
		cout << "----------����ɲο�: " << WSAGetLastError() << "-----------" << endl;
		closesocket(s);//�ͷ�socket��Դ
		WSACleanup();//�ͷ�dll��Դ
		return 0;
	}
	cout << "SUCCESSFULLY!----------һ��׼�����������ڵȴ��ͻ�������----------SUCCESSFULLY!" << endl;
	//--------��ʼ�������״̬----------
	//-------����ɹ����ܵĿͻ��ˣ�Ϊ���Ƿ�����Դ---------
	while (true) {
		int size = sizeof(sockaddr_in);
		sockaddr_in clientaddr;
		SOCKET cs;
		cs = accept(s, (SOCKADDR*)&clientaddr, &size);
		if (cs == INVALID_SOCKET) {
			cout << "----------���ӿͻ���ʧ��----------" << endl;
			closesocket(s);//�ͷ�socket��Դ
			WSACleanup();//�ͷ�dll��Դ
			cout << "----------����ɲο�: " << WSAGetLastError() << "-----------" << endl;
			return 0;
		}
		else {
			//�߳����ԡ��̶߳�ջ��С���߳�ִ�к����������̲߳����������̲߳��������߳�ID��
			//LPVOID��һ��û�����͵�ָ�룬Ҳ����˵����Խ��������͵�ָ�븳ֵ��LPVOID���͵ı�����һ����Ϊ�������ݣ���Ȼ����ʹ�õ�ʱ����ת������
			HANDLE cthread = CreateThread(NULL, 0, process, (LPVOID)cs, 0, NULL);
			CloseHandle(cthread);
		}
	}
	//-------����ɹ����ܵĿͻ��ˣ�Ϊ���Ƿ�����Դ---------
	closesocket(s);//�ͷ�socket��Դ
	WSACleanup();//�ͷ�dll��Դ
	cout << "----------�����������ѽ������ڴ��ʹ���´λ���~----------" << endl;
}
bool judge(char* c) {
	if (c[0] == 'c' && c[1] == 'h' && c[2] == 'a' && c[3] == 'n' && c[4] == 'g' && c[5] == 'e' && c[6] == 'n' && c[7] == 'a' && c[8] == 'm' && c[9] == 'e') {
		return true;
	}return false;
}

bool judgesendto(char*c) {
	if (c[0] == 's' && c[1] == 'e' && c[2] == 'n' && c[3] == 'd' && c[4] == 't' && c[5] == 'o') {
		return true;
	}return false;
}
string getNewName(char*c) {
	string s = "";
	for (int i = 11; i <= 99; i++) {
		if (c[i] == '\0'||c[i]==' ' || c[i] == '/t') { return s; }
		else {
			s += c[i];
		}
	}
}
string toSend(char* c) {
	string s = "";
	for (int i = 7; i <= 99; i++) {
		if (c[i] == '\0' || c[i] == ' ' || c[i] == '/t') { return s; }
		else {
			s += c[i];
		}
	}
}
char* getMessage(char*c) {
	memset(message, 0, 100);
	int j = 0;
	int count = 0;
	for (int i = 0; i <= 99; i++) {
		if (count >= 2) {
			message[j++] = c[i];
		}
		if (c[i] == '\0' || c[i] == '/t') {
			return message;
		}
		else {
			if (c[i] == ' ') { count++; }
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
void outTime() {
	SYSTEMTIME t;
	GetLocalTime(&t);
	cout  << t.wYear << "��" << t.wMonth << "��" << t.wDay << "��" << t.wHour << "ʱ" << t.wMinute << "��" << t.wSecond << "��";
}

map<SOCKET, int>mymap;

DWORD WINAPI process(LPVOID p) {
	SOCKET cs = (SOCKET)p;
	if (mymap.find(cs) == mymap.end()) {
		mymap.insert(pair<SOCKET, int>(cs, 1));
	}
	else {
		mymap[cs] = 1;
	}
	char* name = new char[100];
	strcpy(name, to_string(cs).data());
	send(cs, (const char*)name, 100, 0);
	cout << "****------------------------------------------------------------****" << endl;
	outTime();
	cout << "----------�µ��û�" << to_string(cs).data() << "�ɹ��������죡��----------" << endl;

	int rflag;
	int sflag;
	int flag = 1;
	char* sendbuffer = new char[100];
	char* receivebuffer = new char[100];
	//��Ҫserver��Ҳ��ӡһ��
	do {
		rflag = recv(cs, receivebuffer, 100, 0);
		if (receivebuffer[0]=='q'&& receivebuffer[1] == 'u'&& receivebuffer[2] == 'i'&& receivebuffer[3] == 't'&&receivebuffer[4]=='\0') {
			mymap[cs] = 0;
			if (allname.find(to_string(cs).data()) == allname.end()) {
				cout << "----------�û�" << to_string(cs).data() << "�˳���Ⱥ��-----------" << endl;
				break;
			}else {
				cout << "----------�û�" << allname[to_string(cs).data()] << "�˳���Ⱥ��----------" << endl;
				break;
			}
		}
		if (judge(receivebuffer)) {
			cout << "****------------------------------------------------------------****" << endl;
			string newName = getNewName(receivebuffer);
			allname[to_string(cs).data()] = newName;
			cout << "----------�û�" << to_string(cs).data() << "���ǳƸ�Ϊ" << newName<<"------------" << endl;
			continue;
		}
		if (judgesendto(receivebuffer)) {
			bool stflag = false;
			string s = toSend(receivebuffer);
			for (auto it : mymap) {
				if (allname[to_string(it.first).data()] == s) {
					strcpy(sendbuffer, "�û�");
					if (allname.find(to_string(cs).data()) == allname.end()) {
						strcat(sendbuffer, to_string(cs).data());
					}
					else {
						strcat(sendbuffer, allname[to_string(cs).data()].c_str());
					}
					strcat(sendbuffer, " ����˵��");
					//const char* m = getMessage(sendbuffer);
					cout << message << endl;
					strcat(sendbuffer, (const char *)receivebuffer);
					cout << "****------------------------------------------------------------****" << endl;
					cout << "��";
					outTime();
					if (allname.find(to_string(cs).data()) == allname.end()) {
						cout << "�û�" << cs << "˵��" << receivebuffer << endl;
					}
					else {
						cout << "�û�" << allname[to_string(cs).data()] << "˵��" << receivebuffer << endl;
					}
					//cout << "----------������¼ֻ��" << s << "���Կ���----------" << endl;
					send(it.first, sendbuffer, 100, 0);
					stflag = true;
					break;
				}
				if (to_string(it.first).data() == s) {
						strcpy(sendbuffer, "�û�");
						if (allname.find(to_string(cs).data()) == allname.end()) {
							strcat(sendbuffer, to_string(cs).data());
						}
						else {
							strcat(sendbuffer, allname[to_string(cs).data()].c_str());
						}
						strcat(sendbuffer, " ����˵��");
						//const char* m = getMessage(sendbuffer);
						strcat(sendbuffer, (const char *)receivebuffer);
						//strcat(sendbuffer, (const char*)receivebuffer);
						cout << "****------------------------------------------------------------****" << endl;
						cout << "��";
						outTime();
						if (allname.find(to_string(cs).data()) == allname.end()) {
							cout << "�û�" << cs << "˵��" << receivebuffer << endl;
						}
						else {
							cout << "�û�" << allname[to_string(cs).data()] << "˵��" << receivebuffer << endl;
						}
						//cout << "----------������¼ֻ��" << to_string(it.first).data() << "���Կ���----------" << endl;
						send(it.first, sendbuffer, 100, 0);
						stflag = true;
						break;
				}
			}
			if (stflag == false) {
				strcpy(sendbuffer, "����ʧ��");
				send(cs, sendbuffer, 100, 0);
				cout<<"��һ��˽�ķ���ʧ��"<<endl;
			}
			continue;
		}
		if (rflag > 0) {
			
			strcpy(sendbuffer, "�û�");
			if (allname.find(to_string(cs).data()) == allname.end()) {
				strcat(sendbuffer, to_string(cs).data());
			}
			else {
				strcat(sendbuffer, allname[to_string(cs).data()].c_str());
			}
			strcat(sendbuffer," ˵��");
			strcat(sendbuffer, (const char*)receivebuffer);
			cout << "****------------------------------------------------------------****" << endl;
			cout << "��";
			outTime();
			if (allname.find(to_string(cs).data()) == allname.end()) {
				cout << "�û�" << cs << "˵��" << receivebuffer << endl;
			}
			else {
				cout << "�û�" << allname[to_string(cs).data()] << "˵��" << receivebuffer << endl;
			}
			vector<SOCKET> temp;
			for (auto it : mymap) {
				if (it.first != cs && it.second == 1) {
					auto ans = send(it.first, sendbuffer, 100, 0);
					if (ans == SOCKET_ERROR) {
						cout << "----------���û�" << it.first << "����ʧ�ܣ�" << "�����룺 " << WSAGetLastError()<<"----------" << endl;
						cout << "---------�Ѿ����û�" << it.first << "�Ƴ�����---------" << endl;
						temp.push_back(cs);
					}
				}
			}
			for (int i = 0; i < temp.size(); i++) {
				mymap.erase(temp[i]);
			}
		}
		else {
			flag = 0;
		}
	} while (rflag != SOCKET_ERROR && flag != 0);
	//mymap[cs] = 0;

	return 0;
}