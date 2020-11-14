#pragma once

#include <iostream>
#include <vector>
#include <filesystem>
#include <Winsock2.h>

char* StrToChar(std::string str);
std::vector<std::string> Tokenizer(const char* str, char c);

std::size_t NumberOfFilesInDir();
std::size_t CountFilesInDir(std::filesystem::path path);
std::string GetFileList();
char* ReadFile(const char* filename);


enum PARSER_CODES { SHUTDOWN, CONTINUE };
void SocketHandler(SOCKET Socket, int iResult);
int SendAll(SOCKET Socket, const void* data, int data_size);
int RecvAll(SOCKET Socket, char* data_ptr, int data_size);
int RecvAndWrite(SOCKET Socket, char* filename, char* data_ptr, int data_size);