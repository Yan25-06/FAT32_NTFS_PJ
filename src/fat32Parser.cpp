#include "fat32Parser.h"

Fat32Parser::Fat32Parser(const DiskManager &d, FAT32_BootSector &bS) : disk(d), bootSector(bS) {}

bool Fat32Parser::readBootSector() {
    BYTE bootSectorData[512]; // Boot Sector có kích thước 512 byte
    memset(bootSectorData, 0, sizeof(bootSectorData));

    // Đọc sector đầu tiên (Boot Sector)
    if (!disk.readSector(0, bootSectorData, BYTES_PER_SECTOR)) {
        cerr << "Loi: Khong the doc Boot Sector!" << endl;
        return false;
    }

    // Đọc các trường bằng memcpy để tránh lỗi alignment
    memcpy(&bootSector.bytesPerSector, &bootSectorData[11], sizeof(WORD));
    memcpy(&bootSector.sectorsPerCluster, &bootSectorData[13], sizeof(BYTE));
    memcpy(&bootSector.reservedSectors, &bootSectorData[14], sizeof(WORD));
    memcpy(&bootSector.numFATs, &bootSectorData[16], sizeof(BYTE));
    memcpy(&bootSector.fatSize32, &bootSectorData[36], sizeof(DWORD));

    // Kiểm tra dữ liệu hợp lệ
    if (bootSector.bytesPerSector == 0 || 
        bootSector.sectorsPerCluster == 0 ||
        bootSector.numFATs == 0 ||
        bootSector.fatSize32 == 0) {
        cerr << "Lỗi: Boot Sector không hợp lệ!" << endl;
        return false;
    }

    cout << "So byte/sector: " << bootSector.bytesPerSector << endl;
    cout << "So sector/cluster: " << (int)bootSector.sectorsPerCluster << endl;
    cout << "So sector danh rieng: " << bootSector.reservedSectors << endl;
    cout << "So bang FAT: " << (int)bootSector.numFATs << endl;
    cout << "So sector/bang FAT: " << bootSector.fatSize32 << endl;

    return true;
}