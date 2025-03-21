#ifndef FILERECOVERY_H
#define FILERECOVERY_H

#include "fat32parser.h"
#include "diskManager.h"
#include <fstream>
#include <vector>

class FileRecovery {
private:
    Fat32Parser &fatParser;
    DiskManager &diskManager;
    
    bool findDeletedFile(string &filename, DWORD &outCluster, DWORD &outFileSize);
    bool readCluster(DWORD cluster, vector<BYTE> &buffer);

public:
    FileRecovery(Fat32Parser &parser, DiskManager &disk);
    bool recoverFile(string & filename, const string &outputPath);
    void listDeletedFiles();
};

#endif
