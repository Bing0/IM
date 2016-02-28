/*
 * SocketClient.h
 *
 *  Created on: Feb 23, 2016
 *      Author: alcht
 */

#ifndef SOCKETCLIENT_H_
#define SOCKETCLIENT_H_
#include <pthread.h>

#define BUFFER_SIZE 4096

enum clientStatus {
	Idle, Connected, Disconnected
};

typedef void (*pCallbackUpdateMessage)(char *message, unsigned int messLength);
typedef void (*pCallbackUpdateStatus)(clientStatus currentStatus);

class SocketClient {
public:
	SocketClient();
	virtual ~SocketClient();
	int StartClient(char * serverIP, unsigned int serverPort, pCallbackUpdateMessage pUpdateMessage, pCallbackUpdateStatus pUpdateStauts);

	int SendToServer(char *message, unsigned int messLength);
	int DisconnectFromServer();
private:
	pCallbackUpdateMessage pUpdateMessage;
	pCallbackUpdateStatus pUpdateStauts;
	int clientSockFd;
	pthread_mutex_t sendMutex;
	char recvMessage[BUFFER_SIZE];
	pthread_t recvThreadID;
	static void *ReceiveThread(void *para);
};

#endif /* SOCKETCLIENT_H_ */
