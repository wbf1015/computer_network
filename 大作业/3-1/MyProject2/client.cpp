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
//��ʼ��dll
WSADATA wsadata;

//�����׽�����Ҫ�󶨵ĵ�ַ
SOCKADDR_IN server_addr;
//����·������Ҫ�󶨵ĵ�ַ
SOCKADDR_IN router_addr;
//�����ͻ��˵ĵ�ַ
SOCKADDR_IN client_addr;
SOCKET client;

char* message = new char[10000000];
char* sbuffer = new char[1000];
char* rbuffer = new char[1000];
unsigned long long int messagelength = 0;//���һ��Ҫ�����±��Ƕ���
unsigned long long int messagepointer = 0;//��һ���ô���λ��

int slen = sizeof(server_addr);
int rlen = sizeof(router_addr);

//��������
u_long unlockmode = 1;
u_long lockmode = 0;
const unsigned char MAX_DATA_LENGTH = 0xff;
const u_short SOURCEIP = 0x7f01;
const u_short DESIP = 0x7f01;
const u_short SOURCEPORT = 8887;//�ͻ��˶˿���8887
const u_short DESPORT = 8888;//����˶˿ں���8888
const unsigned char SYN = 0x1;//FIN=0,ACK=0,SYN=1
const unsigned char ACK = 0x2;//FIN=0,ACK=1,SYN=0
const unsigned char SYN_ACK = 0x3;//FIN=0,ACK=1,SYN=1
const unsigned char OVER = 0x8;//OVER=1,FIN=0,ACK=0,SYN=0
const unsigned char OVER_ACK = 0xA;//OVER=1,FIN=0,ACK=1,SYN=0
const unsigned char FIN = 0x10;//FIN=1,OVER=0,FIN=0,ACK=0,SYN=0
const unsigned char FIN_ACK = 0x12;//FIN=1,OVER=0,FIN=0,ACK=1,SYN=0
const unsigned char FINAL_CHECK = 0x20;//FC=1.FIN=0,OVER=0,FIN=0,ACK=0,SYN=0
const int MAX_TIME = 0.2*CLOCKS_PER_SEC; //������ӳ�ʱ��
//����ͷ
struct Header {
    u_short checksum; //16λУ���
    u_short seq; //16λ���к�,��Ϊ��ͣ�ȣ�����ֻ�����λʵ����ֻ��0��1����״̬
    u_short ack; //16λack�ţ���Ϊ��ͣ�ȣ�����ֻ�����λʵ����ֻ��0��1����״̬
    u_short flag;//16λ״̬λ ������һλSYN,�����ڶ�λACK����������λFIN
    u_short length;//16λ����λ
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

int main() {
    initialNeed();
    tryToConnect();
    loadMessage();
    sendmessage();
    tryToDisconnect();

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

    server_addr.sin_family = AF_INET;//ʹ��IPV4
    server_addr.sin_port = htons(8888);//server�Ķ˿ں�
    server_addr.sin_addr.s_addr = htonl(2130706433);//����127.0.0.1

    router_addr.sin_family = AF_INET;//ʹ��IPV4
    router_addr.sin_port = htons(8886);//router�Ķ˿ں�
    router_addr.sin_addr.s_addr = htonl(2130706433);//����127.0.0.1

    //ָ��һ���ͻ���
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(8887);
    client_addr.sin_addr.s_addr = htonl(2130706433);//����127.0.0.1

    client = socket(AF_INET, SOCK_DGRAM, 0);
    bind(client, (SOCKADDR*)&client_addr, sizeof(client_addr));

}

int tryToConnect() {
    Header header;
    char* recvshbuffer = new char[sizeof(header)];
    char* sendshbuffer = new char[sizeof(header)];

    //��һ����������
    header.source_port = SOURCEPORT;
    header.des_port = DESPORT;
    header.flag = SYN;
    header.seq = 0;
    header.ack = 0;
    header.length = 0;
    header.checksum = calcksum((u_short*)&header, sizeof(header));
    cout << vericksum((u_short*)&header, sizeof(header)) << endl;
    u_short* test = (u_short*)&header;
    memcpy(sendshbuffer, &header, sizeof(header));
SEND1:
    if (sendto(client, sendshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
        cout << "��һ������������ʧ��..." << endl;
        return -1;
    }
    cout << "��һ��������Ϣ���ͳɹ�...." << endl;
    //�����Ƿ�Ϊ������ģʽ
    ioctlsocket(client, FIONBIO, &unlockmode);

FIRSTSHAKE:
    //���ü�ʱ��
    clock_t start = clock();

    //��һ�������ش�
    while (recvfrom(client, recvshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {
        if (clock() - start > MAX_TIME) {
            if (sendto(client, sendshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
                cout << "��һ������������ʧ��..." << endl;
                return -1;
            }
            start = clock();
            cout << "��һ��������Ϣ������ʱ....�������·���" << endl;
            goto SEND1;
        }
    }

    //����ڶ���������Ϣ�Ƿ�׼ȷ
    memcpy(&header, recvshbuffer, sizeof(header));
    if (header.flag == SYN_ACK && vericksum((u_short*)&header, sizeof(header)) == 0) {
        cout << "��ȷ���ܵڶ���������Ϣ" << endl;
    }
    else {
        cout << "�����ڴ��ķ�������ݰ�,�����ش���һ���������ݰ�...." << endl;
        goto SEND1;
    }

    //���͵�����������Ϣ
    header.source_port = SOURCEPORT;
    header.des_port = DESPORT;
    header.flag = ACK;
    header.seq = 1;
    header.ack = 1;
    header.checksum = calcksum((u_short*)&header, sizeof(header));
    cout << vericksum((u_short*)&header, sizeof(header)) << endl;
    memcpy(sendshbuffer, &header, sizeof(header));
SEND3:
    if (sendto(client, sendshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
        cout << "������������Ϣ����ʧ�ܣ����Ժ�����...." << endl;
        return -1;
    }
    start = clock();
    while (recvfrom(client, recvshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {
        if (clock() - start >= 5 * MAX_TIME) {
            cout << "������������Ϣ������ʱ...�������·���" << endl;
            goto SEND3;
        }
    }
    cout << "������������ɹ���׼����������" << endl;
    return 1;
}

int loadMessage() {
    string filename;
    cout << "�������ļ�����" << endl;
    cin >> filename;
    ifstream fin(filename.c_str(), ifstream::binary);//�Զ����Ʒ�ʽ���ļ�
    unsigned long long int index = 0;
    unsigned char temp = fin.get();
    while (fin)
    {
        message[index++] = temp;
        temp = fin.get();
    }
    messagelength = index-1;
    fin.close();
    cout << "����ļ����빤��" << endl;
    return 0;
}

int sendmessage() {
    //�����Ƿ�Ϊ������ģʽ
    ioctlsocket(client, FIONBIO, &unlockmode);

    Header header;
    char* recvbuffer = new char[sizeof(header)];
    char* sendbuffer = new char[sizeof(header) + MAX_DATA_LENGTH];

    while (true) {

        int ml;//�������ݴ��䳤��
        if (messagepointer > messagelength) {//����Ҫ�ٷ��ˣ���������
            if (endsend() == 1) { return 1; }
            return -1;
        }
        if (messagelength - messagepointer >= MAX_DATA_LENGTH) {//���԰�������޶ȷ���
            ml = MAX_DATA_LENGTH;
        }
        else {
            ml = messagelength - messagepointer + 1;//��Ҫ���㷢�͵ĳ���
        }

        header.seq = 0;//��η�����������к�
        header.length = ml;//ʵ�����ݳ���
        memset(sendbuffer, 0, sizeof(header) + MAX_DATA_LENGTH);//sendbufferȫ������
        memcpy(sendbuffer, &header, sizeof(header));//����header����
        memcpy(sendbuffer+sizeof(header), message + messagepointer, ml);//������������
        messagepointer += ml;//��������ָ��
        header.checksum = calcksum((u_short*)sendbuffer, sizeof(header)+MAX_DATA_LENGTH);//����У���
        memcpy(sendbuffer, &header, sizeof(header));//���У���
        //cout << vericksum((u_short*)sendbuffer, sizeof(header) + MAX_DATA_LENGTH) << endl;
        //cout << header.seq << endl;
    SEQ0SEND:
        //����seq=0�����ݰ�
        cout << "׼������" << "0" << "�����ݰ��������ݰ���СΪ:" << ml<<" ";
        cout << "У���Ϊ" << vericksum((u_short*)sendbuffer, sizeof(header) + MAX_DATA_LENGTH) << endl;
        //printheader(header);
        //printcharstar(sendbuffer, sizeof(header) + MAX_DATA_LENGTH);
        if (sendto(client, sendbuffer, (sizeof(header) + MAX_DATA_LENGTH), 0, (sockaddr*)&router_addr, rlen) == -1) {
            cout << "seq0���ݰ�����ʧ��....����ԭ��" << endl;
            return -1;
        }
        clock_t start = clock();
    SEQ0RECV:
        //����յ������˾Ͳ����ˣ�������ʱ�ش�
        //�����˷����������Բ��Ῠס
        while (recvfrom(client, recvbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {
            if (clock() - start > MAX_TIME) {
                //printcharstar(sendbuffer, sizeof(header) + MAX_DATA_LENGTH);
                if (sendto(client, sendbuffer, (sizeof(header)+MAX_DATA_LENGTH), 0, (sockaddr*)&router_addr, rlen) == -1) {
                    cout << "seq0���ݰ�����ʧ��....����ԭ��" << endl;
                    return -1;
                }
                start = clock();
                cout << "seq0���ݰ�������ʱ....�����ش�" << endl;
                //goto SEQ0SEND;
            }
        }
        //���ackλ�Ƿ���ȷ�������ȷ��׼������һ�����ݰ�
        memcpy(&header, recvbuffer, sizeof(header));
        cout << "���ܵ���ackΪ" << header.ack << "���ܵ���У���Ϊ" << vericksum((u_short*)&header, sizeof(header)) << endl;
        if (header.ack == 1 && vericksum((u_short*)&header, sizeof(header) == 0)) {
            cout << "seq0���ݰ��ɹ����ܷ����ACK��׼��������һ�����ݰ�" << endl;
        }
        else {
            cout << "�����δ������ȷ�����ݰ�...���ڵȴ��ش�..." << endl;
            goto SEQ0SEND;
        }

        //׼����ʼ��SEQ=1�����ݰ�
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

        header.seq = 1;//������к�
        header.length = ml;//ʵ�����ݳ���
        memset(sendbuffer, 0, sizeof(header) + MAX_DATA_LENGTH);//sendbufferȫ������
        memcpy(sendbuffer, &header, sizeof(header));//����header����
        memcpy(sendbuffer + sizeof(header), message + messagepointer, ml);//������������
        messagepointer += ml;//��������ָ��
        header.checksum = calcksum((u_short*)sendbuffer, sizeof(header) + MAX_DATA_LENGTH);//����У���
        memcpy(sendbuffer, &header, sizeof(header));//���У���
        //cout << vericksum((u_short*)sendbuffer, sizeof(header) + MAX_DATA_LENGTH) << endl;
    SEQ1SEND:
        //����seq=1�����ݰ�
        cout << "׼������" << "1" << "�����ݰ��������ݰ���СΪ:" << ml << " ";
        cout << "У���Ϊ" << vericksum((u_short*)sendbuffer, sizeof(header) + MAX_DATA_LENGTH) << endl;
        if (sendto(client, sendbuffer, (sizeof(header) + MAX_DATA_LENGTH), 0, (sockaddr*)&router_addr, rlen) == -1) {
            cout << "seq1���ݰ�����ʧ��....����ԭ��" << endl;
            return -1;
        }
        start = clock();
    SEQ1RECV:
        //����յ������˾Ͳ����ˣ�������ʱ�ش�
        while (recvfrom(client, recvbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {
            if (clock() - start > MAX_TIME) {
                if (sendto(client, sendbuffer, (sizeof(header)+MAX_DATA_LENGTH), 0, (sockaddr*)&router_addr, rlen) == -1) {
                    cout << "seq1���ݰ�����ʧ��....����ԭ��" << endl;
                    return -1;
                }
                start = clock();
                cout << "seq1���ݰ�������ʱ....�����ش�" << endl;
                //goto SEQ1SEND;
            }
        }
        //���ackλ�Ƿ���ȷ�������ȷ��׼������һ�����ݰ�
        memcpy(&header, recvbuffer, sizeof(header));
        cout << "���ܵ���ackΪ: " << header.ack << "���ܵ���У���Ϊ: " << vericksum((u_short*)&header, sizeof(header)) << endl;
        if (header.ack == 0 && vericksum((u_short*)&header, sizeof(header)) == 0) {
            cout << "seq1�����ݰ��ɹ����ܷ����ACK��׼��������һ�����ݰ�" << endl;
        }
        else {
            cout << "�����δ������ȷ�����ݰ�...���ڵȴ��ش�..." << endl;
            goto SEQ1SEND;
        }
    }
}

int endsend() {
    Header header;
    char* sendbuffer = new char[sizeof(header)];
    char* recvbuffer = new char[sizeof(header)];

    header.flag = OVER;
    header.checksum = calcksum((u_short*)&header, sizeof(header));
    memcpy(sendbuffer, &header, sizeof(header));
SEND:
    if (sendto(client, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
        cout << "��������źŷ���ʧ��...." << endl;
        return -1;
    }
    cout << "��������źŷ��ͳɹ�...." << endl;
    clock_t start = clock();
RECV:
    while (recvfrom(client, recvbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {
        if (clock() - start > MAX_TIME) {
            if (sendto(client, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
                cout << "��������źŷ���ʧ��....����ԭ��" << endl;
                return -1;
            }
            start = clock();
            cout << "��������źŷ�����ʱ....�����ش�" << endl;
            //goto SEND;
        }
    }
    memcpy(&header, recvbuffer, sizeof(header));
    if (header.flag == OVER_ACK && vericksum((u_short*)&header, sizeof(header)) == 0) {
        cout << "���������Ϣ���ͳɹ�....��лʹ��" << endl;
        return 1;
    }
    else {
        cout << "���ݰ�����....���ڵȴ��ش�" << endl;
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
        cout << "��һ�λ��ַ���ʧ��" << endl;
        return -1;
    }
WAIT2:
    clock_t start = clock();
    while (recvfrom(client, recvbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {
        if (clock() - start > MAX_TIME) {
            if (sendto(client, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
                cout << "��һ�λ��ַ���ʧ��" << endl;
                return -1;
            }
            start = clock();
            cout << "��һ�λ�����Ϣ������ʱ�����ط���һ�λ���" << endl;
        }
    }
    memcpy(&header, recvbuffer, sizeof(header));
    if (header.flag == ACK && vericksum((u_short*)&header, sizeof(header)) == 0) {
        cout << "�յ��ڶ��λ�����Ϣ" << endl;
    }
    else {
        cout << "�ڶ��λ�����Ϣ����ʧ��...." << endl;
        goto WAIT2;
    }
WAIT3:
    while(recvfrom(client, recvbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0){}
    memcpy(&header, recvbuffer, sizeof(header));
    if (header.flag == FIN_ACK && vericksum((u_short*)&header, sizeof(header)) == 0) {
        cout << "�յ������λ�����Ϣ" << endl;
    }
    else {
        cout << "�����λ�����Ϣ����ʧ��" << endl;
        goto WAIT3;
    }
    header.seq = 1;
    header.flag = ACK;
    header.checksum = calcksum((u_short*)&header, sizeof(header));
    memcpy(sendbuffer, &header, sizeof(header));
SENDWAVE4:
    if (sendto(client, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
        cout << "���Ĵλ��ַ���ʧ��" << endl;
        return -1;
    }
    start = clock();
    ioctlsocket(client, FIONBIO, &unlockmode);
    while (recvfrom(client, recvbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {
        if (clock() - start > 2 * MAX_TIME) {
            cout << "���ܷ�����ʱ���ط����Ĵλ���" << endl;
            goto SENDWAVE4;
        }
    }
    memcpy(&header, recvbuffer, sizeof(header));
    if (header.flag == FINAL_CHECK && vericksum((u_short*)&header, sizeof(header)) == 0) {
        cout << "�Ĵλ������....�����Ͽ�����" << endl;
        return 1;
    }
    else {
        cout << "���ݰ�����,׼���ط����Ĵ�����" << endl;
        goto SENDWAVE4;
    }

}