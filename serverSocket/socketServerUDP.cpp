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
	SOCKET Socket = socket(AF_INET, SOCK_DGRAM, 0);

    // Fill in socketaddr_in structure (IPv4)
	sockaddr_in serverHint;
	serverHint.sin_addr.S_un.S_addr = ADDR_ANY;
	serverHint.sin_family = AF_INET;
	serverHint.sin_port = htons(5150); // htons: host to network short



	if (bind(Socket, (SOCKADDR*)&serverHint, sizeof(serverHint)) == SOCKET_ERROR)
	{
		wprintf(L"Bind failed with error: %d\n", WSAGetLastError());
		closesocket(Socket);
		WSACleanup();
		return 1;
	}

   // Strucutre to hold client's ip and port
	sockaddr_in client;
	int clientLength = sizeof(client); // The size of the client information

	char recvbuf[DEFAULT_BUFLEN];
    

	// Enter a loop
	while (true)
	{
        // Clearing the client's structure and message buffer (initialization)
		ZeroMemory(&client, clientLength);
		ZeroMemory(recvbuf, DEFAULT_BUFLEN);

		int receivedBytes = recvfrom(Socket, recvbuf, 1024, 0, (SOCKADDR*)&client, &clientLength);
		if (receivedBytes == SOCKET_ERROR)
		{
			wprintf(L"Error Receving from Client: %d\n", WSAGetLastError());
			continue;
		}

		// Display message and client info
        // First initialize clientIP variable
        WCHAR clientIP[DEFAULT_BUFLEN];
		ZeroMemory(clientIP, DEFAULT_BUFLEN);

		// Convert from byte array to chars
		InetNtopW(AF_INET, &client.sin_addr, clientIP, DEFAULT_BUFLEN);

		// Display received data, including the sender's IP
		wprintf(L"[%ls] Said > %hs", clientIP, recvbuf);
	}

	// Close socket
	closesocket(Socket);

	// Shutdown winsock
	WSACleanup();
    return 0;
}