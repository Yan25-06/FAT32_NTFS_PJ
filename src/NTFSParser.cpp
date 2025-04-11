#include "NTFSParser.h"

// ham ghi ra file de test
void WriteHexToFile(const char *filename, UCHAR *buf, int size) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        printf("Khong mo duoc file!\n");
        return;
    }

    // Ghi dòng header cho index cột (00 01 02 ... 0F)
    fprintf(file, "Offset  |  00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F  | ASCII\n");
    fprintf(file, "--------+------------------------------------------------+----------------\n");

    for (int i = 0; i < size; i += 16) {
        // Ghi offset (chỉ mục dòng)
        fprintf(file, "%06X  |  ", i);

        // Ghi 16 byte dữ liệu dạng hex
        for (int j = 0; j < 16 && (i + j) < size; j++) {
            if (j == 8) fprintf(file, " "); // Ngăn cách nhóm 8 byte
            fprintf(file, "%02X ", buf[i + j]);
        }

        // Thêm khoảng trắng nếu dòng cuối có ít hơn 16 byte
        for (int j = size - i; j < 16; j++) {
            if (j == 8) fprintf(file, " "); // Giữ format
            fprintf(file, "   ");
        }

        fprintf(file, " | ");  // Ngăn cách giữa hex và ASCII

        // Ghi 16 byte dữ liệu dạng ASCII nếu có thể đọc được, ngược lại ghi '.'
        for (int j = 0; j < 16 && (i + j) < size; j++) {
            UCHAR c = buf[i + j];
            fprintf(file, "%c", (c >= 32 && c <= 126) ? c : '.');
        }

        fprintf(file, " |\n");
    }

    fclose(file);
}

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
    
    this->bytesPerSector = *(uint16_t*)&bootSectorData[11];  
    this->sectorsPerCluster = bootSectorData[13];  
    this->totalSectors = *(uint64_t*)&bootSectorData[40];
    this->MFTStartCluster = *(uint64_t*)&bootSectorData[48];  
    this->getMFTRunList();
    return true;
}

void NTFSParser::printBasicInfo() {
    cout << "Bytes per sector: " << bytesPerSector << endl;
    cout << "Sectors per cluster: " << sectorsPerCluster << endl;
    cout << "Total Sector: " << totalSectors << endl;
    cout << "MFT Start Cluster: " << MFTStartCluster << endl;
    DWORD offset = this->MFTStartCluster * this->sectorsPerCluster * this->bytesPerSector;
}



bool NTFSParser::getAttrValue(NTFS_ATTRDEF prmAttrTitle, BYTE prmBuf[], BYTE *prmAttrValue)
{
    // Kiểm tra chữ ký của tệp (MFT record signature)
    UINT32 fileSign = *(UINT32*)prmBuf;
    if (fileSign != 0x454C4946)  // "FILE" trong mã ASCII
    {
        cout << "Khong hop le" << endl;
        return 0;  // Không hợp lệ
    }
    UINT16 flag = *(UINT16*)&prmBuf[0x16];  // Cờ trạng thái file (1: tập tin, 2: thư mục, 0 hoặc 2 có thể là đã bị xóa)
    // Lấy vị trí của thuộc tính đầu tiên trong bản ghi MFT (tại offset 20 byte)
    UINT32 attrOffset = *(UINT16*)&prmBuf[20];
    UINT32 attrLen = 0; // Biến lưu độ dài thuộc tính
    // Duyệt qua danh sách các thuộc tính trong bản ghi MFT
    while (attrOffset < 1024)  // Một bản ghi MFT có kích thước cố định là 1024 byte
    {
        // Nếu thuộc tính kết thúc (giá trị 0xFF), dừng lại
        if (prmBuf[attrOffset] == 0xFF)
        {
            // cout << "Het thuoc tinh" << endl;
            return 0;
        }
        // Lấy độ dài của thuộc tính hiện tại (2 byte tại offset +4)
        attrLen = *(UINT16*)&prmBuf[attrOffset + 4];
        // cout << *(UINT16*)&prmBuf[attrOffset] << " " << prmAttrTitle << endl;
        // cout << attrOffset << endl;
        // Nếu thuộc tính hiện tại là thuộc tính cần tìm
        if (prmBuf[attrOffset] == prmAttrTitle && prmAttrTitle != ATTR_FILE_NAME)
        {
            // Sao chép toàn bộ dữ liệu của thuộc tính vào prmAttrValue
            memcpy(prmAttrValue, prmBuf + attrOffset, attrLen);
            return 1;  // Thành công
        }
        else if (prmBuf[attrOffset] == prmAttrTitle && prmBuf[attrOffset+0x59]!=2) {
            memcpy(prmAttrValue, prmBuf + attrOffset, attrLen);
            return 1;  // Thành công
        }
        // Chuyển sang thuộc tính tiếp theo
        attrOffset += attrLen;
    }
    return 0;  // Không tìm thấy thuộc tính
}

