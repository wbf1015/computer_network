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
	cout << "-----------正在初始化聊天室，请稍后...-----------" << endl;
	//----------初始化WSADATA--------------
	WSADATA w;
	int ans;
	ans = WSAStartup(MAKEWORD(2, 2), &w);//初始化socket dll
	if (ans == 0) {
		cout << "----------成功初始化WSADATA----------" << endl;
	}
	else {
		cout << "----------初始化WSADATA失败----------" << endl;
		cout << "----------详情可参看: " << WSAGetLastError()<<"-----------" << endl;
		return 0;
	}
	//----------初始化WSADATA--------------
	//----------启动一个socket-------------
	SOCKET s;
	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//初始化 IPV-4，下层协议选择数据流，根据要求选择TCP协议
	if (s == INVALID_SOCKET) {
		cout << "----------socket创建失败----------" << endl;
		cout << "----------详情可参看: " << WSAGetLastError() << "-----------" << endl;
		WSACleanup();//释放dll资源
		return 0;
	}
	else {
		cout << "-----------socket创建成功-----------" << endl;
	}
	//绑定套接字，将一个指定地址绑定到指定的Socket
	sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = PF_INET;
	cout << "----------请输入提供的ip号码------------" << endl;
	cout << "****如果想使用本地ip输入localhost即可****" << endl;
	cin >> ipaddress;
	cout << "----------请输入欲使用的端口号----------" << endl;
	cout << "****使用默认端口号1234则输入888888****" << endl;
	cin >> port;
	if ((string)ipaddress == "localhost") { strcpy(ipaddress, "127.0.0.1"); }
	if (port <= 0 || port > 65536) port = 1234;
	sa.sin_addr.s_addr = inet_addr(ipaddress);  //查看源码，可以看到这个就是帮我们对S_un.S_addr进行赋值
	sa.sin_port = htons(port);  //端口
	cout << ipaddress << endl;
	cout << port << endl;

	ans = bind(s, (SOCKADDR*)&sa, sizeof(SOCKADDR));//执行绑定
	if (ans == 0) {
		cout << "**----------ip地址绑定成功----------**" << endl;
	}
	else {
		closesocket(s);//释放socket资源
		WSACleanup();//释放dll资源
		cout << WSAGetLastError();
		cout << "-----------ip地址绑定失败-----------" << endl;
		cout << "----------详情可参看: " << WSAGetLastError() << "-----------" << endl;
		return 0;
	}
	//----------启动一个socket-------------
	//--------开始进入监听状态----------
	//进入监听状态
	if (listen(s, 10) == 0) {//最大队列为10
		cout << "----------初始化多人聊天室成功，最多可容纳10人----------" << endl;
		cout << "----------成功进入监听状态----------" << endl;
	}
	else {
		cout << "----------监听状态出错-----------" << endl;
		cout << "----------详情可参看: " << WSAGetLastError() << "-----------" << endl;
		closesocket(s);//释放socket资源
		WSACleanup();//释放dll资源
		return 0;
	}
	cout << "SUCCESSFULLY!----------一切准备就绪，正在等待客户端连接----------SUCCESSFULLY!" << endl;
	//--------开始进入监听状态----------
	//-------处理成功接受的客户端，为他们分配资源---------
	while (true) {
		int size = sizeof(sockaddr_in);
		sockaddr_in clientaddr;
		SOCKET cs;
		cs = accept(s, (SOCKADDR*)&clientaddr, &size);
		if (cs == INVALID_SOCKET) {
			cout << "----------连接客户端失败----------" << endl;
			closesocket(s);//释放socket资源
			WSACleanup();//释放dll资源
			cout << "----------详情可参看: " << WSAGetLastError() << "-----------" << endl;
			return 0;
		}
		else {
			//线程属性、线程堆栈大小、线程执行函数、传入线程参数、创建线程参数、新线程ID好
			//LPVOID是一个没有类型的指针，也就是说你可以将任意类型的指针赋值给LPVOID类型的变量（一般作为参数传递），然后在使用的时候在转换回来
			HANDLE cthread = CreateThread(NULL, 0, process, (LPVOID)cs, 0, NULL);
			CloseHandle(cthread);
		}
	}
	//-------处理成功接受的客户端，为他们分配资源---------
	closesocket(s);//释放socket资源
	WSACleanup();//释放dll资源
	cout << "----------本次聊天室已结束，期待和大家下次会面~----------" << endl;
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
void outTime() {
	SYSTEMTIME t;
	GetLocalTime(&t);
	cout  << t.wYear << "年" << t.wMonth << "月" << t.wDay << "日" << t.wHour << "时" << t.wMinute << "分" << t.wSecond << "秒";
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
	cout << "----------新的用户" << to_string(cs).data() << "成功加入聊天！！----------" << endl;

	int rflag;
	int sflag;
	int flag = 1;
	char* sendbuffer = new char[100];
	char* receivebuffer = new char[100];
	//需要server处也打印一下
	do {
		rflag = recv(cs, receivebuffer, 100, 0);
		if (receivebuffer[0]=='q'&& receivebuffer[1] == 'u'&& receivebuffer[2] == 'i'&& receivebuffer[3] == 't'&&receivebuffer[4]=='\0') {
			mymap[cs] = 0;
			if (allname.find(to_string(cs).data()) == allname.end()) {
				cout << "----------用户" << to_string(cs).data() << "退出了群聊-----------" << endl;
				break;
			}else {
				cout << "----------用户" << allname[to_string(cs).data()] << "退出了群聊----------" << endl;
				break;
			}
		}
		if (judge(receivebuffer)) {
			cout << "****------------------------------------------------------------****" << endl;
			string newName = getNewName(receivebuffer);
			allname[to_string(cs).data()] = newName;
			cout << "----------用户" << to_string(cs).data() << "将昵称改为" << newName<<"------------" << endl;
			continue;
		}
		if (judgesendto(receivebuffer)) {
			bool stflag = false;
			string s = toSend(receivebuffer);
			for (auto it : mymap) {
				if (allname[to_string(it.first).data()] == s) {
					strcpy(sendbuffer, "用户");
					if (allname.find(to_string(cs).data()) == allname.end()) {
						strcat(sendbuffer, to_string(cs).data());
					}
					else {
						strcat(sendbuffer, allname[to_string(cs).data()].c_str());
					}
					strcat(sendbuffer, " 对你说：");
					//const char* m = getMessage(sendbuffer);
					cout << message << endl;
					strcat(sendbuffer, (const char *)receivebuffer);
					cout << "****------------------------------------------------------------****" << endl;
					cout << "在";
					outTime();
					if (allname.find(to_string(cs).data()) == allname.end()) {
						cout << "用户" << cs << "说：" << receivebuffer << endl;
					}
					else {
						cout << "用户" << allname[to_string(cs).data()] << "说：" << receivebuffer << endl;
					}
					//cout << "----------这条记录只有" << s << "可以看到----------" << endl;
					send(it.first, sendbuffer, 100, 0);
					stflag = true;
					break;
				}
				if (to_string(it.first).data() == s) {
						strcpy(sendbuffer, "用户");
						if (allname.find(to_string(cs).data()) == allname.end()) {
							strcat(sendbuffer, to_string(cs).data());
						}
						else {
							strcat(sendbuffer, allname[to_string(cs).data()].c_str());
						}
						strcat(sendbuffer, " 对你说：");
						//const char* m = getMessage(sendbuffer);
						strcat(sendbuffer, (const char *)receivebuffer);
						//strcat(sendbuffer, (const char*)receivebuffer);
						cout << "****------------------------------------------------------------****" << endl;
						cout << "在";
						outTime();
						if (allname.find(to_string(cs).data()) == allname.end()) {
							cout << "用户" << cs << "说：" << receivebuffer << endl;
						}
						else {
							cout << "用户" << allname[to_string(cs).data()] << "说：" << receivebuffer << endl;
						}
						//cout << "----------这条记录只有" << to_string(it.first).data() << "可以看到----------" << endl;
						send(it.first, sendbuffer, 100, 0);
						stflag = true;
						break;
				}
			}
			if (stflag == false) {
				strcpy(sendbuffer, "发送失败");
				send(cs, sendbuffer, 100, 0);
				cout<<"有一条私聊发送失败"<<endl;
			}
			continue;
		}
		if (rflag > 0) {
			
			strcpy(sendbuffer, "用户");
			if (allname.find(to_string(cs).data()) == allname.end()) {
				strcat(sendbuffer, to_string(cs).data());
			}
			else {
				strcat(sendbuffer, allname[to_string(cs).data()].c_str());
			}
			strcat(sendbuffer," 说：");
			strcat(sendbuffer, (const char*)receivebuffer);
			cout << "****------------------------------------------------------------****" << endl;
			cout << "在";
			outTime();
			if (allname.find(to_string(cs).data()) == allname.end()) {
				cout << "用户" << cs << "说：" << receivebuffer << endl;
			}
			else {
				cout << "用户" << allname[to_string(cs).data()] << "说：" << receivebuffer << endl;
			}
			vector<SOCKET> temp;
			for (auto it : mymap) {
				if (it.first != cs && it.second == 1) {
					auto ans = send(it.first, sendbuffer, 100, 0);
					if (ans == SOCKET_ERROR) {
						cout << "----------向用户" << it.first << "发送失败，" << "错误码： " << WSAGetLastError()<<"----------" << endl;
						cout << "---------已经将用户" << it.first << "移除队列---------" << endl;
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