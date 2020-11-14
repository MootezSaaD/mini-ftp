#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <string>
#include <ws2tcpip.h>
#include "ftpHelper.h"

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 4096

std::string serverIP;

SOCKET ClientSocketStart(int port)
{
    //----------------------
    // Create a socket (TCP for now)
    // https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-socket
    SOCKET Socket;
    Socket = socket(AF_INET, SOCK_STREAM, 0);
    if (Socket == INVALID_SOCKET)
    {
        std::cout << "Socket failed with error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }
    //----------------------
    // Bind IP addr and port to socket
    // https://docs.microsoft.com/en-us/windows/win32/winsock/sockaddr-2
    struct sockaddr_in saServer;
    // Fill in socketaddr_in structure (IPv4)
    saServer.sin_family = AF_INET;
    saServer.sin_port = htons(port);                          // htons: host to network short
    inet_pton(AF_INET, serverIP.c_str(), &saServer.sin_addr); // Converts the IPv4 address in its text representation

    //----------------------
    // Connect to server
    int connResult = connect(Socket, (sockaddr *)&saServer, sizeof(saServer));
    if (connResult == SOCKET_ERROR)
    {
        std::cout << "Server connect failed with error: " << WSAGetLastError() << std::endl;
        closesocket(Socket);
        WSACleanup();
        exit(0);
    }
    return Socket;
}

int main(int argc, char const *argv[])
{
    serverIP = argv[1];
    WSADATA wsaData;
    //----------------------
    // Initialize winsock
    // https://docs.microsoft.com/en-us/windows/win32/winsock/initializing-winsock
    // request v2.2
    WORD version = MAKEWORD(2, 2);
    int iResult = WSAStartup(version, &wsaData);
    if (iResult != NO_ERROR)
    {
        std::cout << "WSAStartup failed: " << iResult << std::endl;
        return 1;
    }
    std::cout << "WSAStartup successful" << std::endl;

    // Store server info
    SOCKET ControlSocket = ClientSocketStart(21);

    //----------------------
    // Sending and receiving data
    char buffer[DEFAULT_BUFLEN];
    int bytesRecv;

    SOCKET DataSocket;
    std::string input;
    std::vector<std::string> tokens;
    do
    {
        // Ask user for input
        std::cout << "> ";
        getline(std::cin, input); // To retrieve text with spaces
        if (std::cin.fail() || std::cin.eof())
        {
            std::cin.clear(); // reset cin state
        }
        // Check user has typed
        if (input.size() > 0)
        {
            tokens = Tokenizer(input.c_str(), ' ');
            if (tokens.at(0) == "put" && tokens.size() < 2)
            {
                std::cout << "Invalid command: put <filename>" << std::endl;
                continue;
            }
            else if (tokens.at(0) == "get" && tokens.size() < 2)
            {
                std::cout << "Invalid command get <filename>" << std::endl;
                continue;
            }

            iResult = send(ControlSocket, input.c_str(), input.size() + 1, 0); // + 1 beacuse std::string (char arrays) in C++ end with 0
            if (iResult == SOCKET_ERROR || input == "bye" || input == "quit")
                break;

            if (tokens.at(0) == "put" && tokens.size() == 2)
            {
                const char *filename = tokens.at(1).c_str();
                DataSocket = ClientSocketStart(20);

                char *data = ReadFile(filename);
                int data_size = (int)strlen(data);
                iResult = SendAll(DataSocket, data, data_size);
                SocketHandler(DataSocket, iResult);
            }
            else if (tokens.at(0) == "get" && tokens.size() == 2)
            {
                ZeroMemory(buffer, sizeof(buffer));
                DataSocket = ClientSocketStart(20);
                char *filename = StrToChar(tokens.at(1));
                iResult = RecvAndWrite(DataSocket, filename, buffer, sizeof(buffer));
                SocketHandler(DataSocket, iResult);
            }
            else
            {
                ZeroMemory(buffer, sizeof(buffer));
                DataSocket = ClientSocketStart(20);
                // Wait for response
                bytesRecv = RecvAll(DataSocket, buffer, sizeof(buffer));
                std::cout << buffer << std::endl;
                SocketHandler(DataSocket, iResult);
            }
        }
    } while (iResult > 0);

    //----------------------
    // Shutdown the connection since we're done
    iResult = shutdown(ControlSocket, SD_SEND);
    if (iResult == SOCKET_ERROR)
    {
        std::cout << "Socket shutdown failed with error: " << WSAGetLastError() << std::endl;
        closesocket(ControlSocket);
        WSACleanup();
        return 1;
    }

    // Shutdown everything
    closesocket(ControlSocket);
    WSACleanup();
    return 0;
}