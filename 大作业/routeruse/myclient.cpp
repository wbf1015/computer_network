#include<iostream>
#include<WinSock2.h>
#include<time.h>
#include<fstream>
#include<iostream>
using namespace std;
#pragma comment(lib,"ws2_32.lib")

int main(){
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2,2),&WSAData);

    //声明服务器需要的套接字以及套接字需要绑定的地址
    SOCKADDR_IN server_addr;
    SOCKET server;
    //声明路由器需要的套接字以及套接字需要绑定的地址
    SOCKADDR_IN router_addr;
    SOCKET router
    //声明客户端的地址
    SOCKADDR_in client_addr;
    SOCKET client;

    server_addr.sin_family = AF_INET;//使用IPV4
    server_addr.sin_port = htons(8888);//server的端口号
    server_addr.sin_addr.s_addr = htonl(2130706433);//主机127.0.0.1

    router_addr.sin_family = AF_INET;//使用IPV4
    router_addr.sin_port = htons(8888);//router的端口号
    router_addr.sin_addr.s_addr = htonl(2130706433);//主机127.0.0.1

    //指定一个客户端
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(8887);
    client_addr.sinport.s_addr = htonl(2130706433);

    client=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    bind(client,(sockaddr*)client_addr,sizeof(client_addr));

    char *recvbuffer=new char[1000];
    char *sendbuffer=new char[1000];

    int slen = sizeof(server_addr);
    int rlen = sizeof(router_addr);

    int i=0;
    while(1=1){
        sendbuffer=to_string(i);i++;
        sendto(client,sendbuffer,1000,0,(sockaddr*)router_addr,rlen);
        recvfrom(client,recvbuffer,1000,0,(sockaddr*)&server_addr,&slen);
        cout<<recvfrom<<endl;
    }
}