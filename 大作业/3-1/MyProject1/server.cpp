#include<iostream>
#include<WinSock2.h>
#include<time.h>
#include<fstream>
#include<iostream>
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
const unsigned char MAX_DATA_LENGTH = 0xff;
const u_short SOURCEIP = 0x7f01;
const u_short DESIP = 0x7f01;
const u_short SOURCEPORT = 8888;//源端口是8888
const u_short DESPORT = 8887;//路由器端口号是8887
const unsigned char SYN = 0x1;//OVER=0,FIN=0,ACK=0,SYN=1
const unsigned char ACK = 0x2;//OVER=0,FIN=0,ACK=1,SYN=0
const unsigned char SYN_ACK = 0x3;//OVER=0,FIN=0,ACK=1,SYN=1
const unsigned char OVER = 0x8;//OVER=1,FIN=0,ACK=0,SYN=0
const unsigned char OVER_ACK = 0xA;//OVER=1,FIN=0,ACK=1,SYN=0
const double MAX_TIME = CLOCKS_PER_SEC;
//数据头
struct Header {
    unsigned char seq; //8位序列号,因为是停等，所以只有最低位实际上只有0和1两种状态
    unsigned char ack; //8位ack号，因为是停等，所以只有最低位实际上只有0和1两种状态
    unsigned char empty;//8位空位
    unsigned char flag;//8位状态位 倒数第一位SYN,倒数第二位ACK，倒数第三位FIN，倒数第四位是结束位
    u_short checksum; //16位校验和
    unsigned char empty2;//8位空位
    unsigned char length;//8位长度位
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
        empty = 0;
        length = 0;
        empty2 = 0;
    }
};

//sizeof返回内存字节数 ushort是16位2字节，所以需要把size向上取整
u_short calcksum(u_short* mes, int size) {
    int count = (size + 1) / 2;
    u_short* buf = (u_short*)malloc(size + 1);
    memset(buf, 0, size + 1);
    memcpy(buf, mes, size);
    u_long sum = 0;
    buf += 3;
    count -= 3;
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
    buf += 2;
    count -= 2;
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
int  receive();
int  endsend();
int loadmessage();

int main() {
    initialNeed();
    if (tryToConnect() <= 0) {
        cout << "握手失败，请检查连接后再试" << endl;
        return -1;
    }
    cout << "握手成功！等待客户端传输！...";
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
    server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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
        cout << "正在等待接收数据...." << endl;
    }
    else {
        cout << "不是期待的数据包，正在重传并等待客户端等待重传" << endl;
        goto SECONDSHAKE;
    }
    return 1;
}

int receive() {
    Header header;
    char* recvbuffer = new char[sizeof(header) + MAX_DATA_LENGTH];
    char* sendbuffer = new char[sizeof(header)];

 WAITSEQ0:
    //接受seq=0的数据
    while (true) {
        while (recvfrom(server, recvbuffer, sizeof(header) + MAX_DATA_LENGTH, 0, (sockaddr*)&router_addr, &rlen) == -1) {
            cout << "接受失败...请检查原因" << endl;
        }
        memcpy(&header, recvbuffer, sizeof(header));
        if (header.flag == OVER ) {
            //传输结束，等待添加....
            if (vericksum((u_short*)&header, sizeof(header) == 0)) { if (endsend()) { return 1; }return 0; }
            else { cout << "数据包出错，正在等待重传" << endl; goto WAITSEQ0; }
        }
        if (header.seq == 0 && vericksum((u_short*)&header, sizeof(header)) == 0) {
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
SENDACK0:
    if (sendto(server, sendbuffer, sizeof(header),0, (sockaddr*)&router_addr, rlen) == -1) {
        cout << "ack0发送失败...." << endl;
        return -1;
    }
    clock_t start = clock();
RECVSEQ1:
    while (recvfrom(server, recvbuffer, sizeof(header) + MAX_DATA_LENGTH, 0, (sockaddr*)&router_addr, &rlen) <= 0) {
        if (clock() - start > MAX_TIME) {
            if (sendto(server, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
                cout << "ack0发送失败...." << endl;
                return -1;
            }
            start = clock();
            cout << "ack0消息反馈超时....已重发...." << endl;
            goto SENDACK0;
        }
    }
    memcpy(&header, recvbuffer, sizeof(header));
    if (header.flag == OVER) {
        //传输结束，等待添加....
        if (vericksum((u_short*)&header, sizeof(header) == 0)) { if (endsend()) { return 1; }return 0; }
        else { cout << "数据包出错，正在等待重传" << endl; goto WAITSEQ0; }
    }
    if (header.seq == 1 && vericksum((u_short*)&header, sizeof(header) == 0)) {
        cout << "成功接受seq=1的数据包，正在解析..." << endl;
        memcpy(message + messagepointer, recvbuffer + sizeof(header), header.length);
        messagepointer += header.length;
    }
    else {
        cout << "数据报损坏，正在等待重新传输" << endl;
        goto RECVSEQ1;
    }
    header.ack = 0;
    header.seq = 1;
    header.checksum = calcksum((u_short*)&header, sizeof(header));
    memcpy(sendbuffer, &header, sizeof(header));
    sendto(server, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen);
    goto WAITSEQ0;
}

int endsend() {
    Header header;
    char* sendbuffer = new char[sizeof(header)];
    header.flag = OVER_ACK;
    header.checksum = calcksum((u_short*)&header, sizeof(header));
    if (sendto(server, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) >= 0) return 1;
    return 0;
}

int loadmessage() {
    string filename = "1.jpg";
    ofstream fout(filename.c_str(), ofstream::binary);
    for (int i = 0; i < messagepointer; i++)
    {
        fout << message[i];
    }
    fout.close();
    cout << "文件已成功下载到本地" << endl;
}