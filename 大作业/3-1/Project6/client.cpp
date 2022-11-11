#include <iostream>
#include <WINSOCK2.h>
#include <time.h>
#include <fstream>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

const int MAXSIZE = 1024;//���仺������󳤶�
const unsigned char SYN = 0x1; //SYN = 1 ACK = 0
const unsigned char ACK = 0x2;//SYN = 0, ACK = 1��FIN = 0
const unsigned char ACK_SYN = 0x3;//SYN = 1, ACK = 1
const unsigned char FIN = 0x4;//FIN = 1 ACK = 0
const unsigned char FIN_ACK = 0x5;
const unsigned char OVER = 0x7;//������־
double MAX_TIME = 0.5 * CLOCKS_PER_SEC;


/*
1.��α�ײ���ӵ�UDP�ϣ�
2.�����ʼʱ����Ҫ��������ֶ�����ģ�
3.������λ����Ϊ16λ��2�ֽڣ�����
4.������16λ������ӣ����������λ���򽫸���16�ֽڵĽ�λ���ֵ�ֵ�ӵ����λ�ϣ�������0xBB5E+0xFCED=0x1 B84B����1�ŵ����λ���õ������0xB84C
5.����������ӵõ��Ľ��Ӧ��Ϊһ��16λ������������ȡ������Եõ������checksum�� */
u_short cksum(u_short* mes, int size) {
    int count = (size + 1) / 2;
    u_short* buf = (u_short*)malloc(size + 1);
    memset(buf, 0, size + 1);
    memcpy(buf, mes, size);
    u_long sum = 0;
    while (count--) {
        sum += *buf++;
        if (sum & 0xffff0000) {
            sum &= 0xffff;
            sum++;
        }
    }
    return ~(sum & 0xffff);
}

struct HEADER
{
    u_short sum = 0;//У��� 16λ
    u_short datasize = 0;//���������ݳ��� 16λ
    unsigned char flag = 0;
    //��λ��ʹ�ú���λ��������FIN ACK SYN 
    unsigned char SEQ = 0;
    //��λ����������кţ�0~255��������mod
    HEADER() {
        sum = 0;//У��� 16λ
        datasize = 0;//���������ݳ��� 16λ
        flag = 0;
        //��λ��ʹ�ú���λ��������FIN ACK SYN 
        SEQ = 0;
    }
};

int Connect(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen)//�������ֽ�������
{
    HEADER header;
    char* Buffer = new char[sizeof(header)];

    u_short sum;

    //���е�һ������
    header.flag = SYN;
    header.sum = 0;//У�����0
    u_short temp = cksum((u_short*)&header, sizeof(header));
    header.sum = temp;//����У���
    memcpy(Buffer, &header, sizeof(header));//���ײ����뻺����
    if (sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen) == -1)
    {
        return -1;
    }
    clock_t start = clock(); //��¼���͵�һ������ʱ��

    u_long mode = 1;
    ioctlsocket(socketClient, FIONBIO, &mode);

    //���յڶ�������
    while (recvfrom(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
    {
        if (clock() - start > MAX_TIME)//��ʱ�����´����һ������
        {
            header.flag = SYN;
            header.sum = 0;//У�����0
            header.sum = cksum((u_short*)&header, sizeof(header));//����У���
            memcpy(Buffer, &header, sizeof(header));//���ײ����뻺����
            sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);
            start = clock();
            cout << "��һ�����ֳ�ʱ�����ڽ����ش�" << endl;
        }
    }


    //����У��ͼ���
    memcpy(&header, Buffer, sizeof(header));
    if (header.flag == ACK && cksum((u_short*)&header, sizeof(header) == 0))
    {
        cout << "�յ��ڶ���������Ϣ" << endl;
    }
    else
    {
        cout << "���ӷ��������������ͻ��ˣ�" << endl;
        return -1;
    }

    //���е���������
    header.flag = ACK_SYN;
    header.sum = 0;
    header.sum = cksum((u_short*)&header, sizeof(header));//����У���
    if (sendto(socketClient, (char*)&header, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen) == -1)
    {
        return -1;//�жϿͻ����Ƿ�򿪣�-1Ϊδ��������ʧ��
    }
    cout << "�������ɹ����ӣ����Է�������" << endl;
    return 1;
}


void send_package(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen, char* message, int len, int& order)
{
    HEADER header;
    char* buffer = new char[MAXSIZE + sizeof(header)];
    header.datasize = len;
    header.SEQ = unsigned char(order);//���к�
    memcpy(buffer, &header, sizeof(header));
    memcpy(buffer + sizeof(header), message, sizeof(header) + len);
    u_short check = cksum((u_short*)buffer, sizeof(header) + len);//����У���
    header.sum = check;
    memcpy(buffer, &header, sizeof(header));
    sendto(socketClient, buffer, len + sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);//����
    cout << "Send message " << len << " bytes!" << " flag:" << int(header.flag) << " SEQ:" << int(header.SEQ) << " SUM:" << int(header.sum) << endl;
    clock_t start = clock();//��¼����ʱ��
    //����ack����Ϣ
    while (1 == 1)
    {
        u_long mode = 1;
        ioctlsocket(socketClient, FIONBIO, &mode);
        while (recvfrom(socketClient, buffer, MAXSIZE, 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
        {
            if (clock() - start > MAX_TIME)
            {
                header.datasize = len;
                header.SEQ = u_char(order);//���к�
                header.flag = u_char(0x0);
                memcpy(buffer, &header, sizeof(header));
                memcpy(buffer + sizeof(header), message, sizeof(header) + len);
                u_short check = cksum((u_short*)buffer, sizeof(header) + len);//����У���
                header.sum = check;
                memcpy(buffer, &header, sizeof(header));
                sendto(socketClient, buffer, len + sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);//����
                cout << "TIME OUT! ReSend message " << len << " bytes! Flag:" << int(header.flag) << " SEQ:" << int(header.SEQ) << endl;
                clock_t start = clock();//��¼����ʱ��
            }
        }
        memcpy(&header, buffer, sizeof(header));//���������յ���Ϣ����ȡ
        u_short check = cksum((u_short*)&header, sizeof(header));
        if (header.SEQ == u_short(order) && header.flag == ACK)
        {
            cout << "Send has been confirmed! Flag:" << int(header.flag) << " SEQ:" << int(header.SEQ) << endl;
            break;
        }
        else
        {
            continue;
        }
    }
    u_long mode = 0;
    ioctlsocket(socketClient, FIONBIO, &mode);//�Ļ�����ģʽ
}

void send(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen, char* message, int len)
{
    int packagenum = len / MAXSIZE + (len % MAXSIZE != 0);
    int seqnum = 0;
    for (int i = 0; i < packagenum; i++)
    {
        send_package(socketClient, servAddr, servAddrlen, message + i * MAXSIZE, i == packagenum - 1 ? len - (packagenum - 1) * MAXSIZE : MAXSIZE, seqnum);
        seqnum++;
        if (seqnum > 255)
        {
            seqnum = seqnum - 256;
        }
    }
    //���ͽ�����Ϣ
    HEADER header;
    char* Buffer = new char[sizeof(header)];
    header.flag = OVER;
    header.sum = 0;
    u_short temp = cksum((u_short*)&header, sizeof(header));
    header.sum = temp;
    memcpy(Buffer, &header, sizeof(header));
    sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);
    cout << "Send End!" << endl;
    clock_t start = clock();
    while (1 == 1)
    {
        u_long mode = 1;
        ioctlsocket(socketClient, FIONBIO, &mode);
        while (recvfrom(socketClient, Buffer, MAXSIZE, 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
        {
            if (clock() - start > MAX_TIME)
            {
                char* Buffer = new char[sizeof(header)];
                header.flag = OVER;
                header.sum = 0;
                u_short temp = cksum((u_short*)&header, sizeof(header));
                header.sum = temp;
                memcpy(Buffer, &header, sizeof(header));
                sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);
                cout << "Time Out! ReSend End!" << endl;
                start = clock();
            }
        }
        memcpy(&header, Buffer, sizeof(header));//���������յ���Ϣ����ȡ
        u_short check = cksum((u_short*)&header, sizeof(header));
        if (header.flag == OVER)
        {
            cout << "�Է��ѳɹ������ļ�!" << endl;
            break;
        }
        else
        {
            continue;
        }
    }
    u_long mode = 0;
    ioctlsocket(socketClient, FIONBIO, &mode);//�Ļ�����ģʽ
}



int disConnect(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen)
{
    HEADER header;
    char* Buffer = new char[sizeof(header)];

    u_short sum;

    //���е�һ������
    header.flag = FIN;
    header.sum = 0;//У�����0
    u_short temp = cksum((u_short*)&header, sizeof(header));
    header.sum = temp;//����У���
    memcpy(Buffer, &header, sizeof(header));//���ײ����뻺����
    if (sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen) == -1)
    {
        return -1;
    }
    clock_t start = clock(); //��¼���͵�һ�λ���ʱ��

    u_long mode = 1;
    ioctlsocket(socketClient, FIONBIO, &mode);

    //���յڶ��λ���
    while (recvfrom(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
    {
        if (clock() - start > MAX_TIME)//��ʱ�����´����һ�λ���
        {
            header.flag = FIN;
            header.sum = 0;//У�����0
            header.sum = cksum((u_short*)&header, sizeof(header));//����У���
            memcpy(Buffer, &header, sizeof(header));//���ײ����뻺����
            sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);
            start = clock();
            cout << "��һ�λ��ֳ�ʱ�����ڽ����ش�" << endl;
        }
    }


    //����У��ͼ���
    memcpy(&header, Buffer, sizeof(header));
    if (header.flag == ACK && cksum((u_short*)&header, sizeof(header) == 0))
    {
        cout << "�յ��ڶ��λ�����Ϣ" << endl;
    }
    else
    {
        cout << "���ӷ������󣬳���ֱ���˳���" << endl;
        return -1;
    }

    //���е����λ���
    header.flag = FIN_ACK;
    header.sum = 0;
    header.sum = cksum((u_short*)&header, sizeof(header));//����У���
    if (sendto(socketClient, (char*)&header, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen) == -1)
    {
        return -1;
    }

    start = clock();
    //���յ��Ĵλ���
    while (recvfrom(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
    {
        if (clock() - start > MAX_TIME)//��ʱ�����´�������λ���
        {
            header.flag = FIN;
            header.sum = 0;//У�����0
            header.sum = cksum((u_short*)&header, sizeof(header));//����У���
            memcpy(Buffer, &header, sizeof(header));//���ײ����뻺����
            sendto(socketClient, Buffer, sizeof(header), 0, (sockaddr*)&servAddr, servAddrlen);
            start = clock();
            cout << "���Ĵ����ֳ�ʱ�����ڽ����ش�" << endl;
        }
    }
    cout << "�Ĵλ��ֽ��������ӶϿ���" << endl;
    return 1;
}


int main()
{
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);

    SOCKADDR_IN server_addr;
    SOCKET server;

    server_addr.sin_family = AF_INET;//ʹ��IPV4
    server_addr.sin_port = htons(2456);
    server_addr.sin_addr.s_addr = htonl(2130706433);

    server = socket(AF_INET, SOCK_DGRAM, 0);
    int len = sizeof(server_addr);
    //��������
    if (Connect(server, server_addr, len) == -1)
    {
        return 0;
    }

    string filename;
    cout << "�������ļ�����" << endl;
    cin >> filename;
    ifstream fin(filename.c_str(), ifstream::binary);//�Զ����Ʒ�ʽ���ļ�
    char* buffer = new char[10000000];
    int index = 0;
    unsigned char temp = fin.get();
    while (fin)
    {
        buffer[index++] = temp;
        temp = fin.get();
    }
    fin.close();
    send(server, server_addr, len, (char*)(filename.c_str()), filename.length());
    clock_t start = clock();
    send(server, server_addr, len, buffer, index);
    clock_t end = clock();
    cout << "������ʱ��Ϊ:" << (end - start) / CLOCKS_PER_SEC << "s" << endl;
    cout << "������Ϊ:" << ((float)index) / ((end - start) / CLOCKS_PER_SEC) << "byte/s" << endl;
    disConnect(server, server_addr, len);
}

// ���г���: Ctrl + F5 ����� >����ʼִ��(������)���˵�
// ���Գ���: F5 ����� >����ʼ���ԡ��˵�

// ����ʹ�ü���: 
//   1. ʹ�ý��������Դ�������������/�����ļ�
//   2. ʹ���Ŷ���Դ�������������ӵ�Դ�������
//   3. ʹ��������ڲ鿴���������������Ϣ
//   4. ʹ�ô����б��ڲ鿴����
//   5. ת������Ŀ��>���������Դ����µĴ����ļ�����ת������Ŀ��>�����������Խ����д����ļ���ӵ���Ŀ
//   6. ��������Ҫ�ٴδ򿪴���Ŀ����ת�����ļ���>���򿪡�>����Ŀ����ѡ�� .sln �ļ�
