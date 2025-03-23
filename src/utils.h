#ifndef UTILS_H
#define UTILS_H

#include "fat32Parser.h"
#include <locale>
#include <codecvt>
#include <algorithm>
#include <cctype>
#define SFN_SIZE 11

string extractFileName(const DirectoryEntry *entries, string filename, size_t index);
string utf16le_to_utf8(const wstring &wstr);
string extractSFN(const DirectoryEntry *entries, string filename, size_t index);
string extractLFN(const DirectoryEntry *entries, size_t index);
string convertToSFN(const string &filename);
void trim(string &s);

#endif