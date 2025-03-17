#include "fat32Parser.h"

int main() {
    string drive = "E:";
    DiskManager disk(drive);

    if (!disk.openDrive()) 
        return -1;

    string fsType = disk.getFileSystemType();
    cout << "He thong cua o " << string(drive.begin(), drive.end()) 
              << " : " << fsType << endl;
    
    FAT32_BootSector bootSector;
    Fat32Parser parser(disk, bootSector);

    // Đọc Boot Sector và kiểm tra kết quả
    if (!parser.readBootSector()) {
        cerr << "Loi khi doc" << endl;
    }
    return 0;
}
