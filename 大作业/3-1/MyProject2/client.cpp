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
u_long mode = 1;
const unsigned char MAX_DATA_LENGTH = 0xff;
const u_short SOURCEIP = 0x7f01;
const u_short DESIP = 0x7f01;
const u_short SOURCEPORT = 8887;//�ͻ��˶˿���8887
const u_short DESPORT = 8888;//·�����˿ں���8886
const unsigned char SYN = 0x1;//FIN=0,ACK=0,SYN=1
const unsigned char ACK = 0x2;//FIN=0,ACK=1,SYN=0
const unsigned char SYN_ACK = 0x3;//FIN=0,ACK=1,SYN=1
const unsigned char OVER = 0x8;//OVER=1,FIN=0,ACK=0,SYN=0
const unsigned char OVER_ACK = 0xA;//OVER=1,FIN=0,ACK=1,SYN=0
const double MAX_TIME = CLOCKS_PER_SEC;
//����ͷ
struct Header {
    unsigned char seq; //8λ���к�,��Ϊ��ͣ�ȣ�����ֻ�����λʵ����ֻ��0��1����״̬
    unsigned char ack; //8λack�ţ���Ϊ��ͣ�ȣ�����ֻ�����λʵ����ֻ��0��1����״̬
    unsigned char empty;//8λ��λ
    unsigned char flag;//8λ״̬λ ������һλSYN,�����ڶ�λACK����������λFIN
    u_short checksum; //16λУ���
    unsigned char empty2;//8λ��λ
    unsigned char length;//8λ����λ
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
        empty = 0;
        length = 0;
        empty2 = 0;
    }
};

//sizeof�����ڴ��ֽ��� ushort��16λ2�ֽڣ�������Ҫ��size����ȡ��
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
int endsend();
int loadMessage();

int main() {
    initialNeed();
    if (tryToConnect() <= 0) {
        cout << "����ʧ�ܣ��������Ӻ�����" << endl;
        return -1;
    }
    cout << "���ֳɹ������Խ������ݴ��䣡...";



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

    client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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
    header.empty = 0;
    header.empty2 = 0;
    header.checksum = calcksum((u_short*)&header, sizeof(header));
    //cout << vericksum((u_short*)&header, sizeof(header)) << endl;
    u_short* test = (u_short*)&header;
    memcpy(sendshbuffer, &header, sizeof(header));
    if (sendto(client, sendshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
        cout << "��һ������������ʧ��..." << endl;
        return -1;
    }
    cout << "��һ��������Ϣ���ͳɹ�...." << endl;
    //�����Ƿ�Ϊ������ģʽ
    ioctlsocket(client, FIONBIO, &mode);

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
        }
    }

    //����ڶ���������Ϣ�Ƿ�׼ȷ
    memcpy(&header, recvshbuffer, sizeof(header));
    if (header.flag == SYN_ACK && vericksum((u_short*)&header, sizeof(header)) == 0) {
        cout << "��ȷ���ܵڶ���������Ϣ" << endl;
    }
    else {
        cout << "�����ڴ��ķ�������ݰ�,�����ش���һ���������ݰ�...." << endl;
        goto FIRSTSHAKE;
    }

    //���͵�����������Ϣ
    header.source_port = SOURCEPORT;
    header.des_port = DESPORT;
    header.flag = ACK;
    header.seq = 1;
    header.ack = 1;
    header.checksum = calcksum((u_short*)&header, sizeof(header));
    memcpy(sendshbuffer, &header, sizeof(header));
    if (sendto(client, sendshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
        cout << "������������Ϣ����ʧ�ܣ����Ժ�����...." << endl;
        return -1;
    }
    cout << "������������ɹ���׼����������" << endl;
    return 1;
}

int send() {
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
        memcpy(sendbuffer + sizeof(header), message + messagepointer, ml);//������������
        messagepointer += ml;//��������ָ��
        header.checksum = calcksum((u_short*)sendbuffer, sizeof(header) + MAX_DATA_LENGTH);//����У���
        memcpy(sendbuffer, &header, sizeof(header));//���У���
    SEQ0SEND:
        //����seq=0�����ݰ�
        if (sendto(client, sendbuffer, sizeof(header) + MAX_DATA_LENGTH, 0, (sockaddr*)&router_addr, rlen) == -1) {
            cout << "����ʧ��....����ԭ��" << endl;
            return -1;
        }
        clock_t start = clock();
    SEQ0RECV:
        //����յ������˾Ͳ����ˣ�������ʱ�ش�
        while (recvfrom(client, recvbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {
            if (clock() - start > MAX_TIME) {
                if (sendto(client, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
                    cout << "SEQ=0����Ϣ����ʧ��....����ԭ��" << endl;
                    return -1;
                }
                start = clock();
                cout << "SEQ=0����Ϣ������ʱ....�����ش�" << endl;
                //goto SEQ0SEND;
            }
        }
        //���ackλ�Ƿ���ȷ�������ȷ��׼������һ�����ݰ�
        memcpy(&header, recvbuffer, sizeof(header));
        if (header.ack == 1 && vericksum((u_short*)&header, sizeof(header) == 0)){
            cout << "seq=0�����ݰ��ɹ����ܷ����ACK��׼��������һ�����ݰ�" << endl;
        }
        else {
            cout << "�����ڴ������ݰ���׼���ط�SEQ=0�����ݰ�" << endl;
            goto SEQ0RECV;
        }

        //׼����ʼ��SEQ=1�����ݰ�
        int ml;//�������ݴ��䳤��
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
    SEQ1SEND:
        //����seq=1�����ݰ�
        if (sendto(client, sendbuffer, sizeof(header) + MAX_DATA_LENGTH, 0, (sockaddr*)&router_addr, rlen) == -1) {
            cout << "����ʧ��....����ԭ��" << endl;
            return -1;
        }
        start = clock();
    SEQ1RECV:
            //����յ������˾Ͳ����ˣ�������ʱ�ش�
            while (recvfrom(client, recvbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {
                if (clock() - start > MAX_TIME) {
                    if (sendto(client, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
                        cout << "SEQ=1����Ϣ����ʧ��....����ԭ��" << endl;
                        return -1;
                    }
                    start = clock();
                    cout << "SEQ=1����Ϣ������ʱ....�����ش�" << endl;
                    //goto SEQ1SEND;
                }
            }
        //���ackλ�Ƿ���ȷ�������ȷ��׼������һ�����ݰ�
        memcpy(&header, recvbuffer, sizeof(header));
        if (header.ack == 0 && vericksum((u_short*)&header, sizeof(header)) == 0) {
            cout << "seq=1�����ݰ��ɹ����ܷ����ACK��׼��������һ�����ݰ�" << endl;
        }
        else {
            cout << "�����ڴ������ݰ���׼���ط�SEQ=0�����ݰ�" << endl;
            goto SEQ1RECV;
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

int loadMessage() {
    string filename;
    cout << "�������ļ�����" << endl;
    cin >> filename;
    ifstream fin(filename.c_str(), ifstream::binary);//�Զ����Ʒ�ʽ���ļ�
    int index = 0;
    unsigned char temp = fin.get();
    while (fin)
    {
        message[index++] = temp;
        temp = fin.get();
    }
    fin.close();
}