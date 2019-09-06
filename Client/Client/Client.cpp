#define _CRT_SECURE_NO_WARNINGS

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

// what is the purpose
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

int _cdecl main(int argc, char **argv)
{
	// --------------------- from microsoft tutorial ---------------------------
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	int iRes;

	// Validate the parameters
	if (argc != 3) {
		printf("usage: %s server-name port\n", argv[0]);
		return 1;
	}

	// Initialize Winsock
	iRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iRes != 0) {
		printf("WSAStartup failed with error: %d\n", iRes);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	iRes = getaddrinfo(argv[1], argv[2], &hints, &result);
	if (iRes != 0) {
		printf("getaddrinfo failed with error: %d\n", iRes);
		WSACleanup();
		return 1;
	}

	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
		ConnectSocket = socket(ptr->ai_family,
			ptr->ai_socktype, ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		iRes = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iRes == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	// --------------------------- game logic ---------------------------------

	char fieldbuf[14 * 3 + 1];
	int fieldlen = 14 * 3 + 1;

	char command[3];
	int commandlen = 3;

	char sendbuf[3];
	int sendlen = 3;

	int x, y;

	do {
		// receive the field
		iRes = recv(ConnectSocket, fieldbuf, fieldlen, 0);
		if (iRes > 0) {
			printf("%s", fieldbuf);
		}
		else if (iRes == 0) {
			printf("Connection closed\n");
		}
		else {
			printf("recv failed with error: %d\n", WSAGetLastError());
		}

		// if my turn -> make a move
		// otherwise do nothing
		iRes = recv(ConnectSocket, command, commandlen, 0);
		if (iRes > 0) {
			if (strcmp(command, "MV") == 0) {
				printf("Your move: ");
				scanf("%d %d", &x, &y);
				sprintf(sendbuf, "%d%d", x, y);

				iRes = send(ConnectSocket, sendbuf, sendlen, 0);
				if (iRes == SOCKET_ERROR) {
					printf("send failed with error: %d\n", WSAGetLastError());
					closesocket(ConnectSocket);
					WSACleanup();
					return 1;
				}
			}
		}
		else if (iRes == 0) {
			printf("Connection closed\n");
		}
		else {
			printf("recv failed with error: %d\n", WSAGetLastError());
		}

		iRes = recv(ConnectSocket, command, commandlen, 0);
		if (iRes > 0) {
			if (strcmp(command, "NX") != 0) { // if NX do nothing
				iRes = recv(ConnectSocket, fieldbuf, fieldlen, 0);
				if (iRes > 0) {
					if (strcmp(command, "DR") == 0) { // if DR then draw
						printf("DRAW\n");
					}
					else if (strcmp(command, "W1") == 0) { // player 1 won
						printf("PLAYER 1 WON\n");
					}
					else if (strcmp(command, "W2") == 0) { // player 2 won
						printf("PLAYER 2 WON\n");
					}

					// print field
					printf("%s", fieldbuf);

					break;
				}
				else if (iRes == 0) {
					printf("Connection closed\n");
				}
				else {
					printf("recv failed with error: %d\n", WSAGetLastError());
				}
			}
		}
		else if (iRes == 0) {
			printf("Connection closed\n");
		}
		else {
			printf("recv failed with error: %d\n", WSAGetLastError());
		}

	} while (iRes > 0);

	iRes = shutdown(ConnectSocket, SD_SEND);
	if (iRes == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}

	closesocket(ConnectSocket);
	WSACleanup();

	return 0;
}