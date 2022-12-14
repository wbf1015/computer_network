#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#include<iostream>
#include<WinSock2.h>
#include<time.h>
#include<fstream>
#include<iostream>
#include<string>
using namespace std;
#pragma comment(lib,"ws2_32.lib")
//初始化dll
WSADATA wsadata;

//声明套接字需要绑定的地址
SOCKADDR_IN server_addr;
//声明路由器需要绑定的地址
SOCKADDR_IN router_addr;
//声明客户端的地址
SOCKADDR_IN client_addr;
SOCKET client;

char* message = new char[100000000];
char* sbuffer = new char[1000];
char* rbuffer = new char[1000];
unsigned long long int messagelength = 0;//最后一个要传的下标是多少
unsigned long long int messagepointer = 0;//下一个该传的位置

int slen = sizeof(server_addr);
int rlen = sizeof(router_addr);

//常数设置
u_long unlockmode = 1;
u_long lockmode = 0;
const unsigned char MAX_DATA_LENGTH = 0xff;
const u_short SOURCEIP = 0x7f01;
const u_short DESIP = 0x7f01;
const u_short SOURCEPORT = 8887;//客户端端口是8887
const u_short DESPORT = 8888;//服务端端口号是8888
const unsigned char SYN = 0x1;//FIN=0,ACK=0,SYN=1
const unsigned char ACK = 0x2;//FIN=0,ACK=1,SYN=0
const unsigned char SYN_ACK = 0x3;//FIN=0,ACK=1,SYN=1
const unsigned char OVER = 0x8;//OVER=1,FIN=0,ACK=0,SYN=0
const unsigned char OVER_ACK = 0xA;//OVER=1,FIN=0,ACK=1,SYN=0
const unsigned char FIN = 0x10;//FIN=1,OVER=0,FIN=0,ACK=0,SYN=0
const unsigned char FIN_ACK = 0x12;//FIN=1,OVER=0,FIN=0,ACK=1,SYN=0
const unsigned char FINAL_CHECK = 0x20;//FC=1.FIN=0,OVER=0,FIN=0,ACK=0,SYN=0
const int MAX_TIME = 0.2*CLOCKS_PER_SEC; //最大传输延迟时间
//数据头
struct Header {
    u_short checksum; //16位校验和
    u_short seq; //16位序列号,因为是停等，所以只有最低位实际上只有0和1两种状态
    u_short ack; //16位ack号，因为是停等，所以只有最低位实际上只有0和1两种状态
    u_short flag;//16位状态位 倒数第一位SYN,倒数第二位ACK，倒数第三位FIN
    u_short length;//16位长度位
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
//全局时钟设置
clock_t linkClock;
clock_t ALLSTART;
clock_t ALLEND;

void printcharstar(char* s,int l) {
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
    //buf += 0;
    //count -= 0;
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
int endsend();
int loadMessage();
int sendmessage();
int tryToDisconnect();
void printlog();

int main() {
    initialNeed();
    tryToConnect();
    loadMessage();
    sendmessage();
    tryToDisconnect();
    printlog();

    return 0;
}

void test() {
    int i = 48;
    while (true) {
        sbuffer[0] = (char)i; sbuffer[1] = '\0'; i++;
        if (i > 57) { i = 48; }
        sendto(client, sbuffer, 1000, 0, (sockaddr*)&router_addr, rlen);
        recvfrom(client, rbuffer, 1000, 0, (sockaddr*)&router_addr, &slen);
        cout << rbuffer << endl;
    }
}
void initialNeed() {
    WSAStartup(MAKEWORD(2, 2), &wsadata);

    server_addr.sin_family = AF_INET;//使用IPV4
    server_addr.sin_port = htons(8888);//server的端口号
    server_addr.sin_addr.s_addr = htonl(2130706433);//主机127.0.0.1

    router_addr.sin_family = AF_INET;//使用IPV4
    router_addr.sin_port = htons(8886);//router的端口号
    router_addr.sin_addr.s_addr = htonl(2130706433);//主机127.0.0.1

    //指定一个客户端
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(8887);
    client_addr.sin_addr.s_addr = htonl(2130706433);//主机127.0.0.1

    client = socket(AF_INET, SOCK_DGRAM, 0);
    bind(client, (SOCKADDR*)&client_addr, sizeof(client_addr));
    cout << "[PREPARE]初始化工作完成" << endl;
}

int tryToConnect() {
    linkClock = clock();
    Header header;
    char* recvshbuffer = new char[sizeof(header)];
    char* sendshbuffer = new char[sizeof(header)];

    //第一次握手请求
    header.source_port = SOURCEPORT;
    header.des_port = DESPORT;
    header.flag = SYN;
    header.seq = 0;
    header.ack = 0;
    header.length = 0;
    header.checksum = calcksum((u_short*)&header, sizeof(header));
    //cout << vericksum((u_short*)&header, sizeof(header)) << endl;
    u_short* test = (u_short*)&header;
    memcpy(sendshbuffer, &header, sizeof(header));
SEND1:
    if (sendto(client, sendshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
        cout << "[failed]第一次握手请求发送失败..." << endl;
        return -1;
    }
    cout << "[1]第一次握手消息发送成功...." << endl;
    //设置是否为非阻塞模式
    ioctlsocket(client, FIONBIO, &unlockmode);

FIRSTSHAKE:
    //设置计时器
    clock_t start = clock();

    //第一次握手重传
    while (recvfrom(client, recvshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {
        if (clock() - linkClock > 75 * CLOCKS_PER_SEC) {
            cout << "[failed]连接超时,服务器自动断开" << endl;
            return -1;
        }
        if (clock() - start > MAX_TIME) {
            if (sendto(client, sendshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
                cout << "[failed]第一次握手请求发送失败..." << endl;
                return -1;
            }
            start = clock();
            cout << "[1]第一次握手消息反馈超时....正在重新发送" << endl;
            goto SEND1;
        }
    }

    //检验第二次握手信息是否准确
    memcpy(&header, recvshbuffer, sizeof(header));
    if (header.flag == SYN_ACK && vericksum((u_short*)&header, sizeof(header)) == 0) {
        cout << "[2]正确接受第二次握手信息" << endl;
    }
    else {
        cout << "[1]不是期待的服务端数据包,即将重传第一次握手数据包...." << endl;
        if (clock() - linkClock > 75 * CLOCKS_PER_SEC) {
            cout << "[failed]连接超时,服务器自动断开" << endl;
            return -1;
        }
        goto SEND1;
    }


    //发送第三次握手信息
    header.source_port = SOURCEPORT;
    header.des_port = DESPORT;
    header.flag = ACK;
    header.seq = 1;
    header.ack = 1;
    header.checksum = calcksum((u_short*)&header, sizeof(header));
    //cout << vericksum((u_short*)&header, sizeof(header)) << endl;
    memcpy(sendshbuffer, &header, sizeof(header));
SEND3:
    if (sendto(client, sendshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
        cout << "[failed]第三次握手信息发送失败，请稍后再试...." << endl;
        return -1;
    }
    start = clock();
    while (recvfrom(client, recvshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {
        if (clock() - linkClock > 75 * CLOCKS_PER_SEC) {
            cout << "[failed]连接超时,服务器自动断开" << endl;
            return -1;
        }
        if (clock() - start >= 5 * MAX_TIME) {
            cout << "[failed]第三次握手信息反馈超时...正在重新发送" << endl;
            if (clock() - linkClock > 75 * CLOCKS_PER_SEC) {
                cout << "[failed]连接超时,服务器自动断开" << endl;
                return -1;
            }
            goto SEND3;
        }
    }
    memcpy(&header, recvshbuffer, sizeof(header));
    if (header.flag == ACK && vericksum((u_short*)&header, sizeof(header)) == 0) {
        cout << "[3]正确发送第三次握手消息" << endl;
        cout << "[EVERYTHING_DONE]与服务器建联成功，准备发送数据" << endl;
        return 1;
    }
    else {
        cout << "[ERROR]确认消息接受失败...重发第三次握手" << endl;
        goto SEND3;
    }
    
}

int loadMessage() {
    string filename ;
    cout << "[INPUT]请输入要传输的文件名" << endl;
    cin >> filename;
    ifstream fin(filename.c_str(), ifstream::binary);//以二进制方式打开文件
    unsigned long long int index = 0;
    unsigned char temp = fin.get();
    while (fin)
    {
        message[index++] = temp;
        temp = fin.get();
    }
    messagelength = index-1;
    fin.close();
    cout << "[FINISH]完成文件读入工作" << endl;
    return 0;
}

int sendmessage() {
    ALLSTART = clock();
    //设置是否为非阻塞模式
    ioctlsocket(client, FIONBIO, &unlockmode);

    Header header;
    char* recvbuffer = new char[sizeof(header)];
    char* sendbuffer = new char[sizeof(header) + MAX_DATA_LENGTH];

    while (true) {

        int ml;//本次数据传输长度
        if (messagepointer > messagelength) {//不需要再发了，都发完了
            if (endsend() == 1) { return 1; }
            return -1;
        }
        if (messagelength - messagepointer >= MAX_DATA_LENGTH) {//可以按照最大限度发送
            ml = MAX_DATA_LENGTH;
        }
        else {
            ml = messagelength - messagepointer + 1;//需要计算发送的长度
        }

        header.seq = 0;//这次发的是零号序列号
        header.length = ml;//实际数据长度
        memset(sendbuffer, 0, sizeof(header) + MAX_DATA_LENGTH);//sendbuffer全部置零
        memcpy(sendbuffer, &header, sizeof(header));//拷贝header内容
        memcpy(sendbuffer+sizeof(header), message + messagepointer, ml);//拷贝数据内容
        messagepointer += ml;//更新数据指针
        header.checksum = calcksum((u_short*)sendbuffer, sizeof(header)+MAX_DATA_LENGTH);//计算校验和
        memcpy(sendbuffer, &header, sizeof(header));//填充校验和
        //cout << vericksum((u_short*)sendbuffer, sizeof(header) + MAX_DATA_LENGTH) << endl;
        //cout << header.seq << endl;
    SEQ0SEND:
        //发送seq=0的数据包
        cout << "[0]准备发送" << "0" << "号数据包，该数据包大小为:" << ml<<" ";
        cout << "发送前检验：整体校验为" << vericksum((u_short*)sendbuffer, sizeof(header) + MAX_DATA_LENGTH) << endl;
        //printheader(header);
        //printcharstar(sendbuffer, sizeof(header) + MAX_DATA_LENGTH);
        if (sendto(client, sendbuffer, (sizeof(header) + MAX_DATA_LENGTH), 0, (sockaddr*)&router_addr, rlen) == -1) {
            cout << "[failed]seq0数据包发送失败....请检查原因" << endl;
            return -1;
        }
        clock_t start = clock();
    SEQ0RECV:
        //如果收到数据了就不发了，否则延时重传
        //设置了非阻塞，所以不会卡住
        while (recvfrom(client, recvbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {
            if (clock() - start > MAX_TIME) {
                //printcharstar(sendbuffer, sizeof(header) + MAX_DATA_LENGTH);
                if (sendto(client, sendbuffer, (sizeof(header)+MAX_DATA_LENGTH), 0, (sockaddr*)&router_addr, rlen) == -1) {
                    cout << "[failed]seq0数据包发送失败....请检查原因" << endl;
                    return -1;
                }
                start = clock();
                cout << "[ERROR]seq0数据包反馈超时....正在重传" << endl;
                //goto SEQ0SEND;
            }
        }
        //检查ack位是否正确，如果正确则准备发下一个数据包
        memcpy(&header, recvbuffer, sizeof(header));
        cout << "[GETACK]接受到的ack为" << header.ack << "接受到的校验和为" << vericksum((u_short*)&header, sizeof(header)) << endl;
        if (header.ack == 1 && vericksum((u_short*)&header, sizeof(header) == 0)) {
            cout << "[0CHECKED]seq0数据包成功接受服务端ACK，准备发出下一个数据包" << endl;
        }
        else {
            cout << "[ERROR]服务端未反馈正确的数据包...正在等待重传..." << endl;
            goto SEQ0SEND;
        }

        //准备开始发SEQ=1的数据包
        if (messagepointer > messagelength) {
            if (endsend() == 1) { return 1; }
            return -1;
        }
        if (messagelength - messagepointer >= MAX_DATA_LENGTH) {
            ml = MAX_DATA_LENGTH;
        }
        else {
            ml = messagelength - messagepointer + 1;
        }

        header.seq = 1;//零号序列号
        header.length = ml;//实际数据长度
        memset(sendbuffer, 0, sizeof(header) + MAX_DATA_LENGTH);//sendbuffer全部置零
        memcpy(sendbuffer, &header, sizeof(header));//拷贝header内容
        memcpy(sendbuffer + sizeof(header), message + messagepointer, ml);//拷贝数据内容
        messagepointer += ml;//更新数据指针
        header.checksum = calcksum((u_short*)sendbuffer, sizeof(header) + MAX_DATA_LENGTH);//计算校验和
        memcpy(sendbuffer, &header, sizeof(header));//填充校验和
        //cout << vericksum((u_short*)sendbuffer, sizeof(header) + MAX_DATA_LENGTH) << endl;
    SEQ1SEND:
        //发送seq=1的数据包
        cout << "[1]准备发送" << "1" << "号数据包，该数据包大小为:" << ml << " ";
        cout << "发送前检验：整体校验为" << vericksum((u_short*)sendbuffer, sizeof(header) + MAX_DATA_LENGTH) << endl;
        if (sendto(client, sendbuffer, (sizeof(header) + MAX_DATA_LENGTH), 0, (sockaddr*)&router_addr, rlen) == -1) {
            cout << "[failed]seq1数据包发送失败....请检查原因" << endl;
            return -1;
        }
        start = clock();
    SEQ1RECV:
        //如果收到数据了就不发了，否则延时重传
        while (recvfrom(client, recvbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {
            if (clock() - start > MAX_TIME) {
                if (sendto(client, sendbuffer, (sizeof(header)+MAX_DATA_LENGTH), 0, (sockaddr*)&router_addr, rlen) == -1) {
                    cout << "[failed]seq1数据包发送失败....请检查原因" << endl;
                    return -1;
                }
                start = clock();
                cout << "[ERROR]seq1数据包反馈超时....正在重传" << endl;
                //goto SEQ1SEND;
            }
        }
        //检查ack位是否正确，如果正确则准备发下一个数据包
        memcpy(&header, recvbuffer, sizeof(header));
        cout << "[GETACK]接受到的ack为: " << header.ack << "接受到的校验和为: " << vericksum((u_short*)&header, sizeof(header)) << endl;
        if (header.ack == 0 && vericksum((u_short*)&header, sizeof(header)) == 0) {
            cout << "[1CHECKED]seq1的数据包成功接受服务端ACK，准备发出下一个数据包" << endl;
        }
        else {
            cout << "[ERROR]服务端未反馈正确的数据包...正在等待重传..." << endl;
            goto SEQ1SEND;
        }
    }
}

int endsend() {
    ALLEND = clock();
    Header header;
    char* sendbuffer = new char[sizeof(header)];
    char* recvbuffer = new char[sizeof(header)];

    header.flag = OVER;
    header.checksum = calcksum((u_short*)&header, sizeof(header));
    memcpy(sendbuffer, &header, sizeof(header));
SEND:
    if (sendto(client, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
        cout << "[FAILED]传输结束信号发送失败...." << endl;
        return -1;
    }
    cout << "[SENDOK]传输结束信号发送成功...." << endl;
    clock_t start = clock();
RECV:
    while (recvfrom(client, recvbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {
        if (clock() - start > MAX_TIME) {
            if (sendto(client, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
                cout << "[FAILED]传输结束信号发送失败....请检查原因" << endl;
                return -1;
            }
            start = clock();
            cout << "[ERROR]传输结束信号反馈超时....正在重传" << endl;
            //goto SEND;
        }
    }
    memcpy(&header, recvbuffer, sizeof(header));
    if (header.flag == OVER_ACK && vericksum((u_short*)&header, sizeof(header)) == 0) {
        cout << "[FINFISH]传输结束消息发送成功....感谢使用" << endl;
        return 1;
    }
    else {
        cout << "[ERROR]数据包错误....正在等待重传" << endl;
        goto RECV;
    }
}

int tryToDisconnect() {
    Header header;
    char* sendbuffer = new char[sizeof(header)];
    char* recvbuffer = new char[sizeof(header)];
    header.seq = 0;
    header.flag = FIN;
    header.checksum = calcksum((u_short*)&header, sizeof(header));
    memcpy(sendbuffer, &header, sizeof(header));
    if (sendto(client, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
        cout << "第一次挥手发送失败" << endl;
        return -1;
    }
WAIT2:
    clock_t start = clock();
    while (recvfrom(client, recvbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {
        if (clock() - start > MAX_TIME) {
            if (sendto(client, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
                cout << "第一次挥手发送失败" << endl;
                return -1;
            }
            start = clock();
            cout << "第一次挥手消息反馈超时，已重发第一次挥手" << endl;
        }
    }
    memcpy(&header, recvbuffer, sizeof(header));
    if (header.flag == ACK && vericksum((u_short*)&header, sizeof(header)) == 0) {
        cout << "收到第二次挥手消息" << endl;
    }
    else {
        cout << "第二次挥手消息接受失败...." << endl;
        goto WAIT2;
    }
WAIT3:
    while(recvfrom(client, recvbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0){}
    memcpy(&header, recvbuffer, sizeof(header));
    if (header.flag == FIN_ACK && vericksum((u_short*)&header, sizeof(header)) == 0) {
        cout << "收到第三次挥手消息" << endl;
    }
    else {
        cout << "第三次挥手消息接受失败" << endl;
        goto WAIT3;
    }
    header.seq = 1;
    header.flag = ACK;
    header.checksum = calcksum((u_short*)&header, sizeof(header));
    memcpy(sendbuffer, &header, sizeof(header));
SENDWAVE4:
    if (sendto(client, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
        cout << "第四次挥手发送失败" << endl;
        return -1;
    }
    start = clock();
    ioctlsocket(client, FIONBIO, &unlockmode);
    while (recvfrom(client, recvbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {
        if (clock() - start > 2 * MAX_TIME) {
            cout << "接受反馈超时，重发第四次挥手" << endl;
            goto SENDWAVE4;
        }
    }
    memcpy(&header, recvbuffer, sizeof(header));
    if (header.flag == FINAL_CHECK && vericksum((u_short*)&header, sizeof(header)) == 0) {
        cout << "四次挥手完成....即将断开连接" << endl;
        return 1;
    }
    else {
        cout << "数据包错误,准备重发第四次握手" << endl;
        goto SENDWAVE4;
    }

}

void printlog() {
    cout << "             --------------传输日志--------------" << endl;
    cout << "本次报文总长度为 " << messagepointer << "字节，共分为 " << (messagepointer / 256) + 1 << "个报文段分别转发" << endl;
    double t = (ALLEND - ALLSTART) / CLOCKS_PER_SEC;
    cout << "本次传输的总时长为" <<t <<"秒"<< endl;
    t = messagepointer / t;
    cout << "本次传输吞吐率为" << t <<"字节每秒"<< endl;
}