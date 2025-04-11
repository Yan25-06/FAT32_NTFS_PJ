#ifndef FAT32PARSER_H
#define FAT32PARSER_H

#include "diskManager.h"
#include <cstring>
#include <vector>
#define BYTES_PER_CLUSTER 4
struct FAT32_BootSector {
    WORD bytesPerSector;    // Số byte cho 1 sector
    BYTE sectorsPerCluster; // Số sector cho 1 cluster
    WORD reservedSectors;   // Số sector vùng Bootsector (Số sector dành riêng)
    DWORD fatSize32;        // Số sector cho 1 bảng FAT
    BYTE numFATs;           // Số bảng FAT
    DWORD rootCluster;
} __attribute__((packed));

struct DirectoryEntry {
    BYTE name[11];         // Tên file (8.3 format)
    BYTE attr;             // Thuộc tính file
    BYTE reserved;         // Reserved
    BYTE createTimeTenth;  // Milliseconds since creation
    WORD createTime;       // Thời gian tạo file
    WORD createDate;       // Ngày tạo file
    WORD lastAccessDate;   // Ngày truy cập gần nhất
    WORD startClusterHigh; // 2 byte cao của cluster đầu tiên
    WORD lastWriteTime;    // Thời gian sửa đổi gần nhất
    WORD lastWriteDate;    // Ngày sửa đổi gần nhất
    WORD startClusterLow;  // 2 byte thấp của cluster đầu tiên
    DWORD fileSize;        // Kích thước file
} __attribute__((packed));

struct LFNEntry {
    BYTE order;
    BYTE name1[10];
    BYTE attr;
    BYTE type;
    BYTE checksum;
    BYTE name2[12];
    WORD zero;
    BYTE name3[4];
} __attribute__((packed));
class Fat32Parser {
    private:
    DiskManager &diskManager;
    FAT32_BootSector bootSector;
    bool readBootSector();

    public:
    WORD getBytesPerSector() const;
    BYTE getSectorsPerCluster() const;
    WORD getReservedSectors() const;
    BYTE getNumFATs() const;
    DWORD getFATSize32() const;
    DWORD getRootCluster() const;
    DWORD getClusterSize() const;
    
    void printBootSectorInfo();
    bool readCluster(DWORD cluster, vector<BYTE> &buffer);
    
    Fat32Parser(DiskManager &d);
};

#endif