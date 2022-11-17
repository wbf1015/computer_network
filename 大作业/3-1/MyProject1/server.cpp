#include<iostream>
#include<WinSock2.h>
#include<time.h>
#include<fstream>
#include<iostream>
#include<windows.h>
using namespace std;
#pragma comment(lib,"ws2_32.lib")
//初始化dll
WSADATA wsadata;
//声明服务器需要的套接字以及套接字需要绑定的地址
SOCKADDR_IN server_addr;
SOCKET server;
//声明路由器需要的套接字以及套接字需要绑定的地址
SOCKADDR_IN router_addr;;
//声明客户端的地址
SOCKADDR_IN client_addr;
char* sbuffer = new char[1000];
char* rbuffer = new char[1000];
char* message = new char[10000000];
unsigned long long int messagepointer;
//服务器地址的性质
int clen = sizeof(client_addr);
int rlen = sizeof(router_addr);
//常数设置
u_long blockmode = 0;
u_long unblockmode = 1;
const unsigned char MAX_DATA_LENGTH = 0xff;
const u_short SOURCEIP = 0x7f01;
const u_short DESIP = 0x7f01;
const u_short SOURCEPORT = 8888;//源端口是8888
const u_short DESPORT = 8887;//客户端端口号是8887
const unsigned char SYN = 0x1;//OVER=0,FIN=0,ACK=0,SYN=1
const unsigned char ACK = 0x2;//OVER=0,FIN=0,ACK=1,SYN=0
const unsigned char SYN_ACK = 0x3;//OVER=0,FIN=0,ACK=1,SYN=1
const unsigned char OVER = 0x8;//OVER=1,FIN=0,ACK=0,SYN=0
const unsigned char OVER_ACK = 0xA;//OVER=1,FIN=0,ACK=1,SYN=0
const unsigned char FIN = 0x10;//FIN=1,OVER=0,FIN=0,ACK=0,SYN=0
const unsigned char FIN_ACK = 0x12;//FIN=1,OVER=0,FIN=0,ACK=1,SYN=0
const unsigned char FINAL_CHECK=0x20;//FC=1.FIN=0,OVER=0,FIN=0,ACK=0,SYN=0
const double MAX_TIME = 0.2*CLOCKS_PER_SEC;
//数据头
struct Header {
    u_short checksum; //16位校验和
    u_short seq; //8位序列号,因为是停等，所以只有最低位实际上只有0和1两种状态
    u_short ack; //8位ack号，因为是停等，所以只有最低位实际上只有0和1两种状态
    u_short flag;//8位状态位 倒数第一位SYN,倒数第二位ACK，倒数第三位FIN，倒数第四位是结束位
    u_short length;//8位长度位
    u_short source_ip; //16位ip地址
    u_short des_ip; //16位ip地址
    u_short source_port; //16位源端口号
    u_short des_port; //16位目的端口号
    Header() {//构造函数
        checksum = 0;
        source_ip = SOURCEIP;
        des_ip = DESIP;
        source_port = SOURCEPORT;
        des_port = DESPORT;
        seq = 0;
        ack = 0;
        flag = 0;
        length = 0;
    }
};

void printcharstar(char* s, int l) {
    for (int i = 0; i < l; i++) {
        cout << (int)s[i];
    }
    cout << endl;
}

void printheader(Header& h) {
    cout << "checksum=" << h.checksum << endl;
    cout << "se1=" << h.seq << endl;
    cout << "ack=" << h.ack << endl;
    cout << "flag=" << h.flag << endl;
    cout << "length=" << h.length << endl;
    cout << "sourceip=" << h.source_ip << endl;
    cout << "desip=" << h.des_ip << endl;
    cout << "source_port=" << h.source_port << endl;
    cout << "des_port=" << h.des_port << endl;
}

//sizeof返回内存字节数 ushort是16位2字节，所以需要把size向上取整
u_short calcksum(u_short* mes, int size) {
    int count = (size + 1) / 2;
    u_short* buf = (u_short*)malloc(size + 1);
    memset(buf, 0, size + 1);
    memcpy(buf, mes, size);
    u_long sum = 0;
    buf += 1;
    count -= 1;
    while (count--) {
        sum += *buf++;
        if (sum & 0xffff0000) {
            sum &= 0xffff;
            sum++;
        }
    }
    return ~(sum & 0xffff);
}

