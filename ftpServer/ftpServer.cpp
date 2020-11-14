#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <string>
#include <ws2tcpip.h>
#include "ftpHelper.h"

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 4096

namespace fs = std::filesystem;

SOCKET SocketStarter(int port) {
	//----------------------
	// Create a socket
	// https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-socket
	SOCKET ListenSocket;
	ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (ListenSocket == INVALID_SOCKET) {
		wprintf(L"Socket failed with error: %d\n", WSAGetLastError());
		WSACleanup();
		exit(0);
	}
	//----------------------
	// Bind IP addr and port to socket
	// https://docs.microsoft.com/en-us/windows/win32/winsock/sockaddr-2
	struct sockaddr_in saServer;
	// Fill in socketaddr_in structure (IPv4)
	saServer.sin_family = AF_INET;
	saServer.sin_addr.S_un.S_addr = INADDR_ANY;
	saServer.sin_port = htons(port); // htons: host to network short

	if (bind(ListenSocket,
		(SOCKADDR*)&saServer, sizeof(saServer)) == SOCKET_ERROR) {
		wprintf(L"Bind failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		exit(0);
	}

	//----------------------
	// Listen for incoming connection requests.
	// on the created socket
	// SOMAXCONN: value for maximum background connections allowed
	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		wprintf(L"Listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		exit(0);
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
		exit(0);
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
	// wcout << "Host " << host << " connected on port " << ntohs(client.sin_port) << std::endl;

	//----------------------
	// Close listening/server socket since we have established a connection
	// with a client socket.
	closesocket(ListenSocket);
	return ClientSocket;
}

PARSER_CODES CommandParser(char* recvbuf, SOCKET ControlSocket) {
	std::vector<std::string> tokens = Tokenizer(recvbuf, ' ');
	std::cout << "Command: ";
	for (auto i = tokens.begin(); i != tokens.end(); ++i)
	    std::cout << *i << ' ';
	std::cout << std::endl;
	
	SOCKET DataClientSocket;
	int iRecvResult, iSendResult;
	if (tokens.at(0) == "ls" && tokens.size() == 1) {
		std::string filenames = GetFileList();
		char* cfilenames = StrToChar(filenames);

		DataClientSocket = SocketStarter(20);
		iSendResult = SendAll(DataClientSocket, cfilenames, (int)strlen(cfilenames));
		SocketHandler(DataClientSocket, iSendResult);
		return PARSER_CODES::CONTINUE;
	}
	else if (tokens.at(0) == "get" && tokens.size() == 2) {
		std::string toSendFile = tokens.at(1);
		char* fileToBeSent = ReadFile(toSendFile.c_str());

		DataClientSocket = SocketStarter(20);
		// If ( fileTobeSent == empty ) => file does not exist
		if(fileToBeSent[0] == '\0' || fileToBeSent == NULL) {
			char* message = StrToChar("File does not exist!");
			iSendResult = SendAll(DataClientSocket, message, (int)strlen(message) );
			SocketHandler(DataClientSocket, iSendResult);
		} else {
			int data_size = (int) strlen(fileToBeSent);
			std::cout << "Sending: "<< toSendFile << std::endl;
			iSendResult = SendAll(DataClientSocket, fileToBeSent, data_size);
			SocketHandler(DataClientSocket, iSendResult);
		}
		return PARSER_CODES::CONTINUE;
	}
	else if (tokens.at(0) == "put" && tokens.size() == 2) {
		DataClientSocket = SocketStarter(20);

		std::string toRecvPath = tokens.at(1);
		std::string toRecvFile;

		std::string::size_type pos = toRecvPath.find_last_of("\\/");
		if (pos != std::string::npos) {
			// Broken
			toRecvFile = toRecvPath.substr(pos, toRecvPath.length() - 1);
		} else {
			toRecvFile = toRecvPath;
			std::cout << "Fetching: " << toRecvFile << "..." << std::endl;
		}
		char* ctoRecvFile = StrToChar(toRecvFile);

		//iRecvResult = RecvAndWrite(DataClientSocket, ctoRecvFile);
    	char buffer[DEFAULT_BUFLEN];
		int iRecvResult = RecvAndWrite(DataClientSocket, ctoRecvFile, buffer, sizeof(buffer));

		SocketHandler(DataClientSocket, iRecvResult);
		std::cout << "Done!" << std::endl;
		return PARSER_CODES::CONTINUE;
	}
	else if ((tokens.at(0) == "bye" || tokens.at(0) == "quit") && tokens.size() == 1) {
		iSendResult = shutdown(ControlSocket, SD_BOTH);
		return PARSER_CODES::SHUTDOWN;
	}
	char* response = {"Unsupported command"};

	DataClientSocket = SocketStarter(20);
	iSendResult = SendAll(DataClientSocket, response, (int)strlen(response));
	SocketHandler(DataClientSocket, iSendResult);
	return PARSER_CODES::CONTINUE;
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

	SOCKET ControlSocket = SocketStarter(21);

	int recvbuflen = DEFAULT_BUFLEN;
	char recvbuf[DEFAULT_BUFLEN] = "";
	int iRecvResult;

	// Receive until the peer closes the connection
	do {
		iRecvResult = recv(ControlSocket, recvbuf, recvbuflen, 0);
		if (iRecvResult > 0) {
			//wprintf(L"Bytes received: %d\n", iRecvResult);
			int parserCode = CommandParser(recvbuf, ControlSocket);
			//std::cout << "Parser Code " << parserCode << std::endl;
			if (parserCode == PARSER_CODES::SHUTDOWN) {
				break;
			}
		}
		else if (iRecvResult == 0)
			wprintf(L"Connection with client closed\n");
		else {
			wprintf(L"Recv failed with error: %d\n", WSAGetLastError());
			closesocket(ControlSocket);
			WSACleanup();
			return 1;
		}
	} while (iRecvResult > 0);
	WSACleanup();
	return 0;
}