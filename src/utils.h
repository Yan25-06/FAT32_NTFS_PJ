#ifndef UTILS_H
#define UTILS_H

#include "fat32Parser.h"
#include <locale>
#include <codecvt>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>
#include <cstring>
#define SFN_SIZE 11
//FAT32
string extractFileName(const DirectoryEntry *entries, char firstChar, size_t index);
string utf16le_to_utf8(const wstring &wstr);
string extractSFN(const DirectoryEntry *entries, char firstChar, size_t index);
string extractLFN(const DirectoryEntry *entries, size_t index);
string convertToSFN(const string &filename);
void trim(string &s);
void printMenu();
//NTFS
string ConvertWCharToString(const wchar_t* wStr);
bool WriteVector_ToFile(string filename, const vector<BYTE>& data);
void WriteHex_ToFile(const char *filename, vector<BYTE> buf);
bool checkAllIsZero(vector<BYTE> content);
#endif