u_short vericksum(u_short* mes, int size) {
    int count = (size + 1) / 2;
    u_short* buf = (u_short*)malloc(size + 1);
    memset(buf, 0, size + 1);
    memcpy(buf, mes, size);
    u_long sum = 0;
    //buf += 2;
    //count -= 2;
    while (count--) {
        sum += *buf++;
        if (sum & 0xffff0000) {
            sum &= 0xffff;
            sum++;
        }
    }
    return ~(sum & 0xffff);
}

void test();
void initialNeed();
int  tryToConnect();
int  receivemessage();
int  endreceive();
int loadmessage();
int tryToDisconnect();

int main() {
    initialNeed();
    tryToConnect();
    receivemessage();
    loadmessage();
    tryToDisconnect();
}

void initialNeed() {
    WSAStartup(MAKEWORD(2, 2), &wsadata);

    //指定服务端的性质
    server_addr.sin_family = AF_INET;//使用IPV4
    server_addr.sin_port = htons(8888);//server的端口号
    server_addr.sin_addr.s_addr = htonl(2130706433);//主机127.0.0.1

    //指定路由器的性质
    router_addr.sin_family = AF_INET;//使用IPV4
    router_addr.sin_port = htons(8886);//router的端口号
    router_addr.sin_addr.s_addr = htonl(2130706433);//主机127.0.0.1

    //指定一个客户端
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(8887);
    client_addr.sin_addr.s_addr = htonl(2130706433);//主机127.0.0.1

    //绑定服务端
    server = socket(AF_INET, SOCK_DGRAM, 0);
    bind(server, (SOCKADDR*)&server_addr, sizeof(server_addr));

    //计算地址性质
    int clen = sizeof(client_addr);
    int rlen = sizeof(router_addr);
}

//检测你的初始化是否正确，并不提供功能型帮助
void test() {
    while (true) {
        int length = recvfrom(server, rbuffer, 1000, 0, (sockaddr*)&router_addr, &clen);
        cout << length << "   " << rbuffer << endl;
        sbuffer[0] = 'A'; sbuffer[1] = 'C'; sbuffer[2] = 'K'; sbuffer[3] = '\0';
        sendto(server, sbuffer, 1000, 0, (sockaddr*)&router_addr, rlen);
    }
}

