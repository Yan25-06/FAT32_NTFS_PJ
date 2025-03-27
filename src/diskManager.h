#ifndef DISKMANAGER_H
#define DISKMANAGER_H

#include <windows.h>
#include <iostream>
#include <string>

using namespace std;

class DiskManager {
    private:
    string driveLetter;
    HANDLE hDrive;
    string fileSystemType;

    string getFileSystemType();
    public:
    bool openDrive();
    void closeDrive();
    bool readSector(DWORD sectorNumber, BYTE* buffer, DWORD sectorSize);
    bool writeSector(DWORD sectorNumber, const BYTE* buffer, DWORD sectorSize);
    bool readBytes(DWORD offset, BYTE* buffer, DWORD byteCount);
    string getFSType();

    DiskManager(string);
    ~DiskManager();
};

#endif