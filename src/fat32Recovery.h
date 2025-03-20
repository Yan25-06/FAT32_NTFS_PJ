#ifndef FAT32_RECOVERY_H
#define FAT32_RECOVERY_H

#include "fat32Parser.h"
#include <vector>

class Fat32Recovery {
private:
    Fat32Parser &fatParser;

public:
    Fat32Recovery(Fat32Parser &parser);

    // Quét tất cả các file đã xóa
    vector<string> scanDeletedFiles();

    // Tìm file đã xóa theo tên
    DWORD scanDeletedFile(const string &fileName);

    // Khôi phục file đã xóa
    bool recoverFile(const string &fileName, const string &outputPath);
};

#endif
