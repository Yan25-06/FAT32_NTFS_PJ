#include "fat32Recovery.h"
#include <iostream>

Fat32Recovery::Fat32Recovery(Fat32Parser &parser, DiskManager &disk) 
    : fatParser(parser), diskManager(disk) {}

bool Fat32Recovery::findDeletedFile(string &filename, DWORD &outCluster, DWORD &outFileSize, DWORD startCluster) {
    DWORD bytesPerSector = fatParser.getBytesPerSector();
    DWORD sectorsPerCluster = fatParser.getSectorsPerCluster();
    DWORD clusterSize = bytesPerSector * sectorsPerCluster;

    vector<BYTE> buffer(clusterSize);

    if (!fatParser.readCluster(startCluster, buffer)) {
        cerr << "Loi doc Root Directory!\n";
        return false;
    }

    DirectoryEntry *entries = reinterpret_cast<DirectoryEntry *>(buffer.data());
    size_t numEntries = clusterSize / sizeof(DirectoryEntry);  cout << "Number of entries in this cluster: " << numEntries << endl;
    
    for (size_t i = 0; i < numEntries; i++) {
        cout << i << "," << entries[i].name << endl;
        if (entries[i].name[0] == 0xE5 && entries[i].attr != 0x0F && entries[i].attr != 0x10) { // File da bi xoa
            string fileEntryName = extractFileName(entries, filename, i);
            if (fileEntryName == filename) { // So sánh không lấy ký tự đầu
                outCluster = (entries[i].startClusterHigh << 16) | entries[i].startClusterLow;
                outFileSize = entries[i].fileSize;
                return true;
            }
        }
        
        if ((entries[i].attr & 0x10) && entries[i].name[0] != 0xE5 && entries[i].name[0] != 0x00) {
            char nameWithNull[12];  // Đảm bảo có không gian cho dấu kết thúc '\0'
            memcpy(nameWithNull, entries[i].name, 11); 
            nameWithNull[11] = '\0'; 
            string dirName(nameWithNull);

            if (dirName == ".          " || dirName == "..         ") {
                continue; // Nếu là thư mục '.' hoặc '..', bỏ qua và tiếp tục
            }
            DWORD subDirCluster = (entries[i].startClusterHigh << 16) | entries[i].startClusterLow;
            if (subDirCluster != 0 && findDeletedFile(filename, outCluster, outFileSize, subDirCluster)) {
                return true; // Nếu tìm thấy file trong thư mục con, dừng ngay
            }
        }
    }

    cerr << "Khong tim thay file bi xoa nao!\n";
    return false;
}
// 🔄 Khoi phuc file
bool Fat32Recovery::recoverFile(string &filename, const string &drive) {
    string outputPath = drive + "\\" + "recovered_" + filename;
    DWORD cluster, fileSize;
    
    filename = convertToSFN(filename);

    if (!findDeletedFile(filename, cluster, fileSize, fatParser.getRootCluster())) {
        return false;
    }

    vector<BYTE> fileData(fileSize, 0);
    if (!fatParser.readCluster(cluster, fileData)) {
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
    DWORD bytesPerSector = fatParser.getBytesPerSector();
    DWORD sectorsPerCluster = fatParser.getSectorsPerCluster();
    DWORD clusterSize = bytesPerSector * sectorsPerCluster;

    vector<BYTE> buffer(clusterSize);

    // Đọc cluster của thư mục hiện tại
    if (!fatParser.readCluster(currentCluster, buffer)) {
        cerr << "Loi doc thu muc!\n";
        return;
    }

    DirectoryEntry *entries = reinterpret_cast<DirectoryEntry *>(buffer.data());
    size_t numEntries = clusterSize / sizeof(DirectoryEntry);

    bool found = false;

    for (size_t i = 0; i < numEntries; i++) {
        if (entries[i].name[0] == 0xE5 && entries[i].attr != 0x0F) { // File đã bị xóa
            found = true;

            // Lấy tên file (định dạng 8.3)
            char name[12] = {0};
            memcpy(name, entries[i].name, 11);

            // Lấy số cluster đầu tiên
            DWORD startCluster = (entries[i].startClusterHigh << 16) | entries[i].startClusterLow;
            
            // In thông tin file bị xóa
            printf("%-20s | %-7u | %-10u\n", name, startCluster, entries[i].fileSize);
        }

        // Nếu là thư mục con (attr & 0x10) và không phải "." hay ".."
        if ((entries[i].attr & 0x10) && entries[i].name[0] != 0xE5 && entries[i].name[0] != 0x00) {
            DWORD subDirCluster = (entries[i].startClusterHigh << 16) | entries[i].startClusterLow;
            if (subDirCluster != 0 && subDirCluster != currentCluster) {
                listDeletedFiles(subDirCluster); // Đệ quy vào thư mục con
            }
        }
    }

    if (!found && currentCluster == fatParser.getRootCluster()) {
        cout << "Khong tim thay file bi xoa nao!\n";
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
    DWORD bytesPerSector = fatParser.getBytesPerSector();
    DWORD sectorsPerCluster = fatParser.getSectorsPerCluster();
    DWORD clusterSize = bytesPerSector * sectorsPerCluster;

    vector<BYTE> buffer(clusterSize);

    if (!fatParser.readCluster(startCluster, buffer)) {
        cerr << "Loi doc thu muc o Cluster " << startCluster << "!\n";
        return;
    }

    DirectoryEntry *entries = reinterpret_cast<DirectoryEntry *>(buffer.data());
    size_t numEntries = clusterSize / sizeof(DirectoryEntry);

    for (size_t i = 0; i < numEntries; i++) {
        if (entries[i].name[0] == 0x00) break; // Hết danh sách

        char nameWithNull[12] = {0};
        memcpy(nameWithNull, entries[i].name, 11);
        string entryName(nameWithNull);
        trim(entryName);
        DWORD fileCluster = (entries[i].startClusterHigh << 16) | entries[i].startClusterLow;
        DWORD fileSize = entries[i].fileSize;

        if (entries[i].name[0] != 0xE5 && entries[i].attr != 0x0F && entries[i].attr != 0x10) 
        {
            printf("%-20s | %-7u | %-10u\n", entryName.c_str(), fileCluster, fileSize);
        }
        // Nếu là thư mục con, đệ quy vào trong
        else if (entries[i].attr & 0x10) 
        {
            if (entryName == "." || entryName == "..") 
                continue;
            
            printf("[DIR] %-14s | %-7u | %-10u\n", entryName.c_str(), fileCluster, fileSize);
            listFiles(fileCluster); // Đệ quy
        }
    }
}

