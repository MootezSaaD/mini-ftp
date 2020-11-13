#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <string>
#include <ws2tcpip.h>
#include "ftpHelper.h"

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 4096

std::string serverIP = "127.0.0.1";

SOCKET SocketStarter(int port) {
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
    return Socket;
}

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
        std::cout << "WSAStartup failed: " << iResult << std::endl;
        return 1;
    }
    std::cout << "WSAStartup successful" << std::endl;

    // Store server info
    SOCKET ControlSocket = SocketStarter(21);

    //----------------------
    // Sending and receiving data
    char buf[DEFAULT_BUFLEN];
    std::string input;
    do
    {
        SOCKET DataSocket;
        // Ask user for input
        wprintf(L"> ");
        getline(std::cin, input); // To retrieve text with spaces
        // Check user has typed
        if (input.size() > 0) {
            std::vector<std::string> tokens = Tokenizer(input.c_str(), ' ');
            if (tokens.at(0) == "put" && tokens.size() < 2) {
                std::cout << "INVALID COMMAND: put <filename>" << std::endl;
                continue;
            } else if (tokens.at(0) == "get" && tokens.size() < 2) {
                std::cout << "INVALID COMMAND: get <filename>" << std::endl;
                continue;
            }

            int sendResult = send(ControlSocket, input.c_str(), input.size() + 1, 0); // + 1 beacuse std::string (char arrays) in C++ end with 0
            if (sendResult == SOCKET_ERROR || input == "bye" || input == "quit")
                break;

            if (tokens.at(0) == "put" && tokens.size() == 2) {
                const char* filename = tokens.at(1).c_str();
                DataSocket = SocketStarter(20);

                char* data = ReadFile(filename);
                int data_size = (int)strlen(data);
                int iSendResult = SendAll(DataSocket, data, data_size);
                SocketHandler(DataSocket, iSendResult);
            }
            if(tokens.at(0) == "get" && tokens.size() == 2) {
                DataSocket = SocketStarter(20);
                char* filename = StrToChar(tokens.at(1));
                int iRecvResult = RecvAndWrite(DataSocket, filename);
                SocketHandler(DataSocket, iRecvResult);
            }
            else {
                ZeroMemory(buf, DEFAULT_BUFLEN);
                DataSocket = SocketStarter(20);
                // Wait for response
                int bytesReceived = recv(DataSocket, buf, DEFAULT_BUFLEN, 0);
                if (bytesReceived > 0)
                {
                    std::cout << std::string(buf, 0, bytesReceived);
                    wprintf(L"\n");
                }
            }
        }
    } while (true);
  
    // Shutdown everything
    closesocket(ControlSocket);
    WSACleanup();
    return 0;
}