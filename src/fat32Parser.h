#ifndef FAT32PARSER_H
#define FAT32PARSER_H

#include "diskManager.h"
#include <cstring>
#include <vector>

struct FAT32_BootSector {
    WORD bytesPerSector;    // Số byte cho 1 sector
    BYTE sectorsPerCluster; // Số sector cho 1 cluster
    WORD reservedSectors;   // Số sector vùng Bootsector (Số sector dành riêng)
    DWORD fatSize32;        // Số sector cho 1 bảng FAT
    BYTE numFATs;           // Số bảng FAT
} __attribute__((packed));

class Fat32Parser {
    private:
    DiskManager disk;
    FAT32_BootSector bootSector;

    public:
    Fat32Parser(string drive);
    bool readBootSector();

    Fat32Parser(const DiskManager &d, FAT32_BootSector &bS);
};

#endif