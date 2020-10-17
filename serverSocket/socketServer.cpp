#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <iostream>
#include <Winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

#define DEFAULT_BUFLEN 4096

int main()
{
	WSADATA wsaData;
	//----------------------
	// Initialize winsock
	// https://docs.microsoft.com/en-us/windows/win32/winsock/initializing-winsock
	// request v2.2
	WORD version = MAKEWORD(2, 2);
	int iResult = WSAStartup(version, &wsaData);
	if (iResult != NO_ERROR) {
		wprintf(L"WSAStartup failed: %ld\n", iResult);
		return 1;
	}
	wprintf(L"WSAStartup successful\n");

	//----------------------
	// Create a socket
	// https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-socket
	SOCKET ListenSocket;
	ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (ListenSocket == INVALID_SOCKET) {
		wprintf(L"Socket failed with error: %d\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}
	else
		wprintf(L"Server socket initialized\n");

	//----------------------
	// Bind IP addr and port to socket
	// https://docs.microsoft.com/en-us/windows/win32/winsock/sockaddr-2
	struct sockaddr_in saServer;
	// Fill in socketaddr_in structure (IPv4)
	saServer.sin_family = AF_INET;
	saServer.sin_addr.S_un.S_addr = INADDR_ANY;
	saServer.sin_port = htons(5150); // htons: host to network short

	if (bind(ListenSocket,
		(SOCKADDR*)&saServer, sizeof(saServer)) == SOCKET_ERROR) {
		wprintf(L"Bind failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	//----------------------
	// Listen for incoming connection requests.
	// on the created socket
	// SOMAXCONN: value for maximum background connections allowed
	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		wprintf(L"Listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	//----------------------
	// Prepare object for client socket address input
	sockaddr_in client;
	int clientSize = sizeof(client);

	// Create a SOCKET for accepting incoming requests.
	SOCKET ClientSocket;
	ClientSocket = accept(ListenSocket, (sockaddr*)&client, &clientSize);
	if (ClientSocket == INVALID_SOCKET) {
		wprintf(L"Accept failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	else
		wprintf(L"Client socket initialized\n");

	// Get data from socket
	WCHAR host[NI_MAXHOST]; // client's remote name
	WCHAR service[NI_MAXHOST]; // service (port) the client is connected on

	// Equivalent to initializing a variable to 0;
	ZeroMemory(host, NI_MAXHOST);
	ZeroMemory(service, NI_MAXHOST);

	// Convert IPv4 from bytes to string
	InetNtopW(AF_INET, &client.sin_addr, host, NI_MAXHOST);

	// By default, networking is done with byte ordered in big-endian.
	// ntohs converts TCP/IP network bytes to little-endian (to be processed by CPU).
	std::wcout << "Host " << host << " connected on port " << ntohs(client.sin_port) << std::endl;

	//----------------------
	// Close listening/server socket since we have established a connection
	// with a client socket.
	closesocket(ListenSocket);

	//----------------------
	// Accept and echo message back to client
	int recvbuflen = DEFAULT_BUFLEN;
	char recvbuf[DEFAULT_BUFLEN] = "";
	int iSendResult, iRecvResult;

	// Receive until the peer closes the connection
	do {
		iRecvResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (iRecvResult > 0) {
			wprintf(L"Bytes received: %d\n", iRecvResult);
			// Echo the buffer back to the sender
			iSendResult = send(ClientSocket, recvbuf, iRecvResult, 0);
			if (iSendResult == SOCKET_ERROR) {
				wprintf(L"Send failed with error: %d\n", WSAGetLastError());
				closesocket(ClientSocket);
				WSACleanup();
				return 1;
			}
			wprintf(L"Bytes sent: %d\n", iSendResult);
		}
		else if (iRecvResult == 0)
			wprintf(L"Connection with client closed\n");
		else {
			wprintf(L"Recv failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}
	} while (iRecvResult > 0);

	//----------------------
	// Shutdown the connection since we're done
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		wprintf(L"Client shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}

	// Cleanup
	closesocket(ClientSocket);
	WSACleanup();
	return 0;
}