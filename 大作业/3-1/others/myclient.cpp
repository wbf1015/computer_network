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

char * message = new char[10000000];
unsigned long long int messagelength=0;//最后一个要传的下标是多少
unsigned long long int messagepointer=0;//下一个该传的位置

int slen = sizeof(server_addr);
int rlen = sizeof(router_addr);

//常数设置
u_long mode = 1;
const unsigned char MAX_DATA_LENGTH = 0xff;
const u_short SOURCEIP = 0x7f01;
const u_short DESIP = 0x7f01;
const u_short SOURCEPORT = 8887;//客户端端口是8887
const u_short DESPORT = 8888;//路由器端口号是8886
const unsigned char SYN = 0x1;//FIN=0,ACK=0,SYN=1
const unsigned char ACK = 0x2;//FIN=0,ACK=1,SYN=0
const unsigned char SYN_ACK = 0x3;//FIN=0,ACK=1,SYN=1
const unsigned char OVER = 0x8//OVER=1,FIN=0,ACK=0,SYN=0
const unsigned char OVER_ACK = 0xA//OVER=1,FIN=0,ACK=1,SYN=0
const double MAX_TIME = CLOCKS_PER_SEC;
//数据头
struct Header {
    unsigned char seq; //8位序列号,因为是停等，所以只有最低位实际上只有0和1两种状态
    unsigned char ack; //8位ack号，因为是停等，所以只有最低位实际上只有0和1两种状态
    unsigned char empty;//8位空位
    unsigned char flag;//8位状态位 倒数第一位SYN,倒数第二位ACK，倒数第三位FIN
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

int main() {
    initialNeed();
    if (tryToConnect() <= 0) {
        cout << "握手失败，请检查连接后再试" << endl;
        return -1;
    }
    cout << "握手成功！可以进行数据传输！...";



}

void test() {
    int i = 48;
    while (true) {
        sendbuffer[0] = (char)i; sendbuffer[1] = '\0'; i++;
        if (i > 57) { i = 48; }
        sendto(client, sendbuffer, 1000, 0, (sockaddr*)&router_addr, rlen);
        recvfrom(client, recvbuffer, 1000, 0, (sockaddr*)&router_addr, &slen);
        cout << recvbuffer << endl;
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

    client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    bind(client, (SOCKADDR*)&client_addr, sizeof(client_addr));

}

int tryToConnect() {
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
    header.empty = 0;
    header.empty2 = 0;
    header.checksum = calcksum((u_short*)&header, sizeof(header));
    //cout << vericksum((u_short*)&header, sizeof(header)) << endl;
    u_short* test = (u_short*)&header;
    memcpy(sendshbuffer, &header, sizeof(header));
    if (sendto(client, sendshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
        cout << "第一次握手请求发送失败..." << endl;
        return -1;
    }
    cout << "第一次握手消息发送成功...." << endl;
    //设置是否为非阻塞模式
    ioctlsocket(client, FIONBIO, &mode);

FIRSTSHAKE:
    //设置计时器
    clock_t start = clock();

    //第一次握手重传
    while (recvfrom(client, recvshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {
        if (clock() - start > MAX_TIME) {
            if (sendto(client, sendshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
                cout << "第一次握手请求发送失败..." << endl;
                return -1;
            }
            start = clock();
            cout << "第一次握手消息反馈超时....正在重新发送" << endl;
        }
    }

    //检验第二次握手信息是否准确
    memcpy(&header, recvshbuffer, sizeof(header));
    if (header.flag == SYN_ACK && vericksum((u_short*)&header, sizeof(header)) == 0) {
        cout << "正确接受第二次握手信息" << endl;
    }
    else {
        cout << "不是期待的服务端数据包,即将重传第一次握手数据包...." << endl;
        goto FIRSTSHAKE;
    }

    //发送第三次握手信息
    header.source_port = SOURCEPORT;
    header.des_port = DESPORT;
    header.flag = ACK;
    header.seq = 1;
    header.ack = 1;
    header.checksum = calcksum((u_short*)&header, sizeof(header));
    memcpy(sendshbuffer, &header, sizeof(header));
    if (sendto(client, sendshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
        cout << "第三次握手信息发送失败，请稍后再试...." << endl;
        return -1;
    }
    cout << "与服务器建联成功，准备发送数据" << endl;
    return 1;
}

int send(){
    Header header;
    char * recvbuffer =new char[sizeof(header)];
    char * sendbuffer =new char[sizeof(header)+MAX_DATA_LENGTH];
    
    while(true){

        int ml;//本次数据传输长度
        if(messagepointer>messagelength){
            if(endsend()==1){return 1;}
            return -1;
        }
        if(messagelength-messagepointer>=MAX_DATA_LENGTH){
            ml=MAX_DATA_LENGTH;
        }else{
            ml=messagelength-messagepointer+1;
        }

        header.seq=0;//零号序列号
        header.length=ml;//实际数据长度
        memset(sendbuffer,0,sizeof(header)+MAX_DATA_LENGTH);//sendbuffer全部置零
        memcpy(sendbuffer,&header,sizeof(header));//拷贝header内容
        memcpy(sendbuffer+sizeof(header),message+messagepointer,ml);//拷贝数据内容
        messagepointer+=ml;//更新数据指针
        header.checksum=calcksum((u_short*)sendbuffer,sizeof(header)+MAX_DATA_LENGTH);//计算校验和
        memcpy(sendbuffer,&header,sizeof(header));//填充校验和
SEQ0SEND:
        //发送seq=0的数据包
        if(sendto(client,sendbuffer,sizeof(header)+MAX_DATA_LENGTH,0,(sockaddr*)router_addr,rlen)==-1){
            cout<<"发送失败....请检查原因"<<endl;
            return -1;
        }
        clock_t start =clock();
SEQ0RECV:
        //如果收到数据了就不发了，否则延时重传
        while(recvfrom(client,recvbuffer,sizeof(header),0,(sockaddr*)router_addr,&rlen)<=0){
            if(clock()-start > MAX_TIME){
                if (sendto(client, sendshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
                    cout << "SEQ=0的消息发送失败....请检查原因" << endl;
                    return -1;
                }
                start = clock();
                cout << "SEQ=0的消息反馈超时....正在重传" << endl;
                //goto SEQ0SEND;
            }
        }
        //检查ack位是否正确，如果正确则准备发下一个数据包
        memcpy(&header,recvbuffer,sizeof(header));
        if(header.ack==0 && vericksum((u_short*)&header),sizeof(header)==0){
            cout<<"seq=0的数据包成功接受服务端ACK，准备发出下一个数据包"<<endl;
        }else{
            cout<<"不是期待的数据包，准备重发SEQ=0的数据包"<<endl;
            goto SEQ0RECV;
        }

        //准备开始发SEQ=1的数据包
        int ml;//本次数据传输长度
        if(messagepointer>messagelength){
            if(endsend()==1){return 1;}
            return -1;
        }
        if(messagelength-messagepointer>=MAX_DATA_LENGTH){
            ml=MAX_DATA_LENGTH;
        }else{
            ml=messagelength-messagepointer+1;
        }

        header.seq=1;//零号序列号
        header.length=ml;//实际数据长度
        memset(sendbuffer,0,sizeof(header)+MAX_DATA_LENGTH);//sendbuffer全部置零
        memcpy(sendbuffer,&header,sizeof(header));//拷贝header内容
        memcpy(sendbuffer+sizeof(header),message+messagepointer,ml);//拷贝数据内容
        messagepointer+=ml;//更新数据指针
        header.checksum=calcksum((u_short*)sendbuffer,sizeof(header)+MAX_DATA_LENGTH);//计算校验和
        memcpy(sendbuffer,&header,sizeof(header));//填充校验和
SEQ1SEND:
        //发送seq=0的数据包
        if(sendto(client,sendbuffer,sizeof(header)+MAX_DATA_LENGTH,0,(sockaddr*)router_addr,rlen)==-1){
            cout<<"发送失败....请检查原因"<<endl;
            return -1;
        }
        start =clock();
SEQ1RECV
        //如果收到数据了就不发了，否则延时重传
        while(recvfrom(client,recvbuffer,sizeof(header),0,(sockaddr*)router_addr,&rlen)<=0){
            if(clock()-start > MAX_TIME){
                if (sendto(client, sendshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
                    cout << "SEQ=1的消息发送失败....请检查原因" << endl;
                    return -1;
                }
                start = clock();
                cout << "SEQ=1的消息反馈超时....正在重传" << endl;
                //goto SEQ1SEND;
            }
        }
        //检查ack位是否正确，如果正确则准备发下一个数据包
        memcpy(&header,recvbuffer,sizeof(header));
        if(header.ack==1 && vericksum((u_short*)&header),sizeof(header)==0){
            cout<<"seq=1的数据包成功接受服务端ACK，准备发出下一个数据包"<<endl;
        }else{
            cout<<"不是期待的数据包，准备重发SEQ=0的数据包"<<endl;
            goto SEQ1RECV;
        }
    }
}

int endSend(){
    Header header;
    char * sendbuffer=new char[sizeof(header)];
    char * recvbuffer=new char[sizeof(header)];

    header.flag=OVER;
    header.checksum=calcksum((u_short*)&header,sizeof(header));
    memcpy(sendbuffer,&header,sizeof(header));
SEND:
    if(sendto(client,sendbuffer,sizeof(header),0,(sockadr*)router_addr,rlen)==-1){
        cout<<"传输结束信号发送失败...."<<endl;
        return -1
    }
    cout<<"传输结束信号发送成功...."<<endl;
    clock_t start = clock();
RECV:
    while(recvfrom(client,recvbuffer,sizeof(header),0,(sockaddr*)router_addr,&rlen)<=0){
        if(clock()-start > MAX_TIME){
            if (sendto(client, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
                 cout << "传输结束信号发送失败....请检查原因" << endl;
                return -1;
            }
            start = clock();
            cout << "传输结束信号反馈超时....正在重传" << endl;
            //goto SEND;
        }
    }
    memcpy(&header,recvbuffer,sizeof(header));
    if(header.flag==OVER_ACK && vericksum((u_short*)header,sizeof(header))==0){
        cout<<"传输结束消息发送成功....感谢使用"<<endl;
        return 1;
    }else{
        cout<<"数据包错误....正在等待重传"<<endl;
        goto RECV;
    }
}