int tryToConnect() {
    Header header;//声明一个数据头
    char* recvshbuffer = new char[sizeof(header)];//创建一个和数据头一样大的接收缓冲区
    char* sendshbuffer = new char[sizeof(header)];//创建一个和数据头一样大的发送缓冲区
    //等待第一次握手
    while (true) {
        //收到了第一次握手的申请
        //我觉得他写的不对 返回值不太对
        if (recvfrom(server, recvshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) == -1) {
            cout << "....第一次握手信息接受失败...." << endl;
            return -1;
        }
        memcpy(&header, recvshbuffer, sizeof(header));//给数据头赋值
        //如果是单纯的请求建立连接请求，并且校验和相加取反之后就是0
        if (header.flag == SYN || vericksum((u_short*)(&header), sizeof(header)) == 0) {
            cout << "成功接受客户端请求，第一次握手建立成功...." << endl;
            break;
        }
        else {
            cout << vericksum((u_short*)(&header), sizeof(header)) << endl;
            cout << "第一次握手数据包损坏，正在等待重传..." << endl;
        }
    }
SECONDSHAKE:
    //准备发送第二次握手的信息
    header.source_port = SOURCEPORT;
    header.des_port = DESPORT;
    header.flag = SYN_ACK;
    header.source_port = SOURCEPORT;
    header.des_port = DESPORT;
    header.ack = (header.seq + 1) % 2;
    header.seq = 0;
    header.length = 0;
    header.checksum = calcksum((u_short*)(&header), sizeof(header));
    cout << vericksum((u_short*)&header, sizeof(header)) << endl;
    memcpy(sendshbuffer, &header, sizeof(header));
    if (sendto(server, sendshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
        cout << "....第二次握手消息发送失败...." << endl;
        return -1;
    }
    cout << "第二次握手消息发送成功...." << endl;

    clock_t start = clock();

    //第二次握手消息的超时重传 重传时直接重传sendshbuffer里的内容就可以
    //我觉得他写的不对 返回值不太对
    while (recvfrom(server, recvshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {
        if (clock() - start > MAX_TIME) {
            if (sendto(server, sendshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
                cout << "....第二次握手消息重新发送失败...." << endl;
                return -1;
            }
            cout << "第二次握手消息重新发送成功...." << endl;
            start = clock();
        }
    }

    memcpy(&header, recvshbuffer, sizeof(header));
    if (header.flag == ACK && vericksum((u_short*)(&header), sizeof(header)) == 0) {
        cout << "成功接收第三次握手消息！可以开始接收数据..." << endl;
        header.source_port = SOURCEPORT;
        header.des_port = DESPORT;
        header.flag = ACK;
        header.source_port = SOURCEPORT;
        header.des_port = DESPORT;
        header.ack = (header.seq + 1) % 2;
        header.seq = 0;
        header.length = 0;
        header.checksum = calcksum((u_short*)(&header), sizeof(header));
        memcpy(sendshbuffer, &header, sizeof(header));
        sendto(server, sendshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen);
        cout << "确认信息传输成功...." << endl;
    }
    else {
        cout << "不是期待的数据包，正在重传并等待客户端等待重传" << endl;
        goto SECONDSHAKE;
    }
    cout << "正在等待接收数据...." << endl;
    return 1;
}


int loadmessage() {
    string filename = "2.jpg";
    ofstream fout(filename.c_str(), ofstream::binary);
    for (int i = 0; i < messagepointer; i++)
    {
        fout << message[i];
    }
    fout.close();
    cout << "文件已成功下载到本地" << endl;
    return 0;
}

int receivemessage() {
    Header header;
    char* recvbuffer = new char[sizeof(header) + MAX_DATA_LENGTH];
    char* sendbuffer = new char[sizeof(header)];

WAITSEQ0:
    //接受seq=0的数据
    while (true) {
        //此时可以设置为阻塞模式
        ioctlsocket(server, FIONBIO, &unblockmode);
        while (recvfrom(server, recvbuffer, sizeof(header) + MAX_DATA_LENGTH, 0, (sockaddr*)&router_addr, &rlen) <= 0) {
            //cout << "接受失败...请检查原因" << endl;
            //cout << WSAGetLastError() << endl;
        }
        memcpy(&header, recvbuffer, sizeof(header));
        //cout << header.flag << endl;
        if (header.flag == OVER) {
            //传输结束，等待添加....
            if (vericksum((u_short*)&header, sizeof(header)) == 0) { if (endreceive()) { return 1; }return 0; }
            else { cout << "数据包出错，正在等待重传" << endl; goto WAITSEQ0; }
        }
        cout << header.seq << " " << vericksum((u_short*)recvbuffer, sizeof(header) + MAX_DATA_LENGTH) << endl;
        //printheader(header);
        //printcharstar(recvbuffer, sizeof(header) + MAX_DATA_LENGTH);
        if (header.seq == 0 && vericksum((u_short*)recvbuffer, sizeof(header)+MAX_DATA_LENGTH) == 0) {
            cout << "成功接收seq=0数据包" << endl;
            memcpy(message + messagepointer, recvbuffer + sizeof(header), header.length);
            messagepointer += header.length;
            break;
        }
        else {
            cout << "数据包错误，正在等待对方重新发送" << endl;
        }
    }
    header.ack = 1;
    header.seq = 0;
    header.checksum = calcksum((u_short*)&header, sizeof(header));
    memcpy(sendbuffer, &header, sizeof(header));
SENDACK1:
    if (sendto(server, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
        cout << "ack1发送失败...." << endl;
        return -1;
    }
    clock_t start = clock();
RECVSEQ1:
    //设置为非阻塞模式
    ioctlsocket(server, FIONBIO, &unblockmode);
    while (recvfrom(server, recvbuffer, sizeof(header) + MAX_DATA_LENGTH, 0, (sockaddr*)&router_addr, &rlen) <= 0) {
        if (clock() - start > MAX_TIME) {
            if (sendto(server, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
                cout << "ack0发送失败...." << endl;
                return -1;
            }
            start = clock();
            cout << "ack1消息反馈超时....已重发...." << endl;
            goto SENDACK1;
        }
    }
    memcpy(&header, recvbuffer, sizeof(header));
    if (header.flag == OVER) {
        //传输结束，等待添加....
        if (vericksum((u_short*)&header, sizeof(header) == 0)) { if (endreceive()) { return 1; }return 0; }
        else { cout << "数据包出错，正在等待重传" << endl; goto WAITSEQ0; }
    }
    cout << header.seq << " " << vericksum((u_short*)recvbuffer, sizeof(header) + MAX_DATA_LENGTH) << endl;

    if (header.seq == 1 && vericksum((u_short*)recvbuffer, sizeof(header)+MAX_DATA_LENGTH)==0) {
        cout << "成功接受seq=1的数据包，正在解析..." << endl;
        memcpy(message + messagepointer, recvbuffer + sizeof(header), header.length);
        messagepointer += header.length;
    }
    else {
        cout << "数据包损坏，正在等待重新传输" << endl;
        goto RECVSEQ1;
    }
    header.ack = 0;
    header.seq = 1;
    header.checksum = calcksum((u_short*)&header, sizeof(header));
    memcpy(sendbuffer, &header, sizeof(header));
    sendto(server, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen);
    goto WAITSEQ0;
}

int endreceive() {
    Header header;
    char* sendbuffer = new char[sizeof(header)];
    header.flag = OVER_ACK;
    header.checksum = calcksum((u_short*)&header, sizeof(header));
    memcpy(sendbuffer, &header,sizeof(header));
    if (sendto(server, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) >= 0) return 1;
    return 0;
}

int tryToDisconnect() {
    Header header;
    char* sendbuffer = new char[sizeof(header)];
    char* recvbuffer = new char[sizeof(header)];
RECVWAVE1:
    while(recvfrom(server,recvbuffer,sizeof(header),0,(sockaddr*)&router_addr,&rlen)<=0){}
    memcpy(&header, recvbuffer, sizeof(header));
    if (header.flag == FIN && vericksum((u_short*)&header, sizeof(header)) == 0) {
        cout << "收到第一次挥手信息" << endl;
    }
    else {
        cout << "第一次挥手消息接收失败" << endl;
        goto RECVWAVE1;
    }
 SEND2:
    header.seq = 0;
    header.flag = ACK;
    header.checksum = calcksum((u_short*)&header, sizeof(header));
    memcpy(sendbuffer, &header, sizeof(header));
    if (sendto(server, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
        cout << "第二次挥手消息发送失败..." << endl;
        return -1;
    }
    Sleep(80);
SEND3:
    header.seq = 1;
    header.flag = FIN_ACK;
    header.checksum = calcksum((u_short*)&header, sizeof(header));
    memcpy(sendbuffer, &header, sizeof(header));
    if (sendto(server, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
        cout << "第三次挥手消息发送失败..." << endl;
        return -1;
    }
    clock_t start = clock();
    ioctlsocket(server, FIONBIO, &unblockmode);
    while (recvfrom(server, recvbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {
        if (clock() - start > MAX_TIME) {
            cout << "第四次挥手消息接收延迟...准备重发二三次挥手" << endl;
            ioctlsocket(server, FIONBIO, &blockmode);
            goto SEND2;
        }
    }
SEND5:
    memcpy(&header, recvbuffer, sizeof(header));
    if (header.flag == ACK && vericksum((u_short*)&header, sizeof(header)) == 0) {
        header.seq = 0;
        header.flag = FINAL_CHECK;
        header.checksum = calcksum((u_short*)&header, sizeof(header));
        memcpy(sendbuffer, &header, sizeof(header));
        sendto(server, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen);
        cout << "成功发送确认报文" << endl;
    }
    start = clock();
    while (recvfrom(server, recvbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {
        if (clock() - start > 10 * MAX_TIME) {
            cout << "四次挥手结束，已经断开连接" << endl;
            return 1;
        }
    }
    goto SEND5;
}