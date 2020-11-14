#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <string>
#include <ws2tcpip.h>
#include "ftpHelper.h"

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 4096

namespace fs = std::filesystem;

std::string GetErrorMessage()
{
	std::string errorMessage = "";
	switch (WSAGetLastError())
	{
	case 10060:
		errorMessage = "Socket connection timed out";
		break;
	default:
		errorMessage = "Recv failed with error: " + WSAGetLastError();
	}
	return errorMessage;
}

void setSocketTimeout(SOCKET ListenSocket, int timeout)
{
	int iResult;

	iResult = setsockopt(ListenSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
	if (iResult == SOCKET_ERROR)
	{
		std::cout << "setsockopt for SO_RCVTIMEO failed with error: " << WSAGetLastError() << std::endl;
		closesocket(ListenSocket);
		WSACleanup();
		exit(0);
	}
	else
		std::cout << "Set SO_RCVTIMEO Value: " << timeout << std::endl;

	iResult = setsockopt(ListenSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
	if (iResult == SOCKET_ERROR)
	{
		std::cout << "setsockopt for SO_SNDTIMEO failed with error: " << WSAGetLastError() << std::endl;
		closesocket(ListenSocket);
		WSACleanup();
		exit(0);
	}
	else
		std::cout << "Set SO_SNDTIMEO Value: " << timeout << std::endl;
}

SOCKET ServerSocketStart(int port, int timeout = 10000)
{
	//----------------------
	// Create a socket
	// https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-socket
	SOCKET Socket;
	Socket = socket(AF_INET, SOCK_STREAM, 0);
	if (Socket == INVALID_SOCKET)
	{
		std::cout << "Socket failed with error: " << WSAGetLastError() << std::endl;
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

	if (bind(Socket,
			 (SOCKADDR *)&saServer, sizeof(saServer)) == SOCKET_ERROR)
	{
		std::cout << "Bind failed with error: " <<  WSAGetLastError() << std::endl;
		closesocket(Socket);
		WSACleanup();
		exit(0);
	}

	if (timeout > 0)
		setSocketTimeout(Socket, timeout);

	//----------------------
	// Listen for incoming connection requests.
	// on the created socket
	// SOMAXCONN: value for maximum background connections allowed
	if (listen(Socket, SOMAXCONN) == SOCKET_ERROR)
	{
		std::cout << "Listen failed with error: " << WSAGetLastError() << std::endl;
		closesocket(Socket);
		WSACleanup();
		exit(0);
	}

	//----------------------
	// Prepare object for client socket address input
	sockaddr_in client;
	int clientSize = sizeof(client);

	// Create a SOCKET for accepting incoming requests.
	SOCKET ClientSocket;
	ClientSocket = accept(Socket, (sockaddr *)&client, &clientSize);
	if (ClientSocket == INVALID_SOCKET)
	{
		std::cout << "Accept failed with error: " << WSAGetLastError() << std::endl;
		closesocket(Socket);
		WSACleanup();
		exit(0);
	}
	else
		std::cout << "Client socket initialized" << std::endl;

	// Get data from socket
	WCHAR host[NI_MAXHOST];	   // client's remote name
	WCHAR service[NI_MAXHOST]; // service (port) the client is connected on

	// Equivalent to initializing a variable to 0;
	ZeroMemory(host, NI_MAXHOST);
	ZeroMemory(service, NI_MAXHOST);

	// Convert IPv4 from bytes to string
	InetNtopW(AF_INET, &client.sin_addr, host, NI_MAXHOST);

	// By default, networking is done with byte ordered in big-endian.
	// ntohs converts TCP/IP network bytes to little-endian (to be processed by CPU).
	std::cout << "Host " << host << " connected on port " << ntohs(client.sin_port) << std::endl;

	//----------------------
	// Close listening/server socket since we have established a connection
	// with a client socket.
	closesocket(Socket);
	return ClientSocket;
}

PARSER_CODES CommandParser(char *recvbuf, SOCKET ControlSocket)
{
	std::vector<std::string> tokens = Tokenizer(recvbuf, ' ');
	std::cout << "Command: ";
	for (auto i = tokens.begin(); i != tokens.end(); ++i)
		std::cout << *i << ' ';
	std::cout << std::endl;

	SOCKET DataClientSocket;
	int iRecvResult, iSendResult;
	if (tokens.at(0) == "ls" && tokens.size() == 1)
	{
		std::string filenames = GetFileList();
		char *cfilenames = StrToChar(filenames);

		DataClientSocket = ServerSocketStart(20);
		iSendResult = SendAll(DataClientSocket, cfilenames, (int)strlen(cfilenames));
		SocketHandler(DataClientSocket, iSendResult);
		return PARSER_CODES::CONTINUE;
	}
	else if (tokens.at(0) == "get" && tokens.size() == 2)
	{
		std::string toSendFile = tokens.at(1);
		char *fileToBeSent = ReadFile(toSendFile.c_str());

		DataClientSocket = ServerSocketStart(20);
		// If ( fileTobeSent == empty ) => file does not exist
		if (fileToBeSent[0] == '\0' || fileToBeSent == NULL)
		{
			char *message = StrToChar("File does not exist!");
			iSendResult = SendAll(DataClientSocket, message, (int)strlen(message));
			SocketHandler(DataClientSocket, iSendResult);
		}
		else
		{
			int data_size = (int)strlen(fileToBeSent);
			std::cout << "Sending: " << toSendFile << std::endl;
			iSendResult = SendAll(DataClientSocket, fileToBeSent, data_size);
			SocketHandler(DataClientSocket, iSendResult);
		}
		return PARSER_CODES::CONTINUE;
	}
	else if (tokens.at(0) == "put" && tokens.size() == 2)
	{
		DataClientSocket = ServerSocketStart(20);

		std::string toRecvPath = tokens.at(1);
		std::string toRecvFile;

		std::string::size_type pos = toRecvPath.find_last_of("\\/");
		if (pos != std::string::npos)
		{
			// Broken
			toRecvFile = toRecvPath.substr(pos, toRecvPath.length() - 1);
		}
		else
		{
			toRecvFile = toRecvPath;
			std::cout << "Fetching: " << toRecvFile << "..." << std::endl;
		}
		char *ctoRecvFile = StrToChar(toRecvFile);

		//iRecvResult = RecvAndWrite(DataClientSocket, ctoRecvFile);
		char buffer[DEFAULT_BUFLEN];
		int iRecvResult = RecvAndWrite(DataClientSocket, ctoRecvFile, buffer, sizeof(buffer));

		SocketHandler(DataClientSocket, iRecvResult);
		std::cout << "Done!" << std::endl;
		return PARSER_CODES::CONTINUE;
	}
	else if ((tokens.at(0) == "bye" || tokens.at(0) == "quit") && tokens.size() == 1)
	{
		iSendResult = shutdown(ControlSocket, SD_BOTH);
		return PARSER_CODES::SHUTDOWN;
	}
	char *response = {"Unsupported command"};

	DataClientSocket = ServerSocketStart(20);
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
	if (iResult != NO_ERROR)
	{
		std::cout << "WSAStartup failed: " << iResult << std::endl;
		return 1;
	}
	std::cout << "WSAStartup successful" << std::endl;

	SOCKET ControlSocket = ServerSocketStart(21, 0);

	int recvbuflen = DEFAULT_BUFLEN;
	char recvbuf[DEFAULT_BUFLEN] = "";
	int iRecvResult;

	// Receive until the peer closes the connection
	do
	{
		iRecvResult = recv(ControlSocket, recvbuf, recvbuflen, 0);
		if (iRecvResult > 0)
		{
			//std::cout << "Bytes received: " << iRecvResult << std::endl;
			int parserCode = CommandParser(recvbuf, ControlSocket);
			//std::cout << "Parser Code " << parserCode << std::endl;
			if (parserCode == PARSER_CODES::SHUTDOWN)
			{
				break;
			}
		}
		else if (iRecvResult == 0)
			std::cout << "Connection with client closed" << std::endl;
		else
		{
			std::cout << GetErrorMessage() << std::endl;
			break;
		}
	} while (iRecvResult > 0);

	//----------------------
	// Shutdown the connection since we're done
	iResult = shutdown(ControlSocket, SD_BOTH);
	if (iResult == SOCKET_ERROR)
	{
		std::cout << "Socket shutdown failed with error: " << WSAGetLastError() << std::endl;
		closesocket(ControlSocket);
		WSACleanup();
		return 1;
	}
	closesocket(ControlSocket);
	WSACleanup();
	return 0;
}