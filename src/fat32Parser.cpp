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
    memcpy(&bootSector.rootCluster, &bootSectorData[44], sizeof(DWORD));

    // Kiểm tra dữ liệu hợp lệ
    if (bootSector.bytesPerSector == 0 || 
        bootSector.sectorsPerCluster == 0 ||
        bootSector.numFATs == 0 ||
        bootSector.fatSize32 == 0) {
        cerr << "Error: Boot Sector is invalid\n" << endl;
        return false;
    }
    return true;
}
void Fat32Parser::printBootSectorInfo()
{
    cout << "So byte/sector: " << bootSector.bytesPerSector << endl;
    cout << "So sector/cluster: " << (int)bootSector.sectorsPerCluster << endl;
    cout << "So sector danh rieng: " << bootSector.reservedSectors << endl;
    cout << "So bang FAT: " << (int)bootSector.numFATs << endl;
    cout << "So sector/bang FAT: " << bootSector.fatSize32 << endl;
    cout << "Cluster bat dau cua RDET: " << bootSector.rootCluster << endl;
}
bool Fat32Parser::readCluster(DWORD cluster, BYTE* buffer) {
    if (cluster < 2) return false;  // Cluster 0 và 1 không hợp lệ

    DWORD sector = bootSector.reservedSectors + (bootSector.numFATs * bootSector.fatSize32)
                   + (cluster - 2) * bootSector.sectorsPerCluster;
    
    for (int i = 0; i < bootSector.sectorsPerCluster; i++) {
        if (!disk.readSector(sector + i, buffer + (i * bootSector.bytesPerSector), bootSector.bytesPerSector)) {
            cerr << "Lỗi đọc sector " << (sector + i) << endl;
            return false;
        }
    }

    return true;
}
DWORD Fat32Parser::getNextCluster(DWORD currentCluster) {
    DWORD fatOffset = currentCluster * 4;  // Mỗi entry trong FAT32 chiếm 4 byte
    DWORD sector = bootSector.reservedSectors + (fatOffset / bootSector.bytesPerSector);
    DWORD offset = fatOffset % bootSector.bytesPerSector;
    
    BYTE fatBuffer[bootSector.bytesPerSector];
    if (!disk.readSector(sector, fatBuffer, bootSector.bytesPerSector)) {
        cerr << "Lỗi đọc bảng FAT tại sector " << sector << endl;
        return 0xFFFFFFFF;
    }

    DWORD nextCluster = *reinterpret_cast<DWORD*>(&fatBuffer[offset]) & 0x0FFFFFFF;
    return nextCluster >= 0x0FFFFFF8 ? 0xFFFFFFFF : nextCluster;
}

vector<DirectoryEntry> Fat32Parser::readRootDirectory() {
    vector<DirectoryEntry> entries;
    DWORD currentCluster = bootSector.rootCluster;

    while (currentCluster < 0x0FFFFFF8) { // FAT32 EOF
        BYTE buffer[bootSector.bytesPerSector * bootSector.sectorsPerCluster];
        if (!readCluster(currentCluster, buffer)) {
            cerr << "Lỗi đọc cluster " << currentCluster << endl;
            return entries;
        }

        DirectoryEntry* entry = reinterpret_cast<DirectoryEntry*>(buffer);
        for (int i = 0; i < (bootSector.bytesPerSector * bootSector.sectorsPerCluster) / sizeof(DirectoryEntry); i++) {
            if (entry[i].name[0] == 0x00) break;  // Entry trống, kết thúc danh sách
            entries.push_back(entry[i]);
        }

        currentCluster = getNextCluster(currentCluster); // Di chuyển sang cluster tiếp theo
    }

    return entries;
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

