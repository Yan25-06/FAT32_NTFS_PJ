#include "diskManager.h"

DiskManager::DiskManager(string drive) : driveLetter(drive), hDrive(INVALID_HANDLE_VALUE) {}

DiskManager::~DiskManager() {
    closeDrive();
}

bool DiskManager::openDrive() {
    string device = "\\\\.\\\\" + driveLetter;

    hDrive = CreateFileA( 
        device.c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
        OPEN_EXISTING, 0, NULL);

    if (hDrive == INVALID_HANDLE_VALUE) {
        cerr << "Loi mo o dia! Ma loi: " << GetLastError() << endl;
        return false;
    }
    return true;
}

string DiskManager::getFileSystemType() {
    if (hDrive == INVALID_HANDLE_VALUE) {
        cout << hDrive << endl;
        cerr << "O dia chua duoc mo!\n";
        return "UNKNOWN";
    }

    BYTE bootSector[BYTES_PER_SECTOR] = {0};

    if(!readSector(0, bootSector, BYTES_PER_SECTOR))
    {
        return "UNKNOWN";
    }

    if (memcmp(bootSector + 0x03, "NTFS    ", 8) == 0) 
        return "NTFS";
    if (memcmp(bootSector + 0x52, "FAT32   ", 8) == 0) 
        return "FAT32";

    return "UNKNOWN";
}

void DiskManager::closeDrive() {
    if (hDrive != INVALID_HANDLE_VALUE) {
        CloseHandle(hDrive);
        hDrive = INVALID_HANDLE_VALUE;
    }
}
bool DiskManager::readSector(DWORD sectorNumber, BYTE* buffer, DWORD sectorSize) {
    LARGE_INTEGER sectorOffset;
    sectorOffset.QuadPart = sectorNumber * sectorSize;  // Tính vị trí cần đọc (byte)
    
    // Di chuyển con trỏ file đến sector cần đọc
    if (SetFilePointerEx(hDrive, sectorOffset, NULL, FILE_BEGIN) == 0) {
        cerr << "Loi SetFilePointerEx! Ma loi: " << GetLastError() << endl;
        return false;
    }

    DWORD bytesRead;
    if (!ReadFile(hDrive, buffer, sectorSize, &bytesRead, NULL) || bytesRead != sectorSize) {
        cerr << "Loi doc sector! Ma loi: " << GetLastError() << endl;
        return false;
    }

    return true; 
}

