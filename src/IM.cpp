//============================================================================
// Name        : IM.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include "../head/SocketClient.h"
#include "../head/MD5.h"
#include <semaphore.h>
#include "string.h"
#include "stdlib.h"
#include "stdio.h"

using namespace std;

#define ServerIP "127.0.0.1"
#define ServerPort 6500

class IM {

public:
	int StartIM(char *userName, char *password);
	static clientStatus status;
	int SendToServer(char * message, unsigned int length);
private:
	SocketClient socketClient;
	MD5 md5;
	static void messageCallback(char *message, unsigned int messLength);
	static void statusCallback(clientStatus currentStatus);
	int Login(char *userName, char *password);

	static char recvMessage[BUFFER_SIZE];
	char EIP[20];
	int EPort;
	static unsigned int recvLength;
	static sem_t newRecvedMessage;
	static pthread_mutex_t serverMessage;

	static void * ReceiveThread(void * para);

};

clientStatus IM::status;
char IM::recvMessage[BUFFER_SIZE];
unsigned int IM::recvLength;
sem_t IM::newRecvedMessage;
pthread_mutex_t IM::serverMessage;

int IM::Login(char *userName, char *password) {
	char mes[200];
	char *p;
	int length, slength, res;
	md5.update("nvcaodnkn");
	const char * identifier = md5.toString().data();

	length = 2 + strlen(userName) + 2 + strlen(password) + 2 + strlen(identifier);
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

	*p++ = strlen(identifier) / 256;
	*p++ = strlen(identifier) % 256;
	memcpy(p, identifier, strlen(identifier));
	p += strlen(identifier);

	SendToServer(mes, slength);
	sem_wait(&newRecvedMessage);
	pthread_mutex_lock(&serverMessage);
	for (unsigned int i = 0; i < recvLength; i++) {
		printf("%02X ", (unsigned char) recvMessage[i]);
	}
	printf("\n");
	if (recvMessage[3] == 0) {
		printf("login succeed\n");
		res = 0;
	} else {
		printf("wrong user name or password\n");
		res = 1;
	}

	pthread_mutex_unlock(&serverMessage);

	return res;
}

int IM::StartIM(char *userName, char *password) {
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
	}

	if (status == Connected) {
		if (Login(userName, password)) {
			res = -1;
		} else {
			//login succeed
			pthread_t recvThreadID;
			pthread_create(&recvThreadID, NULL, ReceiveThread, this);
			/*
			 * head
			 * length
			 * destination utf-8 encoded string
			 * Emergency
			 * data
			 *
			 */

			while (1) {
				char destination[60], Emergency, payload[60], mes[200], *p;
				int tmp, slength;
				p = mes;
				slength = 0;

				memset(destination, 0, sizeof(destination));
				memset(payload, 0, sizeof(payload));

				p[0] = 0x30;
				p++;
				slength++;
				printf("Destination emergency Payload:\n");
				scanf("%s %c %s", destination, &Emergency, payload);
				printf("ipnut:\n%s\n", destination);
				printf("%c\n", Emergency);
				printf("%s\nend input\n", payload);

				tmp = 2 + strlen(destination) + 1 + strlen(payload);

				while (tmp > 127) {
					*p = (tmp % 128) | 0x80;
					slength++;
					p++;
				}
				*p = (tmp % 128);
				p++;
				slength++;

				slength += tmp;

				p[0] = strlen(destination) / 256;
				p[1] = strlen(destination) % 256;
				p += 2;

				memcpy(p, destination, strlen(destination));
				p++;

				p[0] = Emergency - 48;
				p++;
				memcpy(p, payload, strlen(payload));

				for (int i = 0; i < slength; i++) {
					printf("%02X ", (unsigned char) mes[i]);
				}
				printf("\n");

				SendToServer(mes, slength);

			}
		}
	}

	return res;
}

void * IM::ReceiveThread(void * para) {
	IM *im = (IM*)para;
	while (1) {
		sem_wait(&newRecvedMessage);
		pthread_mutex_lock(&serverMessage);
		printf("Recv: %s\n", recvMessage);
		for (unsigned int i = 0; i < recvLength; i++) {
			printf("%02X ", (unsigned char) recvMessage[i]);
		}
		printf("\n");
		if (recvMessage[0] == (char)0xC0) {
			char pingrep[] = { (char)0xD0, 0x00 };
			im->SendToServer(pingrep, 2);
		}
		pthread_mutex_unlock(&serverMessage);
	}
	return 0;
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

int main(int argc, char *argv[]) {
	IM im;
	char *userName, *password;

	if (argc != 3) {
		cout << "Please input username and password. example: ./IM wu bbb" << endl;
		return -1;
	}
	userName = argv[1];
	password = argv[2];

	im.StartIM(userName, password);
	cout << "Connect Error\n" << endl;

	return 0;
}
