#include "NTFSParser.h"

NTFSParser::NTFSParser(DiskManager &d) : diskManager(d) {
    getBasicInfo();
}

string ConvertWCharToString(const wchar_t* wStr) {
    if (!wStr) return "";

    int len = WideCharToMultiByte(CP_ACP, 0, wStr, -1, NULL, 0, NULL, NULL);
    if (len <= 0) return "";

    string str(len - 1, 0); // Tạo std::string có đúng số ký tự cần thiết
    WideCharToMultiByte(CP_ACP, 0, wStr, -1, &str[0], len, NULL, NULL);
    
    return str;
}

string NTFSParser::getFileName(DWORD prmOffset)
{
	BYTE		tmpBuf[1024] = {0};
	wchar_t		tmpFileName[MAX_PATH] = {0};
    string s = "";
	//char		tmpAnsiName[MAX_PATH*2] = {0};
    // DWORD offset = prmOffset;
    // diskManager.readBytes(prmOffset, tmpBuf, 1024);
    // DWORD offset = this->MFTStartCluster * this->sectorsPerCluster * this->bytesPerSector;
    if(!diskManager.readBytes(prmOffset, tmpBuf, 1024))
        return s;
	UINT16	tmpUsnOffset = *(UINT16*)&tmpBuf[4];
	memcpy(tmpBuf+0x1FE,tmpBuf+tmpUsnOffset+2,2);
	memcpy(tmpBuf+0x3FE,tmpBuf+tmpUsnOffset+4,2);
	UINT32	attrOffset = *(UINT16*)&tmpBuf[20];
	UINT32	attrLen = 0;
	while (attrOffset < 1024)
	{
		if (tmpBuf[attrOffset] == 0xff)
		{
			return 0;
		}
		attrLen = *(UINT16*)&tmpBuf[attrOffset + 4];
		if (tmpBuf[attrOffset] == ATTR_FILE_NAME && tmpBuf[attrOffset+0x59]!=2)
		{
			UINT8	tmpFileLen = tmpBuf[attrOffset + 0x58];
			memset(tmpFileName,0,sizeof(tmpFileName));
			memcpy(tmpFileName,tmpBuf+attrOffset+0x5A,tmpFileLen<<1);
			//::WideCharToMultiByte(CP_ACP,0,tmpFileName,-1,tmpAnsiName,MAX_PATH*2,0,0);
			return ConvertWCharToString(tmpFileName);
			
		}
		attrOffset += attrLen;
	}
	return s;
}

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
    this->totalSectors = *(uint64_t*)&bootSectorData[40];
    this->MFTStartCluster = *(uint64_t*)&bootSectorData[48];  
    // int c = (int8_t)bootSectorData[64];  
    // this->clustersPerMFTRecord = (c < 0) ? 1 << (-c) : c;  // Chuyển về power of 2
    this->GetMFTRunList();
    return true;
}

void NTFSParser::printBootSectorInfo() {
    cout << "Bytes per sector: " << bytesPerSector << endl;
    cout << "Sectors per cluster: " << sectorsPerCluster << endl;
    cout << "Total Sector: " << totalSectors << endl;
    cout << "MFT Start Cluster: " << MFTStartCluster << endl;
    DWORD offset = this->MFTStartCluster * this->sectorsPerCluster * this->bytesPerSector;
    // cout << "File name: " << getFileName(offset) << endl;
    // cout << "Cluster per Record: " << clustersPerMFTRecord << endl;
    Ntfs_Data_Run *temp = this->MFTRecordList;
    while (temp != NULL) {
        cout << temp->lcn << " " << temp->vcn << " " << temp->length << endl;
        temp = temp->next;
    } 
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
        // cout << *(UINT16*)&prmBuf[attrOffset] << " " << prmAttrTitle << endl;
        // cout << attrOffset << endl;
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

UINT32 NTFSParser::GetAttrFromAttributeList(NTFS_ATTRDEF prmAttrType,UINT32 prmOffset,UCHAR *prmAttrList,UCHAR *prmAttrValue)
{
	UINT32	*tmpAttrLen = (UINT32*)(prmAttrList+4);
	while(prmOffset+4<*tmpAttrLen)
	{
		UINT16	*tmpLen = (UINT16*)(prmAttrList+prmOffset+4); 
		if(prmAttrList[prmOffset] == prmAttrType)
		{
			memcpy(prmAttrValue,prmAttrList+prmOffset,*tmpLen);
			return prmOffset;
		}
		prmOffset += *tmpLen;
	}
	return 0;
}

UINT32	NTFSParser::GetExtendMFTAttrValue(UINT64 prmSeqNum,NTFS_ATTRDEF prmAttrType,UCHAR *prmAttrValue)
{
	UCHAR	tmpBuf[1024] = {0};
	prmSeqNum = prmSeqNum & MFTREFMASK;
	UINT64	tmpOffset = this->GetOffsetByMFTRef(prmSeqNum);
    diskManager.readBytes(tmpOffset, tmpBuf, 1024);
	UINT16	tmpUsnOffset = *(UINT16*)&tmpBuf[4];
	memcpy(tmpBuf+0x1FE,tmpBuf+tmpUsnOffset+2,2);
	memcpy(tmpBuf+0x3FE,tmpBuf+tmpUsnOffset+4,2);
	if(this->GetAttrValue(prmAttrType,tmpBuf,prmAttrValue))
	{
		return 1;
	}
	return 0;
}

UINT64	NTFSParser::GetOffsetByMFTRef(UINT64 prmSeqNo)
{
	prmSeqNo = prmSeqNo & MFTREFMASK;
	UINT64	tmpOffset = prmSeqNo<<1;
	tmpOffset = tmpOffset * bytesPerSector;
	UINT64	tmpVCN = tmpOffset / sectorsPerCluster / bytesPerSector;
	Ntfs_Data_Run	*p = MFTRecordList;
	while(p!=NULL)
	{
		if(tmpVCN > p->vcn && tmpVCN < p->vcn+p->length)
		{
			break;
		}
		p = p->next;
	}
	if(p==NULL)
	{
		return 0;
	}
	UINT64 tmpOfs = tmpOffset - p->vcn * sectorsPerCluster * bytesPerSector;
	return tmpOfs + p->lcn * sectorsPerCluster * bytesPerSector;
}


void NTFSParser::GetDataRunList(UCHAR *prmBuf, UINT16 prmRunListOffset, Ntfs_Data_Run **prmList)
{
    UINT64 index_alloc_size = 0; // Lưu kích thước của data run (số lượng cụm)
    UINT64 lcn = 0; // Lưu Logical Cluster Number (LCN)
    int j;
    UINT16 bufOffset = prmRunListOffset; 
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
        // cout << datarun->length << " " << datarun->lcn << endl;

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

void NTFSParser::GetFileExtent(UCHAR *prmBuf,UINT64 prmMftSector,File_Content_Extent_s **prmFileExtent)
{
	UCHAR	szAttrValue[1024] = { 0 };
	UCHAR	szAttrList[1024] = {0};
	UCHAR	szExtentAttrValue[1024] = {0};
	UINT32 readBytes = 0;
	Ntfs_Data_Run		*runList = NULL;
	Ntfs_Data_Run		*q = NULL;
	UINT32		attrLen = 0;
	UINT32		attrOff = 0;
	UINT32		result = 0;

	if(this->GetAttrValue(ATTR_ATTRIBUTE_LIST,prmBuf,szAttrList)) {
		UINT32	tmpOffset = 0x18;
		while(tmpOffset=this->GetAttrFromAttributeList(ATTR_DATA,tmpOffset,szAttrList,szAttrValue)) {
			UINT16	tmpLen = *(UINT16*)(szAttrValue+4);
			UINT64		seqNum =   *(UINT64*)(szAttrValue + 0x10);
			if(this->GetExtendMFTAttrValue(seqNum,ATTR_DATA,szExtentAttrValue)) {
				UINT16	runlistOffset = *(UINT16*)&szExtentAttrValue[0x20];
				Ntfs_Data_Run *p = NULL;
				this->GetDataRunList(szExtentAttrValue,runlistOffset,&p);
				if (runList == NULL) {
					runList = p;
					q = p;
				}
				else {
					while (q->next) {
						q = q->next;
					}
					q->next = p;
				}
			}
			tmpOffset += tmpLen;
		}
	}
	if(runList!=NULL) {
		goto END;
	}
	// 0x80
	if (this->GetAttrValue(ATTR_DATA, prmBuf, szAttrValue))
	{
		attrLen = *(UINT32*)&szAttrValue[4];
		if (szAttrValue[8] == 0) { // resident
			UINT32 fileLength = *(UINT32*)&szAttrValue[0x10];
			*prmFileExtent = new File_Content_Extent_s;
			(*prmFileExtent)->startSector = prmMftSector;
			(*prmFileExtent)->totalSector = 2;
			(*prmFileExtent)->isPersist = 1;
		}
		else { // non-resident
			UINT16	runlistOffset = *(UINT16*)&szAttrValue[0x20];
			this->GetDataRunList(szAttrValue, runlistOffset, &runList);
			goto END;
		}
	}
END:
	Ntfs_Data_Run *p = runList;
	File_Content_Extent_s *tmpExtent = *prmFileExtent;
	while (p != NULL && p->lcn != 0 && p->length != 0 && p->length < totalSectors / sectorsPerCluster) {
		File_Content_Extent_s *t = new File_Content_Extent_s;
		t->totalSector = p->length* sectorsPerCluster;
		t->startSector = p->lcn * sectorsPerCluster;
		if (tmpExtent == NULL) {
			tmpExtent = t;
			*prmFileExtent = t;
		}
		else {
			tmpExtent->next = t;
			tmpExtent = t;
		}
		p = p->next;
	}
	this->freeRunList(runList);
}

void NTFSParser::GetMFTRunList()
{
    BYTE tmpBuf[1024] = {0};      // Bộ đệm tạm để lưu dữ liệu đọc từ đĩa
    BYTE tmpAttrValue[1024] = {0};// Bộ đệm tạm để lưu giá trị thuộc tính

    // Đọc dữ liệu từ cluster bắt đầu của MFT
    DWORD offset = this->MFTStartCluster * this->sectorsPerCluster * this->bytesPerSector;
    
    diskManager.readBytes(offset, tmpBuf, 1024);
    WriteHexToFile("MFT.txt", tmpBuf, 1024);

    // Lấy giá trị độ lệch của số thứ tự cập nhật (Update Sequence Number - USN)
    UINT16 usnOffset = *(UINT16*)&tmpBuf[4]; 

    // Khôi phục hai byte cuối của sector đầu tiên trong bản ghi MFT
    memcpy(tmpBuf + 0x1FE, tmpBuf + usnOffset + 2, 2);

    // Khôi phục hai byte cuối của sector thứ hai trong bản ghi MFT
    memcpy(tmpBuf + 0x3FE, tmpBuf + usnOffset + 4, 2);

    // Lấy thuộc tính dữ liệu (Data Attribute - 0x80) từ bản ghi MFT
    if (this->GetAttrValue(ATTR_DATA, tmpBuf, tmpAttrValue))
    {
        WriteHexToFile("DATA.txt", tmpAttrValue, 1024);
        UINT16 dataRunOffset = *(UINT16*)&tmpAttrValue[0x20];
        // cout << dataRunOffset << endl;
        // Trích xuất danh sách Data Run từ thuộc tính dữ liệu
        this->GetDataRunList(tmpAttrValue, dataRunOffset, &this->MFTRecordList);
    }
    wchar_t		tmpFileName[MAX_PATH] = {0};
    UINT32	attrOffset = *(UINT16*)&tmpBuf[20];
	UINT32	attrLen = 0;
	while (attrOffset < 1024)
	{
		if (tmpBuf[attrOffset] == 0xff)
            break;
		attrLen = *(UINT16*)&tmpBuf[attrOffset + 4];
		if (tmpBuf[attrOffset] == ATTR_FILE_NAME && tmpBuf[attrOffset+0x59]!=2)
		{
			UINT8	tmpFileLen = tmpBuf[attrOffset + 0x58];
			memset(tmpFileName,0,sizeof(tmpFileName));
			memcpy(tmpFileName,tmpBuf+attrOffset+0x5A,tmpFileLen<<1);
			//::WideCharToMultiByte(CP_ACP,0,tmpFileName,-1,tmpAnsiName,MAX_PATH*2,0,0);
			cout << "File name: " << ConvertWCharToString(tmpFileName) << endl;
			
		}
		attrOffset += attrLen;
	}
}

void NTFSParser::freeRunList(Ntfs_Data_Run *list)
{
	Ntfs_Data_Run	*q = NULL;
	while (list)
	{
		q = list->next;
		delete list;
		list = q;
	}
}

NTFSParser::~NTFSParser() {
    freeRunList(MFTRecordList);
}

int main() {
    // 
    DiskManager disk("F:");
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