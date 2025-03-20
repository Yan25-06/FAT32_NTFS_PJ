#include "fat32Recovery.h"

Fat32Recovery::Fat32Recovery(Fat32Parser &parser) : fatParser(parser) {}

vector<string> Fat32Recovery::scanDeletedFiles() {
    vector<string> deletedFiles;
    vector<DirectoryEntry> entries = fatParser.readRootDirectory();

    for (const auto &entry : entries) {
        if (entry.name[0] == 0xE5) { // File đã xóa có ký tự đầu là 0xE5
            string fileName(reinterpret_cast<const char*>(entry.name), 11);
            deletedFiles.push_back(fileName);
        }
    }
    return deletedFiles;
}