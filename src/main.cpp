#include "fat32Parser.h"

int main() {
    string drive = "E:";
    DiskManager disk(drive);

    if (!disk.openDrive()) 
        return -1;

    string fsType = disk.getFSType();
    cout << "File System Type " << string(drive.begin(), drive.end()) 
              << " : " << fsType << endl;
    
    Fat32Parser parser(disk);

    return 0;
}
