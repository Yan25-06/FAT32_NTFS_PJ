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
        cerr << "Failed to open disk, ERROR: " << GetLastError() << endl;
        return false;
    }
    fileSystemType = getFileSystemType();
    return true;
}

string DiskManager::getFileSystemType() {
    if (hDrive == INVALID_HANDLE_VALUE) {
        cerr << "Drive is not open!\n";
        return "UNKNOWN";
    }

    BYTE bootSector[512] = {0};
    DWORD bytesRead;

    SetFilePointerEx(hDrive, {0}, NULL, FILE_BEGIN);
    if (!ReadFile(hDrive, bootSector, 512, &bytesRead, NULL) || bytesRead != 512) {
        cerr << "Error reading Boot Sector\n";
        return "UNKNOWN";
    }    

    if (memcmp(bootSector + 0x03, "NTFS    ", 8) == 0) 
        return "NTFS";
    if (memcmp(bootSector + 0x52, "FAT32   ", 8) == 0) 
        return "FAT32";

    return "UNKNOWN";
}
string DiskManager::getFSType()
{
    return fileSystemType;
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
        cerr << "SetFilePointerEx failed! ERROR: " << GetLastError() << endl;
        return false;
    }

    DWORD bytesRead;
    if (!ReadFile(hDrive, buffer, sectorSize, &bytesRead, NULL) || bytesRead != sectorSize) {
        cerr << "Read sector failed! ERROR: " << GetLastError() << endl;
        return false;
    }

    return true; 
}
bool DiskManager::writeSector(DWORD sectorNumber, const BYTE* buffer, DWORD sectorSize) {
    LARGE_INTEGER sectorOffset;
    sectorOffset.QuadPart = sectorNumber * sectorSize;  // Tính vị trí cần ghi (byte)

    // Di chuyển con trỏ file đến sector cần ghi
    if (SetFilePointerEx(hDrive, sectorOffset, NULL, FILE_BEGIN) == 0) {
        cerr << "SetFilePointerEx failed! ERROR: " << GetLastError() << endl;
        return false;
    }

    DWORD bytesWritten;
    if (!WriteFile(hDrive, buffer, sectorSize, &bytesWritten, NULL) || bytesWritten != sectorSize) {
        cerr << "Write sector failed! ERROR: " << GetLastError() << endl;
        return false;
    }

    return true;
}

bool DiskManager::readBytes(DWORD offset, BYTE* buffer, DWORD byteCount) {
    LARGE_INTEGER byteOffset;
    byteOffset.QuadPart = offset;  // Vị trí cần đọc (tính theo byte)

    // Di chuyển con trỏ file đến vị trí cần đọc
    if (SetFilePointerEx(hDrive, byteOffset, NULL, FILE_BEGIN) == 0) {
        cerr << "SetFilePointerEx failed! ERROR: " << GetLastError() << endl;
        return false;
    }

    DWORD bytesRead;
    if (!ReadFile(hDrive, buffer, byteCount, &bytesRead, NULL) || bytesRead != byteCount) {
        cerr << "ReadFile failed! ERROR: " << GetLastError() << endl;
        return false;
    }

    return true;
}