void NTFSParser::getMFTRunList() {
    BYTE tmpBuf[1024] = {0};      // Bộ đệm tạm để lưu dữ liệu đọc từ đĩa
    BYTE tmpAttrValue[1024] = {0};// Bộ đệm tạm để lưu giá trị thuộc tính
    
    // Đọc dữ liệu từ cluster bắt đầu của MFT
    DWORD offset = this->MFTStartCluster * this->sectorsPerCluster * this->bytesPerSector;
    if (!diskManager.readBytes(offset, tmpBuf, 1024)) {
        cout << "Loi readBytes " << endl;
        return;
    }
    // Lấy giá trị độ lệch của số thứ tự cập nhật (Update Sequence Number - USN)
    UINT16 usnOffset = *(UINT16*)&tmpBuf[4]; 
    // Khôi phục hai byte cuối của sector đầu tiên trong bản ghi MFT
    memcpy(tmpBuf + 0x1FE, tmpBuf + usnOffset + 2, 2);
    // Khôi phục hai byte cuối của sector thứ hai trong bản ghi MFT
    memcpy(tmpBuf + 0x3FE, tmpBuf + usnOffset + 4, 2);
    if (!getAttrValue(ATTR_DATA, tmpBuf, tmpAttrValue)) {
        cout << "Loi getAttrValue" << endl;
        return;
    }
    UINT16 dataRunOffset = *(UINT16*)&tmpAttrValue[0x20];
    this->getDataRunList(tmpAttrValue, dataRunOffset, MFTRecordList);
}

void NTFSParser::getDataRunList(BYTE *prmBuf, WORD prmRunListOffset, vector<Ntfs_Data_Run> &prmList) 
{
    ULONGLONG index_alloc_size = 0; // Lưu kích thước của data run (số lượng cụm)
    ULONGLONG lcn = 0; // Lưu Logical Cluster Number (LCN)
    int j;
    WORD bufOffset = prmRunListOffset; 
    DWORD temp = 0;

    // Mỗi mục trong danh sách Run có byte đầu tiên:
    // - 4 bit thấp: số byte dùng để lưu kích thước cụm
    // - 4 bit cao: số byte dùng để lưu LCN (vị trí bắt đầu)
    int index = 0;
    while (true)
    {
        index_alloc_size = 0;
        // Tính toán số lượng cụm được cấp phát (Length)
        for (j = 0; j < prmBuf[bufOffset] % 16; j++)
        {
            index_alloc_size += prmBuf[bufOffset + 1 + j] * (ULONGLONG)pow((long double)256, j);
        }
        
        // Tính toán giá trị cụm bắt đầu (LCN - Logical Cluster Number)
        if (prmBuf[bufOffset + prmBuf[bufOffset] % 16 + prmBuf[bufOffset] / 16] > 127)  
        {   
            // Nếu là số âm (bit cao nhất là 1), thì tính bù hai
            for (j = 0; j < prmBuf[bufOffset] / 16; j++)
            {
                temp = ~prmBuf[bufOffset + prmBuf[bufOffset] % 16 + 1 + j]; // Lấy bù
                temp = temp & 255;
                lcn = lcn - temp * (ULONGLONG)pow((long double)256, j);
            }
            lcn = lcn - 1; // Trừ đi 1 để hoàn tất tính bù
        }
        else
        {
            // Nếu là số dương, tính toán trực tiếp
            for (j = 0; j < prmBuf[bufOffset] / 16; j++)
            {
                lcn += prmBuf[bufOffset + prmBuf[bufOffset] % 16 + 1 + j] * (ULONGLONG)pow((long double)256, j);
            }
        }
        Ntfs_Data_Run datarun;
        datarun.lcn = lcn;
        datarun.vcn = 0;
        datarun.length = index_alloc_size; // Số cụm mà Run chiếm dụng
        if (index != 0) {
            datarun.vcn += prmList[index - 1].length; // Tính toán VCN tiếp theo
        }
        prmList.push_back(datarun);
        index++;
        // Tính toán độ lệch của Data Run tiếp theo
        bufOffset = bufOffset + prmBuf[bufOffset] / 16 + prmBuf[bufOffset] % 16 + 1;
        if (0 == prmBuf[bufOffset])
        {
            // Kết thúc danh sách Run List
            break;
        }
    }
}

void NTFSParser::readMFT() {
    BYTE tmpBuf[1024] = {0};      // Bộ đệm tạm để lưu dữ liệu đọc từ đĩa
    BYTE tmpAttrValue[1024] = {0};// Bộ đệm tạm để lưu giá trị thuộc tính
    
    // Đọc dữ liệu từ cluster bắt đầu của MFT
    DWORD offset;
    int n = MFTRecordList.size();
    int numRecord;
    for (int i = 0; i < n; i++) {
        numRecord = MFTRecordList[i].length * sectorsPerCluster * bytesPerSector / 1024;
        offset = MFTRecordList[i].lcn * sectorsPerCluster * bytesPerSector;
        if (!diskManager.readBytes(offset, tmpBuf, 1024)) {
            cout << "Loi readBytes " << endl;
            return;
        }
        for (int j = 0 ; j < numRecord; j++) {
            if (memcmp(tmpBuf, "FILE", 4) == 0) {
                if (getAttrValue(ATTR_FILE_NAME, tmpBuf, tmpAttrValue)) {
                    BYTE tmpFileLen = tmpAttrValue[0x58];
                    wchar_t	tmpFileName[MAX_PATH] = {0};
                    memset(tmpFileName,0,sizeof(tmpFileName));
                    memcpy(tmpFileName,tmpAttrValue+0x5A,tmpFileLen<<1);
                    cout << ConvertWCharToString(tmpFileName) << endl;
                }
            }
            offset += 1024;
            memset(tmpBuf,0,sizeof(tmpBuf));
            memset(tmpAttrValue,0,sizeof(tmpAttrValue));
            diskManager.readBytes(offset, tmpBuf, 1024);
        }       
    }
}

void NTFSParser::getDeletedFileNames() {
    BYTE tmpBuf[1024] = {0};      // Bộ đệm tạm để lưu dữ liệu đọc từ đĩa
    BYTE tmpAttrValue[1024] = {0};// Bộ đệm tạm để lưu giá trị thuộc tính
    
    // Đọc dữ liệu từ cluster bắt đầu của MFT
    DWORD offset;
    int n = MFTRecordList.size();
    int numRecord;
    for (int i = 0; i < n; i++) {
        numRecord = MFTRecordList[i].length * sectorsPerCluster * bytesPerSector / 1024;
        offset = MFTRecordList[i].lcn * sectorsPerCluster * bytesPerSector;
        if (!diskManager.readBytes(offset, tmpBuf, 1024)) {
            cout << "Loi readBytes " << endl;
            return;
        }
        for (int j = 0 ; j < numRecord; j++) {
            if (memcmp(tmpBuf, "FILE", 4) == 0) {
                WORD flag = *(WORD*)&tmpBuf[0x16];
                if (flag == 0) {
                    if (getAttrValue(ATTR_FILE_NAME, tmpBuf, tmpAttrValue)) {
                        BYTE tmpFileLen = tmpAttrValue[0x58];
                        wchar_t	tmpFileName[MAX_PATH] = {0};
                        memset(tmpFileName,0,sizeof(tmpFileName));
                        memcpy(tmpFileName,tmpAttrValue+0x5A,tmpFileLen<<1);
                        // cout << ConvertWCharToString(tmpFileName) << endl;
                    }
                }
            }
            offset += 1024;
            memset(tmpBuf,0,sizeof(tmpBuf));
            memset(tmpAttrValue,0,sizeof(tmpAttrValue));
            diskManager.readBytes(offset, tmpBuf, 1024);
        }       
    }
}

