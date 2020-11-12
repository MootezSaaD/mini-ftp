#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <iostream>
#include <string>
#include <Winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <fstream>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

#define DEFAULT_BUFLEN 4096

string serverIP = "127.0.0.1";

void SocketHandler(SOCKET Socket, int iResult) {
	if (iResult == -1) {
		closesocket(Socket);
		WSACleanup();
	} else {
		closesocket(Socket);
	}
}

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

char* ReadFile(const char* filename) {
    std::ifstream file(filename, std::ios::binary);
    char * buffer;
    if (file) {
        // get length of file:
        file.seekg (0, file.end);
        int length = file.tellg();
        file.seekg (0, file.beg);

        buffer = new char [length];
        // read data as a block:
        file.read (buffer, length);
        file.close();
    }
    return buffer;
}

int SendAll(SOCKET DataClientSocket, const char* filename)
{
    char* data = ReadFile(filename);
    int data_size = (int)strlen(data);
	const char* data_ptr = (const char*)data;
	int bytes_sent;
	while (data_size > 0)
	{
		bytes_sent = send(DataClientSocket, data_ptr, data_size, 0);
		if (bytes_sent == SOCKET_ERROR)
			return -1;
		data_ptr += bytes_sent;
		data_size -= bytes_sent;
	}
	//wprintf(L"Bytes sent: %d\n", bytes_sent);
	return 1;
}

vector<string> Tokenizer(const char* str, char c = ' ')
{
	vector<string> result;
	do
	{
		const char* begin = str;
		while (*str != c && *str)
			str++;
		result.push_back(string(begin, str));
	} while (0 != *str++);
	return result;
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
        wprintf(L"WSAStartup failed: %ld\n", iResult);
        return 1;
    }
    wprintf(L"WSAStartup successful\n");

    // Store server info
    SOCKET ControlSocket = SocketStarter(21);

    //----------------------
    // Sending and receiving data
    char buf[DEFAULT_BUFLEN];
    string input;
    do
    {
        SOCKET DataSocket;
        // Ask user for input
        wprintf(L"> ");
        getline(cin, input); // To retrieve text with spaces
        // Check user has typed
        if (input.size() > 0) {
            vector<string> tokens = Tokenizer(input.c_str());
            if (tokens.at(0) == "put" && tokens.size() < 2) {
                cout << "INVALID COMMAND: put <filename>" << endl;
                continue;
            } else if (tokens.at(0) == "get" && tokens.size() < 2) {
                cout << "INVALID COMMAND: get <filename>" << endl;
                continue;
            }

            int sendResult = send(ControlSocket, input.c_str(), input.size() + 1, 0); // + 1 beacuse string (char arrays) in C++ end with 0
            if (sendResult == SOCKET_ERROR || input == "bye" || input == "quit")
                break;

            if (tokens.at(0) == "put" && tokens.size() == 2) {
                const char* filename = tokens.at(1).c_str();
                DataSocket = SocketStarter(20);
                int iSendResult = SendAll(DataSocket, filename);
                SocketHandler(DataSocket, iSendResult);
            }
            else {
                ZeroMemory(buf, DEFAULT_BUFLEN);
                DataSocket = SocketStarter(20);
                // Wait for response
                int bytesReceived = recv(DataSocket, buf, DEFAULT_BUFLEN, 0);
                if (bytesReceived > 0)
                {
                    std::cout << string(buf, 0, bytesReceived);
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