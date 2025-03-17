#ifndef DISKMANAGER_H
#define DISKMANAGER_H

#include <windows.h>
#include <iostream>
#include <string>

#define BYTES_PER_SECTOR 512

using namespace std;

class DiskManager {
    private:
    string driveLetter;
    HANDLE hDrive;

    public:
    bool openDrive();
    void closeDrive();
    bool readSector(DWORD sectorNumber, BYTE* buffer, DWORD sectorSize);
    string getFileSystemType();
    DiskManager(string);
    ~DiskManager();
};

#endif