#include<iostream>
#include<WinSock2.h>
#include<time.h>
#include<fstream>
#include<iostream>
using namespace std;
#pragma comment(lib,"ws2_32.lib")

int main() {
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);

    //声明服务器需要的套接字以及套接字需要绑定的地址
    SOCKADDR_IN server_addr;
    SOCKET server;
    //声明路由器需要的套接字以及套接字需要绑定的地址
    SOCKADDR_IN router_addr;
    SOCKET router;
    //声明客户端的地址
    SOCKADDR_IN client_addr;

    server_addr.sin_family = AF_INET;//使用IPV4
    server_addr.sin_port = htons(8888);//server的端口号
    server_addr.sin_addr.s_addr = htonl(2130706433);//主机127.0.0.1

    router_addr.sin_family = AF_INET;//使用IPV4
    router_addr.sin_port = htons(8888);//router的端口号
    router_addr.sin_addr.s_addr = htonl(2130706433);//主机127.0.0.1

    //指定一个客户端
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(8887);
    client_addr.sin_addr.s_addr = htonl(2130706433);//主机127.0.0.1


    server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    bind(server, (SOCKADDR*)&server_addr, sizeof(server_addr));

    router = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    bind(router, (SOCKADDR*)&router_addr, sizeof(router_addr));

    char* recvbuffer = new char[1000];
    char* sendbuffer = new char[1000];

    int clen = sizeof(client_addr);
    int rlen = sizeof(router_addr);
    while (true) {
        int length = recvfrom(server, recvbuffer, 1000, 0, (sockaddr*)&client_addr, &clen);
        cout << length << "   " << recvbuffer << endl;
        sendbuffer[0] = 'A', sendbuffer[1] = 'C', sendbuffer[2] = 'K';
        sendto(server, sendbuffer, 1000, 0, (sockaddr*)&router_addr, rlen);
    }
}