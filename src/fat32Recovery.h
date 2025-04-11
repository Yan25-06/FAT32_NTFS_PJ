#ifndef FAT32RECOVERY_H
#define FAT32RECOVERY_H

#include "diskManager.h"
#include "utils.h"

#include <fstream>
#include <vector>

class Fat32Recovery {
private:
    Fat32Parser &fatParser;
    DiskManager &diskManager;
    
    bool findDeletedFile(string &filename, DWORD &outCluster, DWORD &outFileSize, DWORD startCluster, bool isDeletedFolder);
    void listDeletedFiles(DWORD currentCluster);
    void listFiles(DWORD startCluster);
public:
    Fat32Recovery(Fat32Parser &parser, DiskManager &disk);
    bool recoverFile(string & filename, const string &drive);
    void listDeletedFiles();
    void listFiles();
};

#endif
