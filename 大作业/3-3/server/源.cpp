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
const int WINDOWSIZE = 4;//�������ڵĴ�СΪ4
const int SEQSIZE = INT_MAX;//���кŵĴ�СΪ9(0-8)
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
const unsigned char FINAL_CHECK = 0x20;//FC=1.FIN=0,OVER=0,FIN=0,ACK=0,SYN=0
const double MAX_TIME = 0.1 * CLOCKS_PER_SEC;
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


bool canLoad = false;//�Ƿ��ܹ������ļ�������
int SEQWanted = 0;//������Ҫ�յ���
bool canSend = false;//�ܲ��ܷ�������SEQWanted��ACK��Ϣ
bool canExit = false;//�����ܲ����˳�
//ʹ�û�����������һЩȫ�ֱ���
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
        while (recvfrom(server, recvshbuffer, sizeof(header), 0, (sockaddr*)&router_addr, &rlen) <= 0) {
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
    string filename = "1.png";
    ofstream fout(filename.c_str(), ofstream::binary);
    for (int i = 0; i < messagepointer; i++)
    {
        fout << message[i];
    }
    fout.close();
    cout << "[FINISH]�ļ��ѳɹ����ص�����" << endl;
    return 0;
}


DWORD WINAPI Recvprocess(LPVOID p) {
    cout << "successfully created RecvProcess" << endl;
    Header header;
    char* recvbuffer = new char[sizeof(header) + MAX_DATA_LENGTH];
    char* sendbuffer = new char[sizeof(header)];
    int nowpointer=0;
    clock_t c=clock();//���
    while (true) {
        while (recvfrom(server, recvbuffer, sizeof(header) + MAX_DATA_LENGTH, 0, (sockaddr*)&client_addr, &rlen) > 0) {
            c = clock();
            memcpy(&header, recvbuffer, sizeof(header));
            //�ж�У���
            if (vericksum((u_short*)recvbuffer, sizeof(header) + MAX_DATA_LENGTH) != 0) {
                cout << "У��ʹ���" << endl;
                continue;
            }
            //�յ����˳�����
            if (header.flag == OVER) {
                //Ҫ����ȫ�ֱ����ˣ���Ҫ����
                mtx.lock();
                messagepointer = nowpointer - 1;
                canExit = true;
                canLoad = true;
                mtx.unlock();
                return 1;
            }
            //Ҫ��ȫ�ֱ�������������
            mtx.lock();
            //�ɹ����������ݰ�������Ҫ�����ݰ�
            if (header.seq == SEQWanted&&!canSend) {
                cout << "�ɹ�����" << header.seq << "�����ݰ�" << endl;
                canSend = true;
                memcpy(message + nowpointer, recvbuffer + sizeof(header), header.length);
                nowpointer += header.length;
                mtx.unlock();
            }
            else {
                //��������Ϊ�߳�ûͬ����ȷ
                //Ҳ�п��ܾ��Ǵ�����
                cout << "�������" << header.seq << "�����ݰ���������Ҫ����" << SEQWanted << "�����ݰ�" << endl;
                mtx.unlock();
                /*               header.ack = SEQWanted - 1;
                header.checksum = calcksum((u_short*)&header, sizeof(header));
                memcpy(sendbuffer, &header, sizeof(header));
                sendto(server, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen);
                cout << "�ɹ�����" << header.ack << "��ACK" << endl;*/
            }
        }
        mtx.lock();
        //��ʱ��û���յ��ͻ��˵���Ϣ�Ǿ�ֱ���˳�
        if (clock() - c > MAX_TIME&&SEQWanted>0) {
            cout << "[exit] �����Զ��˳�����" << endl;
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
        //�����˳���
        mtx.lock();
        if (canExit) {
            header.ack = 0;
            header.flag = OVER;
            header.checksum= calcksum((u_short*)&header, sizeof(header));
            memcpy(sendbuffer, &header, sizeof(header));
            sendto(server, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen);
            cout << "�ɹ����ͽ����ź�"<<endl;
            mtx.unlock();
            return 1;
        }
        mtx.unlock();
        //����
        mtx.lock();
        //������ڿ��Է���ACK�Ļ�
        if (canSend) {
            header.ack = SEQWanted;
            SEQWanted++;
            if (SEQWanted > SEQSIZE) { SEQWanted = 1; }
            canSend = false;
            header.checksum = calcksum((u_short*)&header, sizeof(header));
            memcpy(sendbuffer, &header, sizeof(header));
            sendto(server, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen);
            cout << "�ɹ�����" << header.ack << "��ACK" << endl;
            c = clock();
            //if����
            mtx.unlock();
            continue;
        }
        else {
            //�����ʱ��û���յ��Լ���Ҫ�����ݰ�
            //�Ͱ���Ҫ�����ݰ�����ȥ
            //��Ϊ���߳��е�ʱ������е���
            //else����
            mtx.unlock();
            //����
            mtx.lock();
            if (clock() - c > MAX_TIME&&SEQWanted>0) {
                header.ack = SEQWanted - 1;
                header.checksum = calcksum((u_short*)&header, sizeof(header));
                memcpy(sendbuffer, &header, sizeof(header));
                sendto(server, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen);
                cout << "�ɹ�����" << header.ack << "��ACK" << endl;
                c = clock();
                //if����
                mtx.unlock();
                continue;
            }
            //else����
            mtx.unlock();
        }
    }
    return 1;
}


//���߳�������Ҳ�Ƿ���
int receivemessage() {
    Header header;
    char* recvbuffer = new char[sizeof(header) + MAX_DATA_LENGTH];
    char* sendbuffer = new char[sizeof(header)];
    int nowpointer = 0;//��һ��Ҫ���յ�
    while (true) {//��������һֱ����
        ioctlsocket(server, FIONBIO, &unblockmode);//����Ϊ������
        while (recvfrom(server, recvbuffer, sizeof(header) + MAX_DATA_LENGTH, 0, (sockaddr*)&router_addr, &rlen) > 0) {//������յ���
            memcpy(&header, recvbuffer, sizeof(header));
            if (header.flag == OVER) { endreceive(); return 1; }
            //cout << header.seq << "  " << nowpointer << "  " << vericksum((u_short*)recvbuffer, sizeof(header) + MAX_DATA_LENGTH) << endl;
            if (header.seq == nowpointer && vericksum((u_short*)recvbuffer, sizeof(header) + MAX_DATA_LENGTH) == 0) {//��ȷ���յ���Ҫ�����ݰ�
                cout << "[ACK]�ɹ��������к�Ϊ" << header.seq << "�����ݱ�,���ڷ���ack" << "��һ�������յ������ݱ����к�Ϊ" << header.seq + 1 << endl;
                memcpy(message + messagepointer, recvbuffer + sizeof(header), header.length);//������������
                messagepointer += header.length;//����λ��ָ��
                header.ack = nowpointer;//��ʾ��������յ���
                if (nowpointer == SEQSIZE) { nowpointer = 0; }
                else { nowpointer++; }//��һ������Ҫ�յ��İ��ĺ�
                header.checksum = calcksum((u_short*)&header, sizeof(header));
                memcpy(sendbuffer, &header, sizeof(header));
                sendto(server, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen);//����ACK
                continue;
            }
            /*
            if (nowpointer == 0) { header.seq = SEQSIZE; }
            else { header.seq = nowpointer-1; }//������һ����
            header.checksum = calcksum((u_short*)&header, sizeof(header));
            memcpy(sendbuffer, &header, sizeof(header));
            sendto(server, sendbuffer, sizeof(header), 0, (sockaddr*)&router_addr, rlen);//����ACK
            cout << "�����ڴ���ack�����ط�" << header.seq << endl;
            */
            cout << "[ERROR]�����ڴ���ack" << endl;
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
    cout << "[FINISH]ȷ����Ϣ���ͳɹ�" << endl;
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