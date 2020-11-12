#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <iostream>
#include <fstream>
#include "ftpHelper.h"

namespace fs = std::filesystem;

char* StrToChar(std::string str) {
	char* charArr = new char[str.size() + 1];
	copy(str.begin(), str.end(), charArr);
	charArr[str.size()] = '\0';
	return charArr;
}

std::vector<std::string> Tokenizer(const char* str, char c)
{
	std::vector<std::string> result;
	do
	{
		const char* begin = str;
		while (*str != c && *str)
			str++;
		result.push_back(std::string(begin, str));
	} while (0 != *str++);
	return result;
}

std::string GetCurrentDirectory()
{
	int shutdown = SHUTDOWN;
	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	std::string::size_type pos = std::string(buffer).find_last_of("\\/");
	return std::string(buffer).substr(0, pos);
}

std::size_t CountFilesInDir(std::filesystem::path path)
{
	using std::filesystem::directory_iterator;
	return std::distance(directory_iterator(path), directory_iterator{});
}

std::string GetFileList() {
	std::string files;
	std::string currentDir(GetCurrentDirectory());
	for (const auto& entry : fs::directory_iterator(currentDir)) {
		files.append(entry.path().filename().string() + '\n');
	}
	return files;
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

void SocketHandler(SOCKET Socket, int iResult) {
	if (iResult == -1) {
		closesocket(Socket);
		WSACleanup();
	} else {
		closesocket(Socket);
	}
}

int SendAll(SOCKET DataClientSocket, const void* data, int data_size)
{
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

int RecvAndWrite(SOCKET DataClientSocket, char* filename)
{
	char buffer[4096];
	int bytes_read;
	std::ofstream file(filename, std::ofstream::out|std::ofstream::binary);
	do
	{
		bytes_read = recv(DataClientSocket, buffer, sizeof(buffer), 0);
		if (bytes_read > 0) {
			file.write(buffer, bytes_read);
			// printf("Buffer: %.*s\n", bytes_read, buffer);
			//or: printf("Buffer: %*.*s\n", bytes_read, bytes_read, buffer);
		} else if (bytes_read == SOCKET_ERROR) {
			file.close();
			return -1;
		}
	}
	while (bytes_read > 0);
	file.close();
	return 1;
}