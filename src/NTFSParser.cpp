#include "NTFSParser.h"

NTFSParser::NTFSParser(DiskManager &d) : diskManager(d) {
    getBasicInfo();
}
// ham ghi ra file de test
void WriteHexToFile(const char *filename, UCHAR *buf, int size) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        printf("ko mo duoc file!\n");
        return;
    }

    for (int i = 0; i < size; i += 16) {
        fprintf(file, "%04X: ", i);  // Ghi offset
        for (int j = 0; j < 16 && (i + j) < size; j++) {
            fprintf(file, "%02X ", buf[i + j]);
        }
        for (int j = size - i; j < 16; j++) {
            fprintf(file, "   ");
        }

        fprintf(file, " | ");
        for (int j = 0; j < 16 && (i + j) < size; j++) {
            UCHAR c = buf[i + j];
            fprintf(file, "%c", (c >= 32 && c <= 126) ? c : '.');
        }

        fprintf(file, " |\n");
    }

    fclose(file);
}


bool NTFSParser::getBasicInfo() {
    BYTE bootSectorData[512] = {0};
    DWORD bytesRead;

    if (!diskManager.readSector(0, bootSectorData, 512))
    {
        cerr << "Read boot sector failed!";
        return false;
    }
    // WriteHexToFile("test.txt", bootSectorData, 512);
    this->bytesPerSector = *(uint16_t*)&bootSectorData[11];  
    this->sectorsPerCluster = bootSectorData[13];  
    this->MFTStartCluster = *(uint64_t*)&bootSectorData[48];  
    int c = (int8_t)bootSectorData[64];  
    this->clustersPerMFTRecord = (c < 0) ? 1 << (-c) : c;  // Chuyển về power of 2
    this->GetMFTRunList();
    return true;
}

void NTFSParser::printBootSectorInfo() {
    cout << "Bytes per sector: " << bytesPerSector << endl;
    cout << "Sectors per cluster: " << sectorsPerCluster << endl;
    cout << "MFT Start Cluster: " << MFTStartCluster << endl;
    cout << "Cluster per Record: " << clustersPerMFTRecord << endl;
    // Ntfs_Data_Run *temp = this->MFTRecordList;
    // while (temp != NULL) {
        // cout << temp->lcn << " " << temp->vcn << " " << temp->length << endl;
        // temp = temp->next;
        // cout << temp->lcn << " " << temp->vcn << " " << temp->length << endl;
    // } 
}

UINT32 NTFSParser::GetAttrValue(NTFS_ATTRDEF prmAttrTitle, BYTE prmBuf[], BYTE *prmAttrValue)
{
    // Kiểm tra chữ ký của tệp (MFT record signature)
    UINT32 fileSign = *(UINT32*)prmBuf;
    if (fileSign != 0x454C4946)  // "FILE" trong mã ASCII
    {
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
            return 0;
        }
        // Lấy độ dài của thuộc tính hiện tại (2 byte tại offset +4)
        attrLen = *(UINT16*)&prmBuf[attrOffset + 4];
        // Nếu thuộc tính hiện tại là thuộc tính cần tìm
        if (prmBuf[attrOffset] == prmAttrTitle)
        {
            // Sao chép toàn bộ dữ liệu của thuộc tính vào prmAttrValue
            memcpy(prmAttrValue, prmBuf + attrOffset, attrLen);
            return 1;  // Thành công
        }
        // Chuyển sang thuộc tính tiếp theo
        attrOffset += attrLen;
    }
    return 0;  // Không tìm thấy thuộc tính
}

