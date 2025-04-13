#include "fat32Recovery.h"
#include <iostream>

Fat32Recovery::Fat32Recovery(Fat32Parser &parser, DiskManager &disk) 
    : fatParser(parser), diskManager(disk) {}

bool Fat32Recovery::findDeletedFile(string &filename, DWORD &outCluster, DWORD &outFileSize, DWORD startCluster, bool isDeletedFolder = 0) {
    DWORD clusterSize = fatParser.getClusterSize();

    vector<BYTE> buffer(clusterSize);

    if (!fatParser.readCluster(startCluster, buffer)) {
        cerr << "Loi doc Root Directory!\n";
        return false;
    }

    DirectoryEntry *entries = reinterpret_cast<DirectoryEntry *>(buffer.data());
    size_t numEntries = clusterSize / sizeof(DirectoryEntry); 
    
    for (size_t i = 0; i < numEntries; i++) {
        // cout << i << "," << entries[i].name << endl;
        if (!isDeletedFolder)
        {
            if (entries[i].name[0] == 0xE5 && entries[i].attr != 0x0F && entries[i].attr != 0x10) 
            {
                string fileEntryName = extractFileName(entries, filename[0], i);
                
                if (fileEntryName == filename) { 
                    // cout << fileEntryName << endl;
                    outCluster = (entries[i].startClusterHigh << 16) | entries[i].startClusterLow;
                    outFileSize = entries[i].fileSize;
                    // cout << "Cluster: " << outCluster << ", File Size: " << outFileSize << endl;
                    return true;
                }
            }
            if ((entries[i].attr & 0x10) && entries[i].name[0] != 0x00 && entries[i].name[0] != 0x2E) 
            {
                DWORD subDirCluster = (entries[i].startClusterHigh << 16) | entries[i].startClusterLow;
                bool isDeleted = (entries[i].name[0] == 0xE5) ? 1 : 0;
                if (subDirCluster != 0 && findDeletedFile(filename, outCluster, outFileSize, subDirCluster, isDeleted)) {
                    return true; // Nếu tìm thấy file trong thư mục con, dừng ngay
                }
            }
        }
        else
        {
            if (entries[i].attr != 0x0F && entries[i].attr != 0x10) 
            {
                string fileEntryName = extractFileName(entries, filename[0], i);
                // cout << fileEntryName << endl;
                if (fileEntryName == filename) {
                    outCluster = (entries[i].startClusterHigh << 16) | entries[i].startClusterLow;
                    outFileSize = entries[i].fileSize;
                    return true;
                }
            }
            if ((entries[i].attr & 0x10) && entries[i].name[0] != 0x00 && entries[i].name[0] != 0x2E) 
            {
                DWORD subDirCluster = (entries[i].startClusterHigh << 16) | entries[i].startClusterLow;
                if (subDirCluster != 0 && findDeletedFile(filename, outCluster, outFileSize, subDirCluster, 1)) {
                    return true; // Nếu tìm thấy file trong thư mục con, dừng ngay
                }
            }
        }
    }
    return false;
}
// 🔄 Khoi phuc file
bool Fat32Recovery::recoverFile(string &filename) {
    string outputPath = diskManager.getDriveLetter() + "\\" + "recovered_" + filename;
    DWORD cluster, fileSize;
    
    if (filename.size() <= SFN_SIZE)
        filename = convertToSFN(filename);

    if (!findDeletedFile(filename, cluster, fileSize, fatParser.getRootCluster())) {
        return false;
    }

    vector<BYTE> fileData(fileSize, 0);
    if (!fatParser.readCluster(cluster, fileData)) {
        cerr << "Loi doc file!\n";
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

void Fat32Recovery::listDeletedFiles() {
    cout << "Danh sach file bi xoa:\n";
    cout << "-------------------------------------------\n";
    cout << "Ten file             | Cluster | Kich thuoc\n";
    cout << "-------------------------------------------\n";
    
    listDeletedFiles(fatParser.getRootCluster());
}

void Fat32Recovery::listDeletedFiles(DWORD currentCluster) {
    DWORD clusterSize = fatParser.getClusterSize();

    vector<BYTE> buffer(clusterSize);

    // Đọc cluster của thư mục hiện tại
    if (!fatParser.readCluster(currentCluster, buffer)) {
        cerr << "Loi doc thu muc!\n";
        return;
    }

    DirectoryEntry *entries = reinterpret_cast<DirectoryEntry *>(buffer.data());
    size_t numEntries = clusterSize / sizeof(DirectoryEntry);

    for (size_t i = 0; i < numEntries; i++) 
    {
        char nameWithNull[SFN_SIZE + 1] = {0};
        memcpy(nameWithNull, entries[i].name, SFN_SIZE);
        string entryName(nameWithNull);
        trim(entryName);

        if (entries[i].name[0] == 0xE5 && entries[i].attr != 0x0F) 
        {
            // Lấy số cluster đầu tiên
            DWORD startCluster = (entries[i].startClusterHigh << 16) | entries[i].startClusterLow;
            // In thông tin file bị xóa
            printf("%-20s | %-7u | %-10u\n", entryName.c_str(), startCluster, entries[i].fileSize);
        }

        // Nếu là thư mục con (attr & 0x10) và không phải "." hay ".."
        if (entries[i].attr & 0x10 && entries[i].name[0] != 0x00 && entries[i].name[0] != 0x2E) 
        {
            DWORD subDirCluster = (entries[i].startClusterHigh << 16) | entries[i].startClusterLow;
            if (subDirCluster != 0 && subDirCluster != currentCluster) {
                listDeletedFiles(subDirCluster); // Đệ quy vào thư mục con
            }
        }
    }
}

void Fat32Recovery::listFiles() {
    cout << "Danh sach file:\n";
    cout << "-------------------------------------------\n";
    cout << "Ten file             | Cluster | Kich thuoc\n";
    cout << "-------------------------------------------\n";

    listFiles(fatParser.getRootCluster());
}

void Fat32Recovery::listFiles(DWORD startCluster) {
    DWORD clusterSize = fatParser.getClusterSize();

    vector<BYTE> buffer(clusterSize);

    if (!fatParser.readCluster(startCluster, buffer)) {
        cerr << "Loi doc thu muc o Cluster " << startCluster << "!\n";
        return;
    }

    DirectoryEntry *entries = reinterpret_cast<DirectoryEntry *>(buffer.data());
    size_t numEntries = clusterSize / sizeof(DirectoryEntry);

    for (size_t i = 0; i < numEntries; i++) {
        if (entries[i].name[0] == 0x00) break; // Hết danh sách

        char nameWithNull[SFN_SIZE + 1] = {0};
        memcpy(nameWithNull, entries[i].name, SFN_SIZE);
        string entryName(nameWithNull);
        trim(entryName);

        DWORD fileCluster = (entries[i].startClusterHigh << 16) | entries[i].startClusterLow;
        DWORD fileSize = entries[i].fileSize;

        if (entries[i].name[0] != 0xE5 && entries[i].attr != 0x0F && entries[i].attr != 0x10) 
        {
            printf("%-20s | %-7u | %-10u\n", entryName.c_str(), fileCluster, fileSize);
        }
        // Nếu là thư mục con, đệ quy vào trong
        else if (entries[i].attr & 0x10 && entries[i].name[0] != 0xE5 && entries[i].name[0] != 0x2E) 
        {   
            printf("[DIR] %-14s | %-7u | %-10u\n", entryName.c_str(), fileCluster, fileSize);
            listFiles(fileCluster); // Đệ quy
        }
    }
}

