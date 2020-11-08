#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <iostream>
#include <Winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <filesystem>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 4096

using namespace std;
namespace fs = filesystem;

string GetCurrentDirectory()
{
	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	string::size_type pos = string(buffer).find_last_of("\\/");
	return string(buffer).substr(0, pos);
}

std::size_t numberOfFilesInDirectory(std::filesystem::path path)
{
	using std::filesystem::directory_iterator;
	return std::distance(directory_iterator(path), directory_iterator{});
}

string GetFileList() {
	string files;
	string currentDir(GetCurrentDirectory());
	for (const auto& entry : fs::directory_iterator(currentDir)) {
		files.append(entry.path().filename().string() + '\n');
	}
	return files;
}

vector<string> Split(const char* str, char c = ' ')
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
	// wcout << "Host " << host << " connected on port " << ntohs(client.sin_port) << endl;

	//----------------------
	// Close listening/server socket since we have established a connection
	// with a client socket.
	closesocket(ListenSocket);
	return ClientSocket;
}

int SendAll(SOCKET client_socket, const void* data, int data_size)
{
	const char* data_ptr = (const char*)data;
	int bytes_sent;
	while (data_size > 0)
	{
		bytes_sent = send(client_socket, data_ptr, data_size, 0);
		if (bytes_sent == SOCKET_ERROR)
			return -1;
		data_ptr += bytes_sent;
		data_size -= bytes_sent;
	}
	wprintf(L"Bytes sent: %d\n", bytes_sent);
	return 1;
}

void SocketHandler(SOCKET Socket, int iSendResult) {
	if (iSendResult == -1) {
		closesocket(Socket);
		WSACleanup();
	} else {
		closesocket(Socket);
	}
}

enum PARSER_CODES {
	SHUTDOWN = 400,
	CONTINUE = 200,
};

PARSER_CODES CommandParser(char* recvbuf, SOCKET ControlSocket) {
	vector<string> tokens = Split(recvbuf);
	// for (auto i = tokens.begin(); i != tokens.end(); ++i)
	//     std::cout << *i << ' ';
	SOCKET DataClientSocket;
	int iSendResult;
	if (tokens.at(0) == "ls" && tokens.size() == 1) {
		string filenames = GetFileList();
		char* cfilenames = new char[filenames.size() + 1];
		copy(filenames.begin(), filenames.end(), cfilenames);
		cfilenames[filenames.size()] = '\0';

		DataClientSocket = SocketStarter(20);
		iSendResult = SendAll(DataClientSocket, cfilenames, (int)strlen(cfilenames));
		cout << iSendResult << endl;
		SocketHandler(DataClientSocket, iSendResult);
		return CONTINUE;
	}
	else if (tokens.at(0) == "get" && tokens.size() == 2) {
		string toDLFile = tokens.at(2);
		return CONTINUE;
	}
	else if (tokens.at(0) == "put" && tokens.size() == 2) {
		string toUPFile = tokens.at(2);
		return CONTINUE;
	}
	else if (tokens.at(0) == "bye" && tokens.size() == 1) {
		iSendResult = shutdown(ControlSocket, SD_BOTH);
		return SHUTDOWN;
	}
	char* response = {"Unsupported command"};
	DataClientSocket = SocketStarter(20);
	iSendResult = SendAll(DataClientSocket, response, (int)strlen(response));
	SocketHandler(DataClientSocket, iSendResult);
	return CONTINUE;
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
			wprintf(L"Bytes received: %d\n", iRecvResult);
			int parserCode = CommandParser(recvbuf, ControlSocket);
			if (parserCode == SHUTDOWN) {
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