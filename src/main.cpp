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
    vector<DirectoryEntry> e = parser.readRootDirectory();

    for (int i = 0; i < e.size(); i++)
    {
        cout << e[i].name << endl;
    }
    cout << endl;
    Fat32Recovery recover(parser);
    vector<string> deletedFile = recover.scanDeletedFiles();

    for (int i = 0; i < deletedFile.size(); i++)
    {
        cout << deletedFile[i] << endl;
    }
    return 0;
}
