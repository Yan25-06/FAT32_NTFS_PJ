#include "NTFSParser.h"

NTFSParser::NTFSParser(DiskManager &d) : diskManager(d) {
    getBasicInfo();
}

bool NTFSParser::getBasicInfo() {
    BYTE bootSectorData[512] = {0};
    DWORD bytesRead;

    if (!diskManager.readSector(0, bootSectorData, 512))
    {
        cerr << "Read boot sector failed!";
        return false;
    }
    memcpy(&bytesPerSector, &bootSectorData[11], sizeof(WORD));        
    memcpy(&sectorsPerCluster, &bootSectorData[13], sizeof(BYTE));     
    memcpy(&totalSectors, &bootSectorData[40], sizeof(ULONGLONG));      
    memcpy(&MFTStartCluster, &bootSectorData[48], sizeof(ULONGLONG));
    return true;
}

void NTFSParser::printBasicInfo() {
    cout << "Bytes per sector: " << bytesPerSector << endl;
    cout << "Sectors per cluster: " << sectorsPerCluster << endl;
    cout << "Total Sector: " << totalSectors << endl;
    cout << "MFT Start Cluster: " << MFTStartCluster << endl;
    DWORD offset = this->MFTStartCluster * this->sectorsPerCluster * this->bytesPerSector;
    // Ntfs_Data_Run *temp = this->MFTRecordList;
    // while (temp != NULL) {
    //     cout << temp->lcn << " " << temp->vcn << " " << temp->length << endl;
    //     temp = temp->next;
    // } 
}
