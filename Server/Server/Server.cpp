/* USAGE
 * start server
 * start client 1: Client.exe localhost 27015
 * start client 2: Client.exe localhost 27016
 */

#define _CRT_SECURE_NO_WARNINGS
#undef UNICODE

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#pragma comment (lib, "Ws2_32.lib")

 // ports
const char ports[2][6] = { "27015", "27016" };

// check if player TURN won
bool is_win(int turn, int field[3][3]) {
	int value = turn == 0 ? 1 : -1;
	if ((field[0][0] == value && field[0][1] == value && field[0][2] == value) ||
		(field[1][0] == value && field[1][1] == value && field[1][2] == value) ||
		(field[2][0] == value && field[2][1] == value && field[2][2] == value) ||
		(field[0][0] == value && field[1][0] == value && field[2][0] == value) ||
		(field[0][1] == value && field[1][1] == value && field[1][2] == value) ||
		(field[0][2] == value && field[1][2] == value && field[2][2] == value) ||
		(field[0][0] == value && field[1][1] == value && field[2][2] == value) ||
		(field[0][2] == value && field[1][1] == value && field[2][0] == value)) {
		return true;
	}
	return false;
}

// check if field is full
bool is_draw(int field[3][3]) {
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			if (field[i][j] == 0) {
				return false;
			}
		}
	}
	return true;
}

// field as int -> field as string
void field_to_str(char* fieldbuf, int fieldlen, int field[3][3]) {
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			strcat(fieldbuf, "| ");
			switch (field[i][j]) {
			case 0:
				strcat(fieldbuf, "  ");
				break;
			case -1:
				strcat(fieldbuf, "o ");
				break;
			case 1:
				strcat(fieldbuf, "x ");
				break;
			default:
				break;
			}
		}
		strcat(fieldbuf, "|\n");
	}
}

