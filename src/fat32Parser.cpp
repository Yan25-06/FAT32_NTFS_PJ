#include "fat32Parser.h"

Fat32Parser::Fat32Parser(DiskManager &d) : disk(d) {
    readBootSector();
}

bool Fat32Parser::readBootSector() {
    BYTE bootSectorData[512] = {0}; // Buffer để chứa dữ liệu Boot Sector
    DWORD bytesRead;

    if (!disk.readSector(0, bootSectorData, 512))
    {
        cerr << "Read boot sector failed!";
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
        cerr << "Error: Boot Sector is invalid\n" << endl;
        return false;
    }

    cout << "So byte/sector: " << bootSector.bytesPerSector << endl;
    cout << "So sector/cluster: " << (int)bootSector.sectorsPerCluster << endl;
    cout << "So sector danh rieng: " << bootSector.reservedSectors << endl;
    cout << "So bang FAT: " << (int)bootSector.numFATs << endl;
    cout << "So sector/bang FAT: " << bootSector.fatSize32 << endl;

    return true;
}

WORD Fat32Parser::getBytesPerSector() const {
    return bootSector.bytesPerSector;
}

BYTE Fat32Parser::getSectorsPerCluster() const {
    return bootSector.sectorsPerCluster;
}

WORD Fat32Parser::getReservedSectors() const {
    return bootSector.reservedSectors;
}

BYTE Fat32Parser::getNumFATs() const {
    return bootSector.numFATs;
}

DWORD Fat32Parser::getFATSize32() const {
    return bootSector.fatSize32;
}