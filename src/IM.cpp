//============================================================================
// Name        : IM.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include "../head/SocketClient.h"
#include <semaphore.h>
#include "string.h"
#include "stdlib.h"
#include "stdio.h"

using namespace std;

#define ServerIP "127.0.0.1"
#define ServerPort 6500

class IM {

public:
	int StartIM();
	static clientStatus status;
	int SendToServer(char * message, unsigned int length);
private:
	SocketClient socketClient;
	static void messageCallback(char *message, unsigned int messLength);
	static void statusCallback(clientStatus currentStatus);
	static char recvMessage[BUFFER_SIZE];
	char EIP[20];
	int EPort;
	static unsigned int recvLength;
	static sem_t newRecvedMessage;
	static pthread_mutex_t serverMessage;
};

clientStatus IM::status;
char IM::recvMessage[BUFFER_SIZE];
unsigned int IM::recvLength;
sem_t IM::newRecvedMessage;
pthread_mutex_t IM::serverMessage;

int IM::StartIM() {
	char mes[200];

	sem_init(&newRecvedMessage, 0, 0);
	serverMessage = PTHREAD_MUTEX_INITIALIZER;
	memset(EIP, 0, 20);
	int res = socketClient.StartClient(ServerIP, ServerPort, messageCallback, statusCallback);
	if (res == 0) {
		status = Connected;
		cout << "connected to server: " << ServerIP << " Port:" << ServerPort << endl;
	} else {
		status = Idle;
		cout << "connect to server: " << ServerIP << " Port:" << ServerPort << " failed!" << endl;
		return res;
	}

	if (status == Connected) {
		char *userName = "武", *password = "ff", *p;
		int length, slength;
		length = 2 + strlen(userName) + 2 + strlen(password);
		slength = length + 2;
		p = mes;
		*p++ = 0x10;
		while (length > 127) {
			*p++ = (length % 128) || 0x80;
			length /= 128;
			slength++;
		}
		*p++ = (length % 128);
		*p++ = strlen(userName) / 256;
		*p++ = strlen(userName) % 256;
		memcpy(p, userName, strlen(userName));
		p += strlen(userName);
		*p++ = strlen(password) / 256;
		*p++ = strlen(password) % 256;
		memcpy(p, password, strlen(password));
		p += strlen(password);

		SendToServer(mes, slength);
		sem_wait(&newRecvedMessage);
		pthread_mutex_lock(&serverMessage);
		for (int i = 0; i < recvLength; i++) {
			printf("%02X ", recvMessage[i]);
		}
//		char * start = strstr(recvMessage, ":");
//		*start = 'P';
//		start++;
//		char * end = strstr(recvMessage, ",");
//		memcpy(EIP, start, end - start);
//		start = strstr(recvMessage, ":");
//		start++;
//		EPort = atoi(start);
//		cout << "IP: " << EIP << " Port:" << EPort << endl;
		pthread_mutex_unlock(&serverMessage);
	} else {
		return res;
	}

//	//connect to exchange server
//	res = socketClient.StartClient(EIP, EPort, messageCallback, statusCallback);
//	if (res == 0) {
//		status = Connected;
//		cout << "connected to server: " << EIP << " Port:" << EPort << endl;
//	} else {
//		status = Idle;
//		cout << "connect to server: " << EIP << " Port:" << EPort << " failed!" << endl;
//		return res;
//	}

	return res;
}

void IM::messageCallback(char *message, unsigned int messLength) {
	pthread_mutex_lock(&serverMessage);
	memcpy(recvMessage, message, messLength);
	recvLength = messLength;
	sem_post(&newRecvedMessage);
	pthread_mutex_unlock(&serverMessage);
}

void IM::statusCallback(clientStatus currentStatus) {
	if (currentStatus == Disconnected) {
		status = Disconnected;
		cout << "lost connection to server: " << ServerIP << endl;
	}
}

int IM::SendToServer(char * message, unsigned int length) {
	int res = -1;
	if (status == Connected) {
		res = socketClient.SendToServer(message, length);
		if (res < 0) {
			status = Idle;
			cout << "lost connection to server: " << ServerIP << endl;
		}
	}
	return res;
}

int main() {
	IM im;
	im.StartIM();
	cout << "Connect Error\n" << endl;

	return 0;
}
