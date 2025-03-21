#include "fat32Recovery.h"
#include <iostream>

FileRecovery::FileRecovery(Fat32Parser &parser, DiskManager &disk) 
    : fatParser(parser), diskManager(disk) {}

// Tim file bi xoa trong Root Directory
bool FileRecovery::findDeletedFile(string &filename, DWORD &outCluster, DWORD &outFileSize) {
    DWORD rootCluster = fatParser.getRootCluster();
    DWORD bytesPerSector = fatParser.getBytesPerSector();
    DWORD sectorsPerCluster = fatParser.getSectorsPerCluster();
    DWORD clusterSize = bytesPerSector * sectorsPerCluster;

    vector<BYTE> buffer(clusterSize);
    
    if (!readCluster(rootCluster, buffer)) {
        cerr << "Loi doc Root Directory!\n";
        return false;
    }

    DirectoryEntry *entries = reinterpret_cast<DirectoryEntry *>(buffer.data());
    size_t numEntries = clusterSize / sizeof(DirectoryEntry);

    for (size_t i = 0; i < numEntries; i++) {
        if (entries[i].name[0] == 0xE5 && entries[i].attr == 32) { // File da bi xoa
            char recoveredName[12] = {0};
            recoveredName[0] = '?'; // Đặt dấu '?' cho ký tự bị mất
            memcpy(&recoveredName[1], &entries[i].name[1], 10); // Bỏ ký tự đầu

            string fileEntryName(recoveredName);
            cout << fileEntryName << endl;
            if (fileEntryName == filename) { // So sánh không lấy ký tự đầu
                outCluster = (entries[i].startClusterHigh << 16) | entries[i].startClusterLow;
                outFileSize = entries[i].fileSize;
                return true;
            }
        }
    }

    cerr << "Khong tim thay file bi xoa nao!\n";
    return false;
}

// Doc du lieu tu cluster
bool FileRecovery::readCluster(DWORD cluster, vector<BYTE> &buffer) {
    DWORD firstSector = fatParser.getReservedSectors() + (fatParser.getNumFATs() * fatParser.getFATSize32()) + ((cluster - 2) * fatParser.getSectorsPerCluster());
    DWORD sectorSize = fatParser.getBytesPerSector();
    
    buffer.resize(sectorSize * fatParser.getSectorsPerCluster());

    for (DWORD i = 0; i < fatParser.getSectorsPerCluster(); i++) {
        if (!diskManager.readSector(firstSector + i, buffer.data() + (i * sectorSize), sectorSize)) {
            cerr << "Loi doc cluster tai sector " << (firstSector + i) << "\n";
            return false;
        }
    }

    return true;
}

// 🔄 Khoi phuc file
bool FileRecovery::recoverFile(string &filename, const string &outputPath) {
    DWORD cluster, fileSize;
    
    if (!findDeletedFile(filename, cluster, fileSize)) {
        return false;
    }

    vector<BYTE> fileData(fileSize, 0);
    if (!readCluster(cluster, fileData)) {
        return false;
    }

    ofstream outFile(outputPath, ios::binary);
    if (!outFile) {
        cerr << "Khong the tao file " << outputPath << "\n";
        return false;
    }

    outFile.write(reinterpret_cast<char *>(fileData.data()), fileSize);
    outFile.close();

    cout << "File da duoc khoi phuc tai: " << outputPath << "\n";
    return true;
}

void FileRecovery::listDeletedFiles() {
    DWORD rootCluster = fatParser.getRootCluster();
    DWORD bytesPerSector = fatParser.getBytesPerSector();
    DWORD sectorsPerCluster = fatParser.getSectorsPerCluster();
    DWORD clusterSize = bytesPerSector * sectorsPerCluster;

    vector<BYTE> buffer(clusterSize);
    
    if (!readCluster(rootCluster, buffer)) {
        cerr << "Loi doc Root Directory!\n";
        return;
    }

    DirectoryEntry *entries = reinterpret_cast<DirectoryEntry *>(buffer.data());
    size_t numEntries = clusterSize / sizeof(DirectoryEntry);

    cout << "Danh sach file bi xoa:\n";
    cout << "--------------------------------------\n";
    cout << "Ten file        | Cluster | Kich thuoc\n";
    cout << "--------------------------------------\n";

    bool found = false;

    for (size_t i = 0; i < numEntries; i++) {
        if (entries[i].name[0] == 0xE5) { // File da bi xoa
            found = true;

            // Lấy tên file (định dạng 8.3)
            char name[12] = {0};
            memcpy(name, entries[i].name, 11);

            // Lấy số cluster đầu tiên
            DWORD startCluster = (entries[i].startClusterHigh << 16) | entries[i].startClusterLow;
            
            // In thông tin file bị xóa
            printf("%-15s | %-7u | %-10u\n", name, startCluster, entries[i].fileSize);
        }
    }

    if (!found) {
        cout << "Khong tim thay file bi xoa nao!\n";
    }
}
