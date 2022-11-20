#include<iostream>
#include<WinSock2.h>
#include<time.h>
#include<fstream>
#include<iostream>
#include<windows.h>
using namespace std;
#pragma comment(lib,"ws2_32.lib")
//��ʼ��dll
WSADATA wsadata;
//������������Ҫ���׽����Լ��׽�����Ҫ�󶨵ĵ�ַ
SOCKADDR_IN server_addr;
SOCKET server;
//����·������Ҫ���׽����Լ��׽�����Ҫ�󶨵ĵ�ַ
SOCKADDR_IN router_addr;;
//�����ͻ��˵ĵ�ַ
SOCKADDR_IN client_addr;
char* sbuffer = new char[1000];
char* rbuffer = new char[1000];
char* message = new char[100000000];
unsigned long long int messagepointer;
//��������ַ������
int clen = sizeof(client_addr);
int rlen = sizeof(router_addr);
//��������
u_long blockmode = 0;
u_long unblockmode = 1;
const unsigned char MAX_DATA_LENGTH = 0xff;
const u_short SOURCEIP = 0x7f01;
const u_short DESIP = 0x7f01;
const u_short SOURCEPORT = 8888;//Դ�˿���8888
const u_short DESPORT = 8887;//�ͻ��˶˿ں���8887
const unsigned char SYN = 0x1;//OVER=0,FIN=0,ACK=0,SYN=1
const unsigned char ACK = 0x2;//OVER=0,FIN=0,ACK=1,SYN=0
const unsigned char SYN_ACK = 0x3;//OVER=0,FIN=0,ACK=1,SYN=1
const unsigned char OVER = 0x8;//OVER=1,FIN=0,ACK=0,SYN=0
const unsigned char OVER_ACK = 0xA;//OVER=1,FIN=0,ACK=1,SYN=0
const unsigned char FIN = 0x10;//FIN=1,OVER=0,FIN=0,ACK=0,SYN=0
const unsigned char FIN_ACK = 0x12;//FIN=1,OVER=0,FIN=0,ACK=1,SYN=0
const unsigned char FINAL_CHECK=0x20;//FC=1.FIN=0,OVER=0,FIN=0,ACK=0,SYN=0
const double MAX_TIME = 0.2*CLOCKS_PER_SEC;
//����ͷ
struct Header {
    u_short checksum; //16λУ���
    u_short seq; //8λ���к�,��Ϊ��ͣ�ȣ�����ֻ�����λʵ����ֻ��0��1����״̬
    u_short ack; //8λack�ţ���Ϊ��ͣ�ȣ�����ֻ�����λʵ����ֻ��0��1����״̬
    u_short flag;//8λ״̬λ ������һλSYN,�����ڶ�λACK����������λFIN����������λ�ǽ���λ
    u_short length;//8λ����λ
    u_short source_ip; //16λip��ַ
    u_short des_ip; //16λip��ַ
    u_short source_port; //16λԴ�˿ں�
    u_short des_port; //16λĿ�Ķ˿ں�
    Header() {//���캯��
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
//ȫ��ʱ������
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

//sizeof�����ڴ��ֽ��� ushort��16λ2�ֽڣ�������Ҫ��size����ȡ��
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

    //ָ������˵�����
    server_addr.sin_family = AF_INET;//ʹ��IPV4
    server_addr.sin_port = htons(8888);//server�Ķ˿ں�
    server_addr.sin_addr.s_addr = htonl(2130706433);//����127.0.0.1

    //ָ��·����������
    router_addr.sin_family = AF_INET;//ʹ��IPV4
    router_addr.sin_port = htons(8886);//router�Ķ˿ں�
    router_addr.sin_addr.s_addr = htonl(2130706433);//����127.0.0.1

    //ָ��һ���ͻ���
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(8887);
    client_addr.sin_addr.s_addr = htonl(2130706433);//����127.0.0.1

    //�󶨷����
    server = socket(AF_INET, SOCK_DGRAM, 0);
    bind(server, (SOCKADDR*)&server_addr, sizeof(server_addr));

    //�����ַ����
    int clen = sizeof(client_addr);
    int rlen = sizeof(router_addr);
    cout << "[PREPARE]��ʼ���������" << endl;
}

//�����ĳ�ʼ���Ƿ���ȷ�������ṩ�����Ͱ���
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
    Header header;//����һ������ͷ
    char* recvshbuffer = new char[sizeof(header)];//����һ��������ͷһ����Ľ��ջ�����
    char* sendshbuffer = new char[sizeof(header)];//����һ��������ͷһ����ķ��ͻ�����
    cout << "[0]���ڵȴ�����...." << endl;
    //�ȴ���һ������
    while (true) {
        //�յ��˵�һ�����ֵ�����
        //�Ҿ�����д�Ĳ��� ����ֵ��̫��
        ioctlsocket(server, FIONBIO, &unblockmode);
        while (recvfrom(server, recvshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen)<=0) {
            if (clock() - linkClock > 75 * CLOCKS_PER_SEC) {
                cout << "[failed]���ӳ�ʱ,�������Զ��Ͽ�" << endl;
                return -1;
            }
            //cout << "....��һ��������Ϣ����ʧ��...." << endl;
            //return -1;
        }
        memcpy(&header, recvshbuffer, sizeof(header));//������ͷ��ֵ
        //����ǵ������������������󣬲���У������ȡ��֮�����0
        if (header.flag == SYN || vericksum((u_short*)(&header), sizeof(header)) == 0) {
            cout << "[1]�ɹ����ܿͻ������󣬵�һ�����ֽ����ɹ�...." << endl;
            break;
        }
        else {
            //cout << vericksum((u_short*)(&header), sizeof(header)) << endl;
            cout << "[failed]��һ���������ݰ��𻵣����ڵȴ��ش�..." << endl;
        }
    }
SECONDSHAKE:
    //׼�����͵ڶ������ֵ���Ϣ
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
        cout << "[failed]�ڶ���������Ϣ����ʧ��...." << endl;
        return -1;
    }
    cout << "[2]�ڶ���������Ϣ���ͳɹ�...." << endl;

    clock_t start = clock();
    if (clock() - linkClock > 75 * CLOCKS_PER_SEC) {
        cout << "[failed]���ӳ�ʱ,�������Զ��Ͽ�" << endl;
        return -1;
    }

    //�ڶ���������Ϣ�ĳ�ʱ�ش� �ش�ʱֱ���ش�sendshbuffer������ݾͿ���
    //�Ҿ�����д�Ĳ��� ����ֵ��̫��
    while (recvfrom(server, recvshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {
        if (clock() - linkClock > 75 * CLOCKS_PER_SEC) {
            cout << "[failed]���ӳ�ʱ,�������Զ��Ͽ�" << endl;
            return -1;
        }
        if (clock() - start > MAX_TIME) {
            if (sendto(server, sendshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
                cout << "[failed]�ڶ���������Ϣ���·���ʧ��...." << endl;
                return -1;
            }
            cout << "[2]�ڶ���������Ϣ���·��ͳɹ�...." << endl;
            start = clock();
        }
    }

    memcpy(&header, recvshbuffer, sizeof(header));
    if (header.flag == ACK && vericksum((u_short*)(&header), sizeof(header)) == 0) {
        cout << "[3]�ɹ����յ�����������Ϣ�����Կ�ʼ��������..." << endl;
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
        cout << "[EVERYTHING_DONE]ȷ����Ϣ����ɹ�...." << endl;
    }
    else {
        cout << "[failed]�����ڴ������ݰ��������ش����ȴ��ͻ��˵ȴ��ش�" << endl;
        if (clock() - linkClock > 75 * CLOCKS_PER_SEC) {
            cout << "[failed]���ӳ�ʱ,�������Զ��Ͽ�" << endl;
            return -1;
        }
        goto SECONDSHAKE;
    }
LAST:
    cout << "[WAITING]���ڵȴ���������...." << endl;
    return 1;
}


int loadmessage() {
    cout << "[STORE]�ļ���������Ϊ1.png" << endl;
    string filename="1.png";
    ofstream fout(filename.c_str(), ofstream::binary);
    for (int i = 0; i < messagepointer; i++)
    {
        fout << message[i];
    }
    fout.close();
    cout << "[FINISH]�ļ��ѳɹ����ص�����" << endl;
    return 0;
}


int receivemessage() {
    Header header;
    char* recvbuffer = new char[sizeof(header) + MAX_DATA_LENGTH];
    char* sendbuffer = new char[sizeof(header)];

WAITSEQ0:
    //����seq=0������
    while (true) {
        //��ʱ��������Ϊ����ģʽ
        ioctlsocket(server, FIONBIO, &unblockmode);
        while (recvfrom(server, recvbuffer, sizeof(header) + MAX_DATA_LENGTH, 0, (sockaddr*)&router_addr, &rlen) <= 0) {
            //cout << "����ʧ��...����ԭ��" << endl;
            //cout << WSAGetLastError() << endl;
        }
        memcpy(&header, recvbuffer, sizeof(header));
        //cout << header.flag << endl;
        if (header.flag == OVER) {
            //����������ȴ����....
            if (vericksum((u_short*)&header, sizeof(header)) == 0) { if (endreceive()) { return 1; }return 0; }
            else { cout << "[ERROR]���ݰ��������ڵȴ��ش�" << endl; goto WAITSEQ0; }
        }
        cout << header.seq << " " << vericksum((u_short*)recvbuffer, sizeof(header) + MAX_DATA_LENGTH) << endl;
        //printheader(header);
        //printcharstar(recvbuffer, sizeof(header) + MAX_DATA_LENGTH);
        if (header.seq == 0 && vericksum((u_short*)recvbuffer, sizeof(header)+MAX_DATA_LENGTH) == 0) {
            cout << "[0CHECKED]�ɹ�����seq=0���ݰ�" << endl;
            memcpy(message + messagepointer, recvbuffer + sizeof(header), header.length);
            messagepointer += header.length;
            break;
        }
        else {
            cout << "[ERROR]���ݰ��������ڵȴ��Է����·���" << endl;
        }
    }
    header.ack = 1;
    header.seq = 0;
    header.checksum = calcksum((u_short*)&header, sizeof(header));
    memcpy(sendbuffer, &header, sizeof(header));
SENDACK1:
    if (sendto(server, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
        cout << "[failed]ack1����ʧ��...." << endl;
        return -1;
    }
    clock_t start = clock();
RECVSEQ1:
    //����Ϊ������ģʽ
    ioctlsocket(server, FIONBIO, &unblockmode);
    while (recvfrom(server, recvbuffer, sizeof(header) + MAX_DATA_LENGTH, 0, (sockaddr*)&router_addr, &rlen) <= 0) {
        if (clock() - start > MAX_TIME) {
            if (sendto(server, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
                cout << "[failed]ack1����ʧ��...." << endl;
                return -1;
            }
            start = clock();
            cout << "[ERROR]ack1��Ϣ������ʱ....���ط�...." << endl;
            goto SENDACK1;
        }
    }
    memcpy(&header, recvbuffer, sizeof(header));
    if (header.flag == OVER) {
        //����������ȴ����....
        if (vericksum((u_short*)&header, sizeof(header) == 0)) { if (endreceive()) { return 1; }return 0; }
        else { cout << "[ERROR]���ݰ��������ڵȴ��ش�" << endl; goto WAITSEQ0; }
    }
    cout << header.seq << " " << vericksum((u_short*)recvbuffer, sizeof(header) + MAX_DATA_LENGTH) << endl;

    if (header.seq == 1 && vericksum((u_short*)recvbuffer, sizeof(header)+MAX_DATA_LENGTH)==0) {
        cout << "[1CHECKED]�ɹ�����seq=1�����ݰ������ڽ���..." << endl;
        memcpy(message + messagepointer, recvbuffer + sizeof(header), header.length);
        messagepointer += header.length;
    }
    else {
        cout << "[ERROR]���ݰ��𻵣����ڵȴ����´���" << endl;
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
    cout << "[FINISH]ȷ����Ϣ���ͳɹ�" << endl;
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
        cout << "�յ���һ�λ�����Ϣ" << endl;
    }
    else {
        cout << "��һ�λ�����Ϣ����ʧ��" << endl;
        goto RECVWAVE1;
    }
 SEND2:
    header.seq = 0;
    header.flag = ACK;
    header.checksum = calcksum((u_short*)&header, sizeof(header));
    memcpy(sendbuffer, &header, sizeof(header));
    if (sendto(server, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
        cout << "�ڶ��λ�����Ϣ����ʧ��..." << endl;
        return -1;
    }
    Sleep(80);
SEND3:
    header.seq = 1;
    header.flag = FIN_ACK;
    header.checksum = calcksum((u_short*)&header, sizeof(header));
    memcpy(sendbuffer, &header, sizeof(header));
    if (sendto(server, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
        cout << "�����λ�����Ϣ����ʧ��..." << endl;
        return -1;
    }
    clock_t start = clock();
    ioctlsocket(server, FIONBIO, &unblockmode);
    while (recvfrom(server, recvbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {
        if (clock() - start > MAX_TIME) {
            cout << "���Ĵλ�����Ϣ�����ӳ�...׼���ط������λ���" << endl;
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
        cout << "�ɹ�����ȷ�ϱ���" << endl;
    }
    start = clock();
    while (recvfrom(server, recvbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {
        if (clock() - start > 10 * MAX_TIME) {
            cout << "�Ĵλ��ֽ������Ѿ��Ͽ�����" << endl;
            return 1;
        }
    }
    goto SEND5;
}