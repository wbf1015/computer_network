#include<iostream>
#include<WinSock2.h>
#include<time.h>
#include<fstream>
#include<iostream>
#include<windows.h>
#include<map>
#include<mutex>
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
char* message = new char[100000000];
unsigned long long int messagepointer;
//服务器地址的性质
int clen = sizeof(client_addr);
int rlen = sizeof(router_addr);
//常数设置
u_long blockmode = 0;
u_long unblockmode = 1;
const int WINDOWSIZE = 4;//滑动窗口的大小为4
const int SEQSIZE = INT_MAX;//序列号的大小为9(0-8)
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
const unsigned char FINAL_CHECK = 0x20;//FC=1.FIN=0,OVER=0,FIN=0,ACK=0,SYN=0
const double MAX_TIME = 0.1 * CLOCKS_PER_SEC;
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
//全局时钟设置
clock_t linkClock;

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


bool canLoad = false;//是否能够下载文件到本地
int SEQWanted = 0;//现在想要收到的
bool canSend = false;//能不能发送现在SEQWanted的ACK信息
bool canExit = false;//现在能不能退出
//使用互斥量来保护一些全局变量
mutex mtx;
DWORD WINAPI Recvprocess(LPVOID p);
DWORD WINAPI Sendprocess(LPVOID p);

int main() {
    initialNeed();
    tryToConnect();
    HANDLE cthread = CreateThread(NULL, 0, Recvprocess, (LPVOID)nullptr, 0, NULL);
    CloseHandle(cthread);
    HANDLE ccthread = CreateThread(NULL, 0, Sendprocess, (LPVOID)nullptr, 0, NULL);
    CloseHandle(ccthread);
    while (true) {
        if (canLoad) {
            loadmessage();
            break;
        }
    }
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
    cout << "[PREPARE]初始化工作完成" << endl;
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
    linkClock = clock();
    Header header;//声明一个数据头
    char* recvshbuffer = new char[sizeof(header)];//创建一个和数据头一样大的接收缓冲区
    char* sendshbuffer = new char[sizeof(header)];//创建一个和数据头一样大的发送缓冲区
    cout << "[0]正在等待连接...." << endl;
    //等待第一次握手
    while (true) {
        //收到了第一次握手的申请
        //我觉得他写的不对 返回值不太对
        ioctlsocket(server, FIONBIO, &unblockmode);
        while (recvfrom(server, recvshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {
            if (clock() - linkClock > 75 * CLOCKS_PER_SEC) {
                cout << "[failed]连接超时,服务器自动断开" << endl;
                return -1;
            }
            //cout << "....第一次握手信息接受失败...." << endl;
            //return -1;
        }
        memcpy(&header, recvshbuffer, sizeof(header));//给数据头赋值
        //如果是单纯的请求建立连接请求，并且校验和相加取反之后就是0
        if (header.flag == SYN || vericksum((u_short*)(&header), sizeof(header)) == 0) {
            cout << "[1]成功接受客户端请求，第一次握手建立成功...." << endl;
            break;
        }
        else {
            //cout << vericksum((u_short*)(&header), sizeof(header)) << endl;
            cout << "[failed]第一次握手数据包损坏，正在等待重传..." << endl;
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
    //cout << vericksum((u_short*)&header, sizeof(header)) << endl;
    memcpy(sendshbuffer, &header, sizeof(header));
    if (sendto(server, sendshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
        cout << "[failed]第二次握手消息发送失败...." << endl;
        return -1;
    }
    cout << "[2]第二次握手消息发送成功...." << endl;

    clock_t start = clock();
    if (clock() - linkClock > 75 * CLOCKS_PER_SEC) {
        cout << "[failed]连接超时,服务器自动断开" << endl;
        return -1;
    }

    //第二次握手消息的超时重传 重传时直接重传sendshbuffer里的内容就可以
    //我觉得他写的不对 返回值不太对
    while (recvfrom(server, recvshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {
        if (clock() - linkClock > 75 * CLOCKS_PER_SEC) {
            cout << "[failed]连接超时,服务器自动断开" << endl;
            return -1;
        }
        if (clock() - start > MAX_TIME) {
            if (sendto(server, sendshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
                cout << "[failed]第二次握手消息重新发送失败...." << endl;
                return -1;
            }
            cout << "[2]第二次握手消息重新发送成功...." << endl;
            start = clock();
        }
    }

    memcpy(&header, recvshbuffer, sizeof(header));
    if (header.flag == ACK && vericksum((u_short*)(&header), sizeof(header)) == 0) {
        cout << "[3]成功接收第三次握手消息！可以开始接收数据..." << endl;
    SEND4:
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
        cout << "[EVERYTHING_DONE]确认信息传输成功...." << endl;
    }
    else {
        cout << "[failed]不是期待的数据包，正在重传并等待客户端等待重传" << endl;
        if (clock() - linkClock > 75 * CLOCKS_PER_SEC) {
            cout << "[failed]连接超时,服务器自动断开" << endl;
            return -1;
        }
        goto SECONDSHAKE;
    }
LAST:
    cout << "[WAITING]正在等待接收数据...." << endl;
    return 1;
}
int loadmessage() {
    cout << "[STORE]文件将被保存为1.png" << endl;
    string filename = "1.png";
    ofstream fout(filename.c_str(), ofstream::binary);
    for (int i = 0; i < messagepointer; i++)
    {
        fout << message[i];
    }
    fout.close();
    cout << "[FINISH]文件已成功下载到本地" << endl;
    return 0;
}


DWORD WINAPI Recvprocess(LPVOID p) {
    cout << "successfully created RecvProcess" << endl;
    Header header;
    char* recvbuffer = new char[sizeof(header) + MAX_DATA_LENGTH];
    char* sendbuffer = new char[sizeof(header)];
    int nowpointer=0;
    clock_t c=clock();//如果
    while (true) {
        while (recvfrom(server, recvbuffer, sizeof(header) + MAX_DATA_LENGTH, 0, (sockaddr*)&client_addr, &rlen) > 0) {
            c = clock();
            memcpy(&header, recvbuffer, sizeof(header));
            //判断校验和
            if (vericksum((u_short*)recvbuffer, sizeof(header) + MAX_DATA_LENGTH) != 0) {
                cout << "校验和错误" << endl;
                continue;
            }
            //收到了退出请求
            if (header.flag == OVER) {
                //要访问全局变量了，需要加锁
                mtx.lock();
                messagepointer = nowpointer - 1;
                canExit = true;
                canLoad = true;
                mtx.unlock();
                return 1;
            }
            //要对全局变量操作，加锁
            mtx.lock();
            //成功接收了数据包，是需要的数据包
            if (header.seq == SEQWanted&&!canSend) {
                cout << "成功接收" << header.seq << "号数据包" << endl;
                canSend = true;
                memcpy(message + nowpointer, recvbuffer + sizeof(header), header.length);
                nowpointer += header.length;
                mtx.unlock();
            }
            else {
                //可能是因为线程没同步正确
                //也有可能就是传乱了
                cout << "错误接受" << header.seq << "号数据包，现在需要接收" << SEQWanted << "号数据包" << endl;
                mtx.unlock();
                /*               header.ack = SEQWanted - 1;
                header.checksum = calcksum((u_short*)&header, sizeof(header));
                memcpy(sendbuffer, &header, sizeof(header));
                sendto(server, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen);
                cout << "成功发送" << header.ack << "号ACK" << endl;*/
            }
        }
        mtx.lock();
        //长时间没有收到客户端的信息那就直接退出
        if (clock() - c > MAX_TIME&&SEQWanted>0) {
            cout << "[exit] 开启自动退出机制" << endl;
            messagepointer = nowpointer - 1;
            canExit = true;
            canLoad = true;
            mtx.unlock();
            return 1;
        }
        mtx.unlock();
    }
    return 1;
}
DWORD WINAPI Sendprocess(LPVOID p) {
    cout << "successfully created SendProcess" << endl;
    Header header;
    char* sendbuffer = new char[sizeof(header)];
    clock_t c=clock();
    while (true) {
        //可以退出了
        mtx.lock();
        if (canExit) {
            header.ack = 0;
            header.flag = OVER;
            header.checksum= calcksum((u_short*)&header, sizeof(header));
            memcpy(sendbuffer, &header, sizeof(header));
            sendto(server, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen);
            cout << "成功发送结束信号"<<endl;
            mtx.unlock();
            return 1;
        }
        mtx.unlock();
        //加锁
        mtx.lock();
        //如果现在可以发送ACK的话
        if (canSend) {
            header.ack = SEQWanted;
            SEQWanted++;
            if (SEQWanted > SEQSIZE) { SEQWanted = 1; }
            canSend = false;
            header.checksum = calcksum((u_short*)&header, sizeof(header));
            memcpy(sendbuffer, &header, sizeof(header));
            sendto(server, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen);
            cout << "成功发送" << header.ack << "号ACK" << endl;
            c = clock();
            //if解锁
            mtx.unlock();
            continue;
        }
        else {
            //如果长时间没有收到自己想要的数据包
            //就把想要的数据包发过去
            //因为多线程有的时候可能有点乱
            //else解锁
            mtx.unlock();
            //加锁
            mtx.lock();
            if (clock() - c > MAX_TIME&&SEQWanted>0) {
                header.ack = SEQWanted - 1;
                header.checksum = calcksum((u_short*)&header, sizeof(header));
                memcpy(sendbuffer, &header, sizeof(header));
                sendto(server, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen);
                cout << "成功发送" << header.ack << "号ACK" << endl;
                c = clock();
                //if解锁
                mtx.unlock();
                continue;
            }
            //else解锁
            mtx.unlock();
        }
    }
    return 1;
}


//多线程这仨函数也是废了
int receivemessage() {
    Header header;
    char* recvbuffer = new char[sizeof(header) + MAX_DATA_LENGTH];
    char* sendbuffer = new char[sizeof(header)];
    int nowpointer = 0;//下一个要接收的
    while (true) {//反正就是一直接收
        ioctlsocket(server, FIONBIO, &unblockmode);//设置为非阻塞
        while (recvfrom(server, recvbuffer, sizeof(header) + MAX_DATA_LENGTH, 0, (sockaddr*)&router_addr, &rlen) > 0) {//如果我收到了
            memcpy(&header, recvbuffer, sizeof(header));
            if (header.flag == OVER) { endreceive(); return 1; }
            //cout << header.seq << "  " << nowpointer << "  " << vericksum((u_short*)recvbuffer, sizeof(header) + MAX_DATA_LENGTH) << endl;
            if (header.seq == nowpointer && vericksum((u_short*)recvbuffer, sizeof(header) + MAX_DATA_LENGTH) == 0) {//正确接收到想要的数据包
                cout << "[ACK]成功接收序列号为" << header.seq << "的数据报,正在发送ack" << "下一个期望收到的数据报序列号为" << header.seq + 1 << endl;
                memcpy(message + messagepointer, recvbuffer + sizeof(header), header.length);//拷贝数据内容
                messagepointer += header.length;//重置位置指针
                header.ack = nowpointer;//表示这个包我收到了
                if (nowpointer == SEQSIZE) { nowpointer = 0; }
                else { nowpointer++; }//下一个我想要收到的包的号
                header.checksum = calcksum((u_short*)&header, sizeof(header));
                memcpy(sendbuffer, &header, sizeof(header));
                sendto(server, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen);//发送ACK
                continue;
            }
            /*
            if (nowpointer == 0) { header.seq = SEQSIZE; }
            else { header.seq = nowpointer-1; }//发送上一个包
            header.checksum = calcksum((u_short*)&header, sizeof(header));
            memcpy(sendbuffer, &header, sizeof(header));
            sendto(server, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen);//发送ACK
            cout << "不是期待的ack，已重发" << header.seq << endl;
            */
            cout << "[ERROR]不是期待的ack" << endl;
        }
    }

}
int endreceive() {
    Header header;
    char* sendbuffer = new char[sizeof(header)];
    header.flag = OVER_ACK;
    header.checksum = calcksum((u_short*)&header, sizeof(header));
    memcpy(sendbuffer, &header, sizeof(header));
    if (sendto(server, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) >= 0) return 1;
    cout << "[FINISH]确认消息发送成功" << endl;
    return 0;
}
int tryToDisconnect() {
    Header header;
    char* sendbuffer = new char[sizeof(header)];
    char* recvbuffer = new char[sizeof(header)];
RECVWAVE1:
    while (recvfrom(server, recvbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {}
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