int __cdecl main(void)
{

	// --------------------- from microsoft tutorial ---------------------------
	WSADATA wsaData;
	int iRes, iSendRes;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket[2] = { INVALID_SOCKET, INVALID_SOCKET };

	struct addrinfo* result = NULL;
	struct addrinfo hints;

	iRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iRes != 0) {
		printf("WSAStartup failed with error: %d\n", iRes);
		return 1;
	}

	for (int i = 0; i < 2; i++) {
		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;

		iRes = getaddrinfo(NULL, ports[i], &hints, &result);
		if (iRes != 0) {
			printf("getaddrinfo failed with error: %d\n", iRes);
			WSACleanup();
			return 1;
		}

		ListenSocket = socket(result->ai_family, result->ai_socktype,
			result->ai_protocol);
		if (ListenSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			freeaddrinfo(result);
			WSACleanup();
			return 1;
		}

		iRes = bind(ListenSocket, result->ai_addr,
			(int)result->ai_addrlen);
		if (iRes == SOCKET_ERROR) {
			printf("bind failed with error: %d\n", WSAGetLastError());
			freeaddrinfo(result);
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		freeaddrinfo(result);

		iRes = listen(ListenSocket, SOMAXCONN);
		if (iRes == SOCKET_ERROR) {
			printf("listen failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		ClientSocket[i] = accept(ListenSocket, NULL, NULL);
		if (ClientSocket[i] == INVALID_SOCKET) {
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		closesocket(ListenSocket);
	}

	// --------------------------- game logic ---------------------------------

	int x, y;
	int field[3][3] = { 0 };
	int turn = 0; // 1st player

	char fieldbuf[14 * 3 + 1] = { 0 };
	int fieldlen = 14 * 3 + 1;

	// move, wait, draw, win, next (= neither win nor draw)
	char command[6][3] = { "MV", "WT", "DR", "W1", "W2", "NX" };
	int commandlen = 3;

	char movebuf[3];
	int movebuflen = 3;

	do {
		// send field
		memset(fieldbuf, 0, fieldlen);
		field_to_str(fieldbuf, fieldlen, field);
		for (int i = 0; i < 2; i++) {
			iSendRes = send(ClientSocket[i], fieldbuf, fieldlen, 0);
			if (iSendRes == SOCKET_ERROR) {
				printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(ClientSocket[0]);
				closesocket(ClientSocket[1]);
				WSACleanup();
				return 1;
			}
		}

		// send move (MV) / wait (WT)
		iSendRes = send(ClientSocket[turn], command[0], commandlen, 0);
		if (iSendRes == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket[0]);
			closesocket(ClientSocket[1]);
			WSACleanup();
			return 1;
		}
		iSendRes = send(ClientSocket[turn == 0 ? 1 : 0], command[1], commandlen, 0);
		if (iSendRes == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket[0]);
			closesocket(ClientSocket[1]);
			WSACleanup();
			return 1;
		}

		// receive coordinates
		iRes = recv(ClientSocket[turn], movebuf, movebuflen, 0);
		if (iRes > 0) {
			x = movebuf[0] - 48;
			y = movebuf[1] - 48;

			// check validity of x and y
			if ((x < 0 || x > 2) || (y < 0 || y > 2) || field[x][y] != 0) {
				for (int i = 0; i < 2; i++) {
					iSendRes = send(ClientSocket[i], command[5], commandlen, 0);
					if (iSendRes == SOCKET_ERROR) {
						printf("send failed with error: %d\n", WSAGetLastError());
						closesocket(ClientSocket[0]);
						closesocket(ClientSocket[1]);
						WSACleanup();
						return 1;
					}
				}
				continue;
			}

			field[x][y] = turn == 0 ? 1 : -1;

			if (is_win(turn, field)) { // send W1 or W2
				for (int i = 0; i < 2; i++) {
					iSendRes = send(ClientSocket[i], command[turn == 0 ? 3 : 4], commandlen, 0);
					if (iSendRes == SOCKET_ERROR) {
						printf("send failed with error: %d\n", WSAGetLastError());
						closesocket(ClientSocket[0]);
						closesocket(ClientSocket[1]);
						WSACleanup();
						return 1;
					}
				}
				break;
			}
			else if (is_draw(field)) { // send DR
				for (int i = 0; i < 2; i++) {
					iSendRes = send(ClientSocket[i], command[2], commandlen, 0);
					if (iSendRes == SOCKET_ERROR) {
						printf("send failed with error: %d\n", WSAGetLastError());
						closesocket(ClientSocket[0]);
						closesocket(ClientSocket[1]);
						WSACleanup();
						return 1;
					}
				}
				break;
			}
			else { // send NX (continue playing)
				for (int i = 0; i < 2; i++) {
					iSendRes = send(ClientSocket[i], command[5], commandlen, 0);
					if (iSendRes == SOCKET_ERROR) {
						printf("send failed with error: %d\n", WSAGetLastError());
						closesocket(ClientSocket[0]);
						closesocket(ClientSocket[1]);
						WSACleanup();
						return 1;
					}
				}
			}
		}
		else if (iRes == 0) {
			printf("Connection closing...\n");
		}
		else {
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket[0]);
			closesocket(ClientSocket[1]);
			WSACleanup();
			return 1;
		}

		turn = turn == 0 ? 1 : 0;
	} while (iRes > 0);

	// send end state of the field
	memset(fieldbuf, 0, fieldlen);
	field_to_str(fieldbuf, fieldlen, field);
	for (int i = 0; i < 2; i++) {
		iSendRes = send(ClientSocket[i], fieldbuf, fieldlen, 0);
		if (iSendRes == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket[0]);
			closesocket(ClientSocket[1]);
			WSACleanup();
			return 1;
		}
	}

	iRes = shutdown(ClientSocket[0], SD_SEND);
	if (iRes == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket[0]);
		closesocket(ClientSocket[1]);
		WSACleanup();
		return 1;
	}

	iRes = shutdown(ClientSocket[1], SD_SEND);
	if (iRes == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket[0]);
		closesocket(ClientSocket[1]);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(ClientSocket[0]);
	closesocket(ClientSocket[1]);
	WSACleanup();

	return 0;
}