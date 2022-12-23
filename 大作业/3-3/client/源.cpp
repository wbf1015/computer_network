#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#include<iostream>
#include<WinSock2.h>
#include<time.h>
#include<fstream>
#include<iostream>
#include<string>
#include<map>
#include<thread>
#include<vector>
#include<set>
#include<mutex>
#pragma comment(lib,"ws2_32.lib")
using namespace std;
//��ʼ��dll
WSADATA wsadata;

//�����׽�����Ҫ�󶨵ĵ�ַ
SOCKADDR_IN server_addr;
//����·������Ҫ�󶨵ĵ�ַ
SOCKADDR_IN router_addr;
//�����ͻ��˵ĵ�ַ
SOCKADDR_IN client_addr;
SOCKET client;

char* message = new char[100000000];
char* sbuffer = new char[1000];
char* rbuffer = new char[1000];
unsigned long long int messagelength = 0;//���һ��Ҫ�����±��Ƕ���
unsigned long long int messagepointer = 0;//��һ���ô���λ��

int slen = sizeof(server_addr);
int rlen = sizeof(router_addr);


//���������3-3���õ�
//�����һ�¾��ǻ���һ�����кŴ���256�ֽ�
int MSS = 256;//������256Ϊ��λ
int WINDOWSIZE = 1;//��ʼ��ʱ�򴰿���1
int ssthresh = 8; //�����ڵ�8��ʱ�����ӵ������׶�
const int MAX_WINDOWSIZE = 1024;//��Ϊ���ܹ����к�ֻ��1024�����ڴ�С�����ٴ���
const int SEQSIZE = INT_MAX;//���кŸ��㿪��1024
int stage = 1;//�����׶Σ�1���������� 2����ӵ������ 3������ٻָ�
map<int, bool>ifRecv;//�ж��Ƿ��յ�
map<int, unsigned long long int>startmap;//�����洢��Ӧ�����к���message����ʼλ�ã�����ÿ�����Ĵ�С����256
map<int, int>lengthmap;//�����洢��Ӧ�����кŵ�Ҫת����data��length
clock_t c;//��ʱ�ش��õ�

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
const int MAX_TIME = 0.5 * CLOCKS_PER_SEC; //������ӳ�ʱ��
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
//ȫ��ʱ������
clock_t linkClock;
clock_t ALLSTART;
clock_t ALLEND;


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
DWORD WINAPI Recvprocess(LPVOID p);
DWORD WINAPI Sendprocess(LPVOID p);
int WINDOWStart = 0;//���ڵĳ�ʼָ��
int WINDOWNow = 0;//��һ��Ӧ�÷��Ĵ���ָ��
int canexit = 0;
mutex mtx;//����һ����������ȫ�ֱ���


int main() {
    initialNeed();
    tryToConnect();
    loadMessage();
    HANDLE cthread = CreateThread(NULL, 0, Recvprocess, (LPVOID)nullptr, 0, NULL);
    CloseHandle(cthread);
    HANDLE ccthread = CreateThread(NULL, 0, Sendprocess, (LPVOID)nullptr, 0, NULL);
    CloseHandle(ccthread);
    ALLSTART = clock();
    while (true) {
        if (canexit == 1) {
            ALLEND = clock();
            break;
        }
    }
    printlog();
    while (true) {

    }
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
    cout << "[PREPARE]��ʼ���������" << endl;
}
int tryToConnect() {
    linkClock = clock();
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
    //cout << vericksum((u_short*)&header, sizeof(header)) << endl;
    u_short* test = (u_short*)&header;
    memcpy(sendshbuffer, &header, sizeof(header));
SEND1:
    if (sendto(client, sendshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
        cout << "[failed]��һ������������ʧ��..." << endl;
        return -1;
    }
    cout << "[1]��һ��������Ϣ���ͳɹ�...." << endl;
    //�����Ƿ�Ϊ������ģʽ
    ioctlsocket(client, FIONBIO, &unlockmode);

FIRSTSHAKE:
    //���ü�ʱ��
    clock_t start = clock();

    //��һ�������ش�
    while (recvfrom(client, recvshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {
        if (clock() - linkClock > 75 * CLOCKS_PER_SEC) {
            cout << "[failed]���ӳ�ʱ,�������Զ��Ͽ�" << endl;
            return -1;
        }
        if (clock() - start > MAX_TIME) {
            if (sendto(client, sendshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
                cout << "[failed]��һ������������ʧ��..." << endl;
                return -1;
            }
            start = clock();
            cout << "[1]��һ��������Ϣ������ʱ....�������·���" << endl;
            goto SEND1;
        }
    }

    //����ڶ���������Ϣ�Ƿ�׼ȷ
    memcpy(&header, recvshbuffer, sizeof(header));
    if (header.flag == SYN_ACK && vericksum((u_short*)&header, sizeof(header)) == 0) {
        cout << "[2]��ȷ���ܵڶ���������Ϣ" << endl;
    }
    else {
        cout << "[1]�����ڴ��ķ�������ݰ�,�����ش���һ���������ݰ�...." << endl;
        if (clock() - linkClock > 75 * CLOCKS_PER_SEC) {
            cout << "[failed]���ӳ�ʱ,�������Զ��Ͽ�" << endl;
            return -1;
        }
        goto SEND1;
    }


    //���͵�����������Ϣ
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
        cout << "[failed]������������Ϣ����ʧ�ܣ����Ժ�����...." << endl;
        return -1;
    }
    start = clock();
    while (recvfrom(client, recvshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {
        if (clock() - linkClock > 75 * CLOCKS_PER_SEC) {
            cout << "[failed]���ӳ�ʱ,�������Զ��Ͽ�" << endl;
            return -1;
        }
        if (clock() - start >= 5 * MAX_TIME) {
            cout << "[failed]������������Ϣ������ʱ...�������·���" << endl;
            if (clock() - linkClock > 75 * CLOCKS_PER_SEC) {
                cout << "[failed]���ӳ�ʱ,�������Զ��Ͽ�" << endl;
                return -1;
            }
            goto SEND3;
        }
    }
    memcpy(&header, recvshbuffer, sizeof(header));
    if (header.flag == ACK && vericksum((u_short*)&header, sizeof(header)) == 0) {
        cout << "[3]��ȷ���͵�����������Ϣ" << endl;
        cout << "[EVERYTHING_DONE]������������ɹ���׼����������" << endl;
        return 1;
    }
    else {
        cout << "[ERROR]ȷ����Ϣ����ʧ��...�ط�����������" << endl;
        goto SEND3;
    }

}
int loadMessage() {
    string filename;
    cout << "[INPUT]������Ҫ������ļ���" << endl;
    cin >> filename;
    ifstream fin(filename.c_str(), ifstream::binary);//�Զ����Ʒ�ʽ���ļ�
    unsigned long long int index = 0;
    unsigned char temp = fin.get();
    while (fin)
    {
        message[index++] = temp;
        temp = fin.get();
    }
    messagelength = index - 1;
    fin.close();
    return 0;
}
void printlog() {
    cout << "             --------------������־--------------" << endl;
    cout << "���α����ܳ���Ϊ " << messagelength << "�ֽڣ�����Ϊ " << (messagelength / 256) + 1 << "�����Ķηֱ�ת��" << endl;
    double t = (ALLEND - ALLSTART) / CLOCKS_PER_SEC;
    cout << "���δ������ʱ��Ϊ" << t << "��" << endl;
    t = messagelength / t;
    cout << "���δ���������Ϊ" << t << "�ֽ�ÿ��" << endl;
}
//������Ҫ��Ҫ�����������̺߳���
// 
//һ��������ack һ�����𷢰�
DWORD WINAPI Recvprocess(LPVOID p) {
    cout << "successfully created RecvProcess" << endl;
    Header header;
    char* recvbuffer = new char[sizeof(header)];
    int dupACKcount = 0;
    set<int> cache;
    //�Ҹо��������ƺ��������÷�����
    while (true) {
        while (recvfrom(client, recvbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) > 0) {
            //�յ���server����Ϣ
            memcpy(&header, recvbuffer, sizeof(header));
            cout << "[RECV] �յ�" << header.ack << "��ȷ�ϵ����ݰ�" << endl;
            //У��Ͳ���ȷ����
            if (vericksum((u_short*)&header, sizeof(header)) != 0) {
                cout << "[erro] " << header.ack << "�����ݰ���" << endl;
                continue;
            }
            //���ﻹ��̫ȷ��,����cache��ʹ��
            //���ﻹҪ�ٸ�һ��
            //�൱�ڻ��ڷ����Ѿ����µ���Щ�����Զ����ε�
            if (stage!=3&&cache.find(header.ack)!=cache.end()) {
                //��������������ACK���·�������stage3
                //ֻ��Ҫ���������͵�λ�ã�û��Ҫ����stage��
                /*mtx.lock();
                WINDOWStart = header.ack + 1;
                WINDOWNow = WINDOWStart;
                WINDOWSIZE = 1;
                mtx.unlock();*/
                break;
            }
            if (header.flag == OVER) {
                mtx.lock();
                cout << "[RECV] �յ��˳�֪ͨ�������˳�" << endl;
                canexit = 1;
                mtx.unlock();
                return 1; 
            }
            //������ ack����ȷ ֱ�Ӽ��ظ���
            if (header.ack != WINDOWStart) {
                //int ml;
                //if ((header.ack+ 1) * MAX_DATA_LENGTH > messagelength) {
                //    ml = messagelength - (WINDOWStart)*MAX_DATA_LENGTH;
                //}
                //else { ml = MAX_DATA_LENGTH; }
                //header.length = ml;
                //header.seq = header.ack+1;
                //memset(sendBuffer, 0, sizeof(header) + MAX_DATA_LENGTH);
                //memcpy(sendBuffer, &header, sizeof(header));
                //memcpy(sendBuffer + sizeof(header), message + ((header.ack+1) * MAX_DATA_LENGTH), header.length);
                //header.checksum = calcksum((u_short*)sendBuffer, sizeof(header) + MAX_DATA_LENGTH);
                //memcpy(sendBuffer, &header, sizeof(header));
                ////��Ϊ���ط� û��Ҫ��������map Ҳ���ø���messagepointer
                //sendto(client, sendBuffer, sizeof(header) + MAX_DATA_LENGTH, 0, (sockaddr*)&router_addr, rlen);
                //cout << "[send] ���" << header.ack << "�����ݰ����ش�" << endl;
                //����ȣ���ʵһ���ܺõĲ��Ծ����������ܻ���
                if (header.ack > WINDOWStart) {
                    //�൱���������Ҷ��յ��ˣ������ʱ��������ʰ��ʼ
                    //�������ʱ���ó�ֵduplicateACK����Ϊ������������������Ϊ���̵߳��µ�
                    //ʵ���Ϸ�����Ѿ��յ����ˣ�������ֻ��Ҫ������Ҫ���ĸĳɷ������Ҫ�ľ�����
                    mtx.lock();
                    WINDOWStart = header.ack+1;
                    WINDOWNow = WINDOWStart;
                    WINDOWSIZE = 1;
                    mtx.unlock();
                }
                else {
                    mtx.lock();
                    //������������Ķ�����
                    for (int i = header.ack + 1; i <= WINDOWNow;i++) {
                        ifRecv[i] = false;
                    }
                    //����Ҫ�������λ
                    WINDOWStart = header.ack + 1;
                    WINDOWNow = WINDOWStart;
                    WINDOWSIZE = 1;
                    //��ʽ����stage3
                    if (++dupACKcount > 3) { 
                        dupACKcount = 3;
                        ssthresh = WINDOWSIZE / 2;
                        if (ssthresh < 2) { ssthresh = 2; }
                        WINDOWSIZE = ssthresh + (3 * MSS / MAX_DATA_LENGTH);
                        cout << "[stage] ������ٻָ��׶Σ����ڴ�С����Ϊ" << WINDOWSIZE << "��������ʼΪ"<<WINDOWStart << endl;
                    }
                    cache.insert(header.ack);
                    mtx.unlock();
                }
            }
            else {
                //ACK����ȷ��
                mtx.lock();
                ifRecv[header.ack] = true;
                cout << "[RECV]�ɹ�����" << header.ack << "�����ݰ���ACK" << endl;
                mtx.unlock();
                //�ӿ����ش��ָ�����
                if (stage == 3) {
                    mtx.lock();
                    dupACKcount = 0;
                    WINDOWSIZE = ssthresh;//�����ڿ�ʼ�ͻ��ǽ���ӵ������׶�
                    stage = 2;
                    cout << "[stage] ����ӵ������׶�" << endl;
                    cout << "[WINDOW] ���ڵĴ�����ʼ�ǣ�" << WINDOWStart << "���ڴ�С��:" << WINDOWSIZE << endl;
                    mtx.unlock();
                    continue;
                }
                if (stage == 1) {
                    //�൱�ڷ���
                    mtx.lock();
                    WINDOWSIZE = (2 * WINDOWSIZE * MSS) / MAX_DATA_LENGTH;
                    if (WINDOWSIZE >= ssthresh) { 
                        WINDOWSIZE = ssthresh; 
                        stage = 2;
                        cout << "[stage] ����ӵ������׶�" << endl;
                    }
                    if (WINDOWSIZE >= MAX_WINDOWSIZE) { WINDOWSIZE = MAX_WINDOWSIZE; }
                    cout << "[WINDOW] ���ڵĴ�����ʼ�ǣ�" << WINDOWStart << "���ڴ�С��:" << WINDOWSIZE << endl;
                    mtx.unlock();
                    continue;
                }
                if (stage == 2) {
                    //�൱�ڼ�һ
                    mtx.lock();
                    WINDOWSIZE = (1 + WINDOWSIZE) * MSS / MAX_DATA_LENGTH;
                    if (WINDOWSIZE >= MAX_WINDOWSIZE) { WINDOWSIZE = MAX_WINDOWSIZE; }
                    cout << "[WINDOW] ���ڵĴ�����ʼ�ǣ�" << WINDOWStart << "���ڴ�С��:" << WINDOWSIZE << endl;
                    //���ⲹ��һ�°ɣ�Ҫ��һֱ��̫������
                    ssthresh++;
                    mtx.unlock();
                }
            }
        }
    }
}
bool CanSend(int s, int n) {
        if (n - s > WINDOWSIZE) {  
            return false; 
        }
        return true;
}

DWORD WINAPI Sendprocess(LPVOID p) {
    cout << "successfully created SendProcess" << endl;
    Header header;
    char* sendBuffer = new char[sizeof(header)+MAX_DATA_LENGTH];
    c = clock();
    while (true) {
        //���˳����˳�
        mtx.lock();
        if (canexit==1) {
            cout << "[exit] �˳��������" << endl;
            mtx.unlock();
            break;
        }
        mtx.unlock();
        if (stage == 3) { //�����ش��Ĵ���
            //ֻҪ��û�յ����Ҿ�һֱ��
            //�൱������Ͳ���WINDOWNow��������
            mtx.lock();
            header.seq = WINDOWStart;
            memset(sendBuffer, 0, sizeof(header) + MAX_DATA_LENGTH);
            memcpy(sendBuffer, &header, sizeof(header));
            int ml;
            if ((header.seq + 1) * MAX_DATA_LENGTH > messagelength) {
                ml = messagelength - (header.seq) * MAX_DATA_LENGTH;
            }
            else {
                ml = MAX_DATA_LENGTH;
            }
            header.length = ml;
            memcpy(sendBuffer + sizeof(header), message+((header.seq)*MAX_DATA_LENGTH), ml);
            header.checksum = calcksum((u_short*)sendBuffer, sizeof(header) + MAX_DATA_LENGTH);
            memcpy(sendBuffer, &header, sizeof(header));
            //��Ϊ���ط� û��Ҫ��������map Ҳ���ø���messagepointer
            sendto(client, sendBuffer, sizeof(header) + MAX_DATA_LENGTH, 0, (sockaddr*)&router_addr, rlen);
            WINDOWNow = WINDOWStart;
            cout << "[send] �����ش��׶��ش�"<<header.seq<<"�����ݰ�" << endl;
            mtx.unlock();
            continue;
        }
        //��״̬���˳��ĺ���֢
        mtx.lock();
        if (WINDOWNow < WINDOWStart) { WINDOWNow = WINDOWStart; }
        mtx.unlock();
        //���ܲ��ܷ�
        //������
        //������WINDOWNow�����ĵط�
        mtx.lock();
        if (CanSend(WINDOWStart, WINDOWNow)) {//������Է��͵Ļ�
            //���㷢�͵ĳ���
            if (WINDOWNow > messagelength / MAX_DATA_LENGTH) {
                if (ifRecv[messagelength / MAX_DATA_LENGTH] == true) {
                    //���Է�over��
                    header.seq = 0;
                    header.flag = OVER;
                    memset(sendBuffer, 0, sizeof(header) + MAX_DATA_LENGTH);
                    memcpy(sendBuffer, &header, sizeof(header));
                    header.checksum = calcksum((u_short*)sendBuffer, sizeof(header) + MAX_DATA_LENGTH);
                    memcpy(sendBuffer, &header, sizeof(header));
                    //����ת��
                    sendto(client, sendBuffer, sizeof(header) + MAX_DATA_LENGTH, 0, (sockaddr*)&router_addr, rlen);
                    sendto(client, sendBuffer, sizeof(header) + MAX_DATA_LENGTH, 0, (sockaddr*)&router_addr, rlen);
                    cout << "[send] �ɹ����ͽ�������" << endl;
                }
            }
            else {
                int ml;
                if ((WINDOWStart + 1) * MAX_DATA_LENGTH > messagelength) {
                    ml = messagelength - (WINDOWStart)*MAX_DATA_LENGTH;
                }
                else { ml = MAX_DATA_LENGTH; }
                header.length = ml;
                header.seq = WINDOWNow++;//����WindowNow
                memset(sendBuffer, 0, sizeof(header) + MAX_DATA_LENGTH);
                memcpy(sendBuffer, &header, sizeof(header));
                memcpy(sendBuffer + sizeof(header), message + ((header.seq) * MAX_DATA_LENGTH), ml);
                header.checksum = calcksum((u_short*)sendBuffer, sizeof(header) + MAX_DATA_LENGTH);
                memcpy(sendBuffer, &header, sizeof(header));
                sendto(client, sendBuffer, sizeof(header) + MAX_DATA_LENGTH, 0, (sockaddr*)&router_addr, rlen);
                cout << "[send] �ɹ�����" << header.seq << "�����ݰ�" << endl;
            }   
        }
        //���ܲ��ܵ�������
        //������
        //
        mtx.unlock();
        mtx.lock();
        if (ifRecv.find(WINDOWStart) != ifRecv.end()) {
            if (ifRecv[WINDOWStart] == true) {
                ifRecv[WINDOWStart] = false;
                if (WINDOWStart != SEQSIZE) { WINDOWStart++; }
                else { WINDOWStart = 1; }
                c = clock();
                int windowEnd = WINDOWStart + WINDOWSIZE;
                cout << "[WINDOWS] ���ڵĴ��ڴ�СΪ��" << WINDOWStart << " - " << windowEnd << endl;
            }
            //��״̬���˳��ĺ���֢
            if (WINDOWNow < WINDOWStart) { WINDOWNow = WINDOWStart; }
        }
        mtx.unlock();
        //��ʱ ����������״̬
        if (clock() - c > MAX_TIME) {
            mtx.lock();
            WINDOWNow = WINDOWStart+1;//���������һ��
            WINDOWSIZE = (1 * MSS) / MAX_DATA_LENGTH;
            ssthresh /= 2;
            if (ssthresh < 1) { ssthresh = 1; }
            stage = 1;
            //������Ҫ����ش��Ҿ���
            //������
            int ml;
            if ((WINDOWStart + 1) * MAX_DATA_LENGTH > messagelength) {
                ml = messagelength - (WINDOWStart)*MAX_DATA_LENGTH;
            }
            else { ml = MAX_DATA_LENGTH; }
            header.length = ml;
            header.seq = WINDOWStart;
            memset(sendBuffer, 0, sizeof(header) + MAX_DATA_LENGTH);
            memcpy(sendBuffer, &header, sizeof(header));
            memcpy(sendBuffer + sizeof(header), message + (header.seq*MAX_DATA_LENGTH), header.length);
            header.checksum = calcksum((u_short*)sendBuffer, sizeof(header) + MAX_DATA_LENGTH);
            memcpy(sendBuffer, &header, sizeof(header));
            //��Ϊ���ط� û��Ҫ��������map Ҳ���ø���messagepointer
            sendto(client, sendBuffer, sizeof(header) + MAX_DATA_LENGTH, 0, (sockaddr*)&router_addr, rlen);
            cout << "[send] ���" << header.seq << "�����ݰ����ش�" << endl;
            mtx.unlock();
        }
    }
    return 1;
}




//���̵߳Ļ��������ͷ���
int sendmessage() {
    ALLSTART = clock();
    int sWindow = 0;//�������Ҫ��ʼ�İ�����û�յ�ȷ��
    int eWindow = 0 + WINDOWSIZE - 1;//��������кŶ��ǿ��Է���
    int nowpointer = 0;//���Ҫ������Ӧ��ʹ��������к�
    Header header;
    char* recvbuffer = new char[sizeof(header)];
    char* sendbuffer = new char[sizeof(header) + MAX_DATA_LENGTH];
    clock_t start = clock();
    while (true) {//һֱ��ѭ��
        ioctlsocket(client, FIONBIO, &unlockmode);//����Ϊ������
        while (recvfrom(client, recvbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) > 0) {//����յ���
            Header temp;
            memcpy(&temp, recvbuffer, sizeof(header));
            if (temp.ack == sWindow && vericksum((u_short*)recvbuffer, sizeof(header)) == 0) {//�յ���������Ҫ��
                cout << "[ACK]��ȷ����ack��Ϊ" << temp.ack << "��ack" << endl;
                if (sWindow == SEQSIZE) { sWindow = 0; }//�������ڣ���ʼλ�õĴ��ڴӿ�ʼ���µ���
                else { sWindow++; }
                if (eWindow + 1 > SEQSIZE) {//ֻ�е�eWindow=8ʱ�Żᷢ��
                    eWindow = 0;
                }
                else {//����ÿ��eWindowֻ��Ҫ++
                    eWindow++;
                }
                cout << "[WINDOW]Ŀǰ�������ڵ�ֵΪ" << sWindow << "--" << eWindow << endl;
                //��Ϊ�յ���������Ҫ�ģ��������ʱ��һ������ת��һ��������������·��ģ�����Ҫ����һϵ�ж���
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
                header.seq = nowpointer;
                header.length = ml;
                //header.checksum = calcksum((u_short*)&header, sizeof(header));
                memset(sendbuffer, 0, sizeof(header) + MAX_DATA_LENGTH);
                memcpy(sendbuffer, &header, sizeof(header));
                memcpy(sendbuffer + sizeof(header), message + messagepointer, ml);
                messagepointer += ml;
                header.checksum = calcksum((u_short*)sendbuffer, sizeof(header) + MAX_DATA_LENGTH);
                memcpy(sendbuffer, &header, sizeof(header));
                if (nowpointer == SEQSIZE) { nowpointer = 0; }
                else { nowpointer++; }//����nowpointer
                //cout << vericksum((u_short*)sendbuffer, sizeof(header) + MAX_DATA_LENGTH) << endl;//���У���
                sendto(client, sendbuffer, sizeof(header) + MAX_DATA_LENGTH, 0, (sockaddr*)(&router_addr), rlen);
                cout << "[SEND]�ѷ������к�Ϊ" << header.seq << "�����ݱ�" << endl;
                startmap[header.seq] = messagepointer - ml;//���Ҫ�ط�����¼�������message��ƫ��
                lengthmap[header.seq] = ml;//�����Ҫ�ط�����¼�䳤��
                start = clock();//���ͳɹ����������кŵļ�ʱ
            }
            else {//�յ��Ĳ�������Ҫ�ģ�˵�����ܶ��������ˣ����Ҿ��ط����������յ�����С�����к�
                //���ת������Ҫ����messagepointer��windows��Ϊ�����·���
                Header h;
                h.seq = sWindow;
                h.length = lengthmap[sWindow];
                memset(sendbuffer, 0, sizeof(h) + MAX_DATA_LENGTH);
                memcpy(sendbuffer, &h, sizeof(h));
                memcpy(sendbuffer + sizeof(h), message + startmap[h.seq], h.length);
                h.checksum = calcksum((u_short*)&sendbuffer, sizeof(h) + MAX_DATA_LENGTH);
                memcpy(sendbuffer, &h, sizeof(h));
                sendto(client, sendbuffer, sizeof(h) + MAX_DATA_LENGTH, 0, (sockaddr*)(&router_addr), rlen);
                cout << "[ERROR]����ackʧ�ܣ������ش����ݱ�" << h.seq << endl;
                start = clock();
                break;//��ȥ�����ܲ��ܷ��µİ�
            }
        }
        if (nowpointer == eWindow) {//û�յ�������û�ÿɷ���
            if (clock() - start > MAX_TIME) {//��ʱ�ش�
                //���ת������Ҫ����messagepointer��windows��Ϊ�����·���
                int ss = sWindow;
                cout << "[TIMEOUT]��ʱ�ش�����!" << endl;
                while (ss != nowpointer) {//�ط����е����ݱ�
                    Header h;
                    h.seq = ss;
                    h.length = lengthmap[ss];
                    memset(sendbuffer, 0, sizeof(h) + MAX_DATA_LENGTH);
                    memcpy(sendbuffer, &h, sizeof(h));
                    memcpy(sendbuffer + sizeof(h), message + startmap[h.seq], h.length);
                    h.checksum = calcksum((u_short*)sendbuffer, sizeof(h) + MAX_DATA_LENGTH);
                    memcpy(sendbuffer, &h, sizeof(h));
                    //cout << vericksum((u_short*)sendbuffer, sizeof(h) + MAX_DATA_LENGTH);
                    sendto(client, sendbuffer, sizeof(h) + MAX_DATA_LENGTH, 0, (sockaddr*)&router_addr, rlen);
                    cout << "[SEND]�Ѿ�������к�Ϊ" << ss << "�����ݱ����ش�" << endl;
                    if (ss == SEQSIZE) { ss = 0; }
                    else { ss++; }
                }
                start = clock();
            }
        }
        else {//û�յ��������еĿɷ�
            //������ֻ��Ҫ����nowpointer��������Ҫ����windows��Ϊû�յ��Ϸ���ȷ��
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
            Header hh;
            hh.seq = nowpointer;
            hh.length = ml;
            //header.checksum = calcksum((u_short*)&header, sizeof(header));
            memset(sendbuffer, 0, sizeof(hh) + MAX_DATA_LENGTH);
            memcpy(sendbuffer, &hh, sizeof(hh));
            memcpy(sendbuffer + sizeof(hh), message + messagepointer, ml);
            messagepointer += ml;
            hh.checksum = calcksum((u_short*)sendbuffer, sizeof(hh) + MAX_DATA_LENGTH);
            memcpy(sendbuffer, &hh, sizeof(hh));
            //cout << vericksum((u_short*)sendbuffer, sizeof(hh) + MAX_DATA_LENGTH) << endl;
            if (nowpointer == SEQSIZE) { nowpointer = 0; }
            else { nowpointer++; }//����nowpointer�����ǲ��õ�����������
            sendto(client, sendbuffer, sizeof(hh) + MAX_DATA_LENGTH, 0, (sockaddr*)(&router_addr), rlen);
            cout << "[SNED]�ѷ������к�Ϊ" << hh.seq << "�����ݱ�" << endl;
            startmap[hh.seq] = messagepointer - ml;//���Ҫ�ط�����¼�������message��ƫ��
            lengthmap[hh.seq] = ml;
            //���β����ļ�ʱ��Ϊû���յ��µĺϷ���ack
        }
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
    while (recvfrom(client, recvbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {}
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
        cout << "[FAILED]��������źŷ���ʧ��...." << endl;
        return -1;
    }
    cout << "[SENDOK]��������źŷ��ͳɹ�...." << endl;
    clock_t start = clock();
RECV:
    while (recvfrom(client, recvbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {
        if (clock() - start > MAX_TIME) {
            if (sendto(client, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen) == -1) {
                cout << "[FAILED]��������źŷ���ʧ��....����ԭ��" << endl;
                return -1;
            }
            start = clock();
            cout << "[ERROR]��������źŷ�����ʱ....�����ش�" << endl;
            //goto SEND;
        }
    }
    memcpy(&header, recvbuffer, sizeof(header));
    if (header.flag == OVER_ACK && vericksum((u_short*)&header, sizeof(header)) == 0) {
        cout << "[FINFISH]���������Ϣ���ͳɹ�....��лʹ��" << endl;
        return 1;
    }
    else {
        cout << "[ERROR]���ݰ�����....���ڵȴ��ش�" << endl;
        goto RECV;
    }
}