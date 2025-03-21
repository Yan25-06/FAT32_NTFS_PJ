#include "fat32Recovery.h"

int main() {
    string drive = "E:";
    DiskManager disk(drive);

    if (!disk.openDrive()) 
        return -1;

    string fsType = disk.getFSType();
    cout << "File System Type " << string(drive.begin(), drive.end()) 
              << " : " << fsType << endl;
    
    Fat32Parser parser(disk);
    parser.printBootSectorInfo();
    
    string filename = "?HW3    SQL"; // File cần khôi phục (tên 8.3)
    string outputPath = "test.sql";

    FileRecovery recovery(parser, disk);
    recovery.listDeletedFiles();
    if (recovery.recoverFile(filename, outputPath)) {
        cout << "File da duoc khoi phuc thanh cong!\n";
    } else {
        cerr << "Khoi phuc that bai!\n";
    }

    return 0;
}