bool NTFSParser::getDeletedFileRecord(string fileName, BYTE *fileBuf) {
    BYTE tmpBuf[1024] = {0};      // Bộ đệm tạm để lưu dữ liệu đọc từ đĩa
    BYTE tmpAttrValue[1024] = {0};// Bộ đệm tạm để lưu giá trị thuộc tính
    
    // Đọc dữ liệu từ cluster bắt đầu của MFT
    DWORD offset;
    int n = MFTRecordList.size();
    int numRecord;
    for (int i = 0; i < n; i++) {
        numRecord = MFTRecordList[i].length * sectorsPerCluster * bytesPerSector / 1024;
        offset = MFTRecordList[i].lcn * sectorsPerCluster * bytesPerSector;
        // cout << offset << endl;
        if (!diskManager.readBytes(offset, tmpBuf, 1024)) {
            cout << "Loi readBytes " << endl;
            return 0;
        }
        for (int j = 0 ; j < numRecord; j++) {
            if (memcmp(tmpBuf, "FILE", 4) == 0) {
                WORD flag = *(WORD*)&tmpBuf[0x16];
                if (flag == 0) {
                    if (getAttrValue(ATTR_FILE_NAME, tmpBuf, tmpAttrValue)) {
                        BYTE tmpFileLen = tmpAttrValue[0x58];
                        wchar_t	tmpFileName[MAX_PATH] = {0};
                        memset(tmpFileName,0,sizeof(tmpFileName));
                        memcpy(tmpFileName,tmpAttrValue+0x5A,tmpFileLen<<1);
                        if (fileName == ConvertWCharToString(tmpFileName)) {
                            // cout << fileName << endl;
                            memcpy(fileBuf, tmpBuf, 1024);
                            // WriteHexToFile("MFTRecord.txt", fileBuf, 1024);
                            return 1;
                        }
                    }
                }
            }
            offset += 1024;
            memset(tmpBuf,0,sizeof(tmpBuf));
            memset(tmpAttrValue,0,sizeof(tmpAttrValue));
            diskManager.readBytes(offset, tmpBuf, 1024);
        }       
    }
    return 0;
}

bool NTFSParser::getFileContent(BYTE fileBuf[], vector<BYTE> &fileContent) {
    BYTE tmpAttrValue[1024] = {0};
    if (getAttrValue(ATTR_DATA, fileBuf, tmpAttrValue)) {
        // WriteHexToFile("DATA_ATTR.txt", tmpAttrValue, 1024);
        // attrLen = *(UINT32*)&szAttrValue[4];
        if (tmpAttrValue[8] == 0)
        {
            DWORD fileLength = *(UINT32*)&tmpAttrValue[0x10];
            DWORD fileOffset = *(UINT32*)&tmpAttrValue[0x14];
            fileContent.assign(tmpAttrValue + fileOffset, tmpAttrValue + fileOffset + fileLength);
            return 1;
        }
        else 
        {
            ULONGLONG lengthFile = *(UINT64*)&tmpAttrValue[0x30];
            vector<Ntfs_Data_Run> datarun;
            WORD dataRunOffset = *(UINT16*)&tmpAttrValue[0x20];
            this->getDataRunList(tmpAttrValue, dataRunOffset, datarun);
            DWORD offset;
            int n = datarun.size();
            const int bytesPerCluster = sectorsPerCluster * bytesPerSector;
            BYTE tmpBuf[bytesPerCluster] = {0};
            for (int i = 0; i < n; i++) {
                offset = datarun[i].lcn * sectorsPerCluster * bytesPerSector;
                if (!diskManager.readBytes(offset, tmpBuf, bytesPerCluster)) {
                    cout << "Loi readBytes " << endl;
                    return 0;
                }
                int numCluster = (i == n - 1) ? datarun[i].length - 1 : datarun[i].length;
                for (int j = 0 ; j < numCluster; j++) {
                    fileContent.insert(fileContent.end(), tmpBuf, tmpBuf + bytesPerCluster);
                    offset += bytesPerCluster;
                    memset(tmpBuf,0,sizeof(tmpBuf));
                    diskManager.readBytes(offset, tmpBuf, bytesPerCluster);
                }
            }
            int lastClusterLength = lengthFile % bytesPerCluster;
            fileContent.insert(fileContent.end(), tmpBuf, tmpBuf + lastClusterLength);
            // cout << fileContent.size() << endl;
            return 1;
        }
    }
    return 0;
}

void NTFSParser::recoverDeletedFile(string fileName, string drive) {
    BYTE tmpBuf[1024] = {0};
    vector<BYTE> tmpContent;
    if(!getDeletedFileRecord(fileName, tmpBuf)) {
        cout << "Khong the tim duoc thong tin tep co ten " << fileName << "trong o dia " << drive << endl;
        cout << "Kiem tra lai ten file can khoi phuc.\n";
        return;
    }
    if (!getFileContent(tmpBuf, tmpContent)) {
        cout << "Khong lay duoc du lieu cua file can tim.\n";
        return;
    }
    // WriteHex_ToFile("content.txt", tmpContent); // debug
    if (checkAllIsZero(tmpContent)) {
        cout << "Du lieu cua file da bi he thong ghi de.\nKhong khoi phuc duoc file.\n";
        return;
    }
    string outputPath = drive + "\\" + "recovered_" + fileName;
    if (!WriteVector_ToFile(outputPath, tmpContent)) {
        cout << "Loi tao file moi. Thu lai\n";
        return;
    }
    cout << "File da khoi phuc thanh cong!\nTuy nhien van co the co loi trong du lieu cua file, vui long kiem tra lai.\n";
}
