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

int main(int argc, char const *argv[])
{
    // Check if socket type,server IP and port are provided
    if (argc < 4)
    {
        wprintf(L"Usage: clientSocket <IP-ADDR> <TYPE: 1: TCP | 2: UDP> <PORT>\n");
        return 0;
    }
    // Store server info
    string serverIP = argv[1];
    int type, port;
    try
    {
        type = std::stoi(argv[2]);
        port = std::stoi(argv[3]);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }

    WSADATA wsaData;
    //----------------------
    // Initialize winsock
    // https://docs.microsoft.com/en-us/windows/win32/winsock/initializing-winsock
    // request v2.2
    WORD version = MAKEWORD(2, 2);
    int iResult = WSAStartup(version, &wsaData);
    if (iResult != NO_ERROR)
    {
        wprintf(L"WSAStartup failed: %ld\n", iResult);
        return 1;
    }
    wprintf(L"WSAStartup successful\n");

    //----------------------
    // Create a socket
    // https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-socket
    SOCKET Socket;
    Socket = socket(AF_INET, type, 0);
    if (Socket == INVALID_SOCKET)
    {
        wprintf(L"Socket failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    else
        wprintf(L"Server socket initialized\n");

    
    // Fill in socketaddr_in structure (IPv4)
    struct sockaddr_in saServer;
    saServer.sin_family = AF_INET;
    saServer.sin_port = htons(port);                          // htons: host to network short
    inet_pton(AF_INET, serverIP.c_str(), &saServer.sin_addr); // Converts the IPv4 address in its text representation

    // In case of a TCP Socket
    if (type == 1)
    {

        //----------------------
        // Connect to server
        int connResult = connect(Socket, (sockaddr *)&saServer, sizeof(saServer));
        if (connResult == SOCKET_ERROR)
        {
            wprintf(L"Server connect failed with error: %d\n", WSAGetLastError());
            closesocket(Socket);
            WSACleanup();
            return 0;
        }

        //----------------------
    }
    // Sending and receiving data
    char buf[DEFAULT_BUFLEN];
    string input;
    do
    {
        // Ask user for input
        wprintf(L"\n> ");
        getline(cin, input); // To retrieve text with spaces
                             // Send input
        // Check user has typed
        if (input.size() > 0)
        {
            // For TCP connections
            if (type == 1)
            {
                int sendResult = send(Socket, input.c_str(), input.size() + 1, 0); // + 1 beacuse string (char arrays) in C++ end with 0
                if (sendResult != SOCKET_ERROR)
                {
                    // Wait for response
                    ZeroMemory(buf, DEFAULT_BUFLEN);
                    int bytesReceived = recv(Socket, buf, DEFAULT_BUFLEN, 0);
                    if (bytesReceived > 0)
                    {
                        std::cout << "SERVER> " << string(buf, 0, bytesReceived);
                    }
                }
            } else {
                int sendResult = sendto(Socket, input.c_str(), input.size() + 1, 0, (SOCKADDR*)&saServer, sizeof(saServer));
                if(sendResult == SOCKET_ERROR) {
                    wprintf(L"Message Not Sent, Error %d", WSAGetLastError());
                } else {
                    ZeroMemory(buf, DEFAULT_BUFLEN);
                    int serverLength = sizeof(saServer);
                    int bytesReceived = recvfrom(Socket, buf, DEFAULT_BUFLEN, 0, (SOCKADDR*)&saServer, &serverLength);
                    {
                        std::cout << "SERVER> " << string(buf, 0, bytesReceived);
                    }
                }
            }
        }
    } while (input.size() > 0);

    // Shutdown everything
    closesocket(Socket);
    WSACleanup();

    return 0;
}
