#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <iostream>
#include <string>
#include <Winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

#define DEFAULT_BUFLEN 4096

int mane()
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

    // Store server info
    string serverIP = "127.0.0.1";
    int port = 21;

    //----------------------
    // Create a socket (TCP for now)
    // https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-socket
    SOCKET Socket;
    Socket = socket(AF_INET, SOCK_STREAM, 0);
    if (Socket == INVALID_SOCKET) {
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
    saServer.sin_port = htons(port); // htons: host to network short
    inet_pton(AF_INET, serverIP.c_str(), &saServer.sin_addr); // Converts the IPv4 address in its text representation

    //----------------------
    // Connect to server
    int connResult = connect(Socket, (sockaddr*)&saServer, sizeof(saServer));
    if (connResult == SOCKET_ERROR) {
        wprintf(L"Server connect failed with error: %d\n", WSAGetLastError());
        closesocket(Socket);
        WSACleanup();
        return 0;
    }

    //----------------------
    // Sending and receiving data
    char buf[DEFAULT_BUFLEN];
    string input;
    do
    {
        // Ask user for input
        wprintf(L"> ");
        getline(cin, input); // To retrieve text with spaces
        // Send input
        // Check user has typed
        if (input.size() > 0) {
            int sendResult = send(Socket, input.c_str(), input.size() + 1, 0); // + 1 beacuse string (char arrays) in C++ end with 0
            if (sendResult != SOCKET_ERROR)
            {
                // Wait for response
                ZeroMemory(buf, DEFAULT_BUFLEN);

                // Store server info
                int port = 20;

                //----------------------
                // Create a socket (TCP for now)
                // https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-socket
                SOCKET DataSocket;
                DataSocket = socket(AF_INET, SOCK_STREAM, 0);
                if (DataSocket == INVALID_SOCKET) {
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
                saServer.sin_port = htons(port); // htons: host to network short
                inet_pton(AF_INET, serverIP.c_str(), &saServer.sin_addr); // Converts the IPv4 address in its text representation

                //----------------------
                // Connect to server
                int connResult = connect(DataSocket, (sockaddr*)&saServer, sizeof(saServer));
                if (connResult == SOCKET_ERROR) {
                    wprintf(L"Server connect failed with error: %d\n", WSAGetLastError());
                    closesocket(DataSocket);
                    WSACleanup();
                    return 0;
                }

                int bytesReceived = recv(DataSocket, buf, DEFAULT_BUFLEN, 0);
                if (bytesReceived > 0)
                {
                    std::cout << "SERVER> " << string(buf, 0, bytesReceived);
                }
                closesocket(DataSocket);
                WSACleanup();
            }
        }
    } while (input.size() > 0);

    // Shutdown everything
    closesocket(Socket);
    WSACleanup();

    return 0;
}