void NTFSParser::GetDataRunList(UCHAR *prmBuf, UINT16 prmRunListOffset, Ntfs_Data_Run **prmList)
{
    UINT64 index_alloc_size = 0; // Lưu kích thước của data run (số lượng cụm)
    UINT64 lcn = 0; // Lưu Logical Cluster Number (LCN)
    int j;
    UINT16 bufOffset = prmRunListOffset; // Độ lệch bắt đầu của Data Run
    UINT32 temp = 0;
    *prmList = NULL;
    Ntfs_Data_Run *p = NULL;

    // Mỗi mục trong danh sách Run có byte đầu tiên:
    // - 4 bit thấp: số byte dùng để lưu kích thước cụm
    // - 4 bit cao: số byte dùng để lưu LCN (vị trí bắt đầu)
    
    while (true)
    {
        index_alloc_size = 0;
        // Tính toán số lượng cụm được cấp phát (Length)
        for (j = 0; j < prmBuf[bufOffset] % 16; j++)
        {
            index_alloc_size += prmBuf[bufOffset + 1 + j] * (UINT64)pow((long double)256, j);
        }

        // Tính toán giá trị cụm bắt đầu (LCN - Logical Cluster Number)
        if (prmBuf[bufOffset + prmBuf[bufOffset] % 16 + prmBuf[bufOffset] / 16] > 127)  
        {   
            // Nếu là số âm (bit cao nhất là 1), thì tính bù hai
            for (j = 0; j < prmBuf[bufOffset] / 16; j++)
            {
                temp = ~prmBuf[bufOffset + prmBuf[bufOffset] % 16 + 1 + j]; // Lấy bù
                temp = temp & 255;
                lcn = lcn - temp * (UINT64)pow((long double)256, j);
            }
            lcn = lcn - 1; // Trừ đi 1 để hoàn tất tính bù
        }
        else
        {
            // Nếu là số dương, tính toán trực tiếp
            for (j = 0; j < prmBuf[bufOffset] / 16; j++)
            {
                lcn += prmBuf[bufOffset + prmBuf[bufOffset] % 16 + 1 + j] * (UINT64)pow((long double)256, j);
            }
        }

        // Tạo một nút mới trong danh sách Data Run
        Ntfs_Data_Run *datarun = new Ntfs_Data_Run;
        if (*prmList == NULL)
        {
            *prmList = datarun;
        }
        datarun->lcn = lcn;
        datarun->vcn = 0;
        datarun->length = index_alloc_size; // Số cụm mà Run chiếm dụng

        if (p != NULL)
        {
            datarun->vcn += p->length; // Tính toán VCN tiếp theo
            p->next = datarun;
        }
        p = datarun;

        // Tính toán độ lệch của Data Run tiếp theo
        bufOffset = bufOffset + prmBuf[bufOffset] / 16 + prmBuf[bufOffset] % 16 + 1;
        if (0 == prmBuf[bufOffset])
        {
            // Kết thúc danh sách Run List
            break;
        }
    }
}

void NTFSParser::GetMFTRunList()
{
    BYTE tmpBuf[1024] = {0};      // Bộ đệm tạm để lưu dữ liệu đọc từ đĩa
    BYTE tmpAttrValue[1024] = {0};// Bộ đệm tạm để lưu giá trị thuộc tính

    // Đọc dữ liệu từ cluster bắt đầu của MFT
    cout << this->MFTStartCluster * this->sectorsPerCluster << endl;
    diskManager.readSector(this->MFTStartCluster * this->sectorsPerCluster, tmpBuf, 1024);
    WriteHexToFile("output_hex.txt", tmpBuf, 1024);

    // Lấy giá trị độ lệch của số thứ tự cập nhật (Update Sequence Number - USN)
    UINT16 usnOffset = *(UINT16*)&tmpBuf[4]; 

    // Khôi phục hai byte cuối của sector đầu tiên trong bản ghi MFT
    memcpy(tmpBuf + 0x1FE, tmpBuf + usnOffset + 2, 2);

    // Khôi phục hai byte cuối của sector thứ hai trong bản ghi MFT
    memcpy(tmpBuf + 0x3FE, tmpBuf + usnOffset + 4, 2);

    // Lấy thuộc tính dữ liệu (Data Attribute - 0x80) từ bản ghi MFT
    if (this->GetAttrValue(ATTR_DATA, tmpBuf, tmpAttrValue))
    {
        UINT16 dataRunOffset = *(UINT16*)&tmpAttrValue[0x20];
        // Trích xuất danh sách Data Run từ thuộc tính dữ liệu
        this->GetDataRunList(tmpAttrValue, dataRunOffset, &this->MFTRecordList);
    }
}

int main() {
    // 
    DiskManager disk("\\\\.\\F:");
    if (!disk.openDrive()) {
        cerr << "Khong the mo o dia!\n";
        return -1;
    }
    NTFSParser parser(disk);
    parser.printBootSectorInfo();
    // if (parser.readBootSector()) {
    //     cout << "Boot sector read successfully!" << endl;
    // }
    return 0;
}