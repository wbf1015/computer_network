#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#include<iostream>
using namespace std;
int main1() {
	char* message = new char[1000];
	char c[4] = { 'a','b','c','\0'};
	strcpy(message, c);
	char d[4] = { 'e','f','g','\0' };
	strcpy(message + 2, d);
	cout << message << endl;
	cout << sizeof(message) << endl;
	return 0;
}