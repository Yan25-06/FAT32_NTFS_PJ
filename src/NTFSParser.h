#include "diskManager.h"
#include <cstring>
#include <vector>
#include <stdlib.h>
#include <cmath>
using namespace std;

#pragma pack(push,1)
enum NTFS_ATTRDEF
{
	ATTR_STANDARD = 0x10, // Thuộc tính thông tin tiêu chuẩn
	ATTR_ATTRIBUTE_LIST = 0x20, // Danh sách thuộc tính
	ATTR_FILE_NAME = 0x30, // Thuộc tính tên tập tin
	ATTR_VOLUME_VERSION = 0x40, // Thuộc tính phiên bản của volume
	ATTR_SECURITY_DESCRIPTOR = 0x50, // Mô tả bảo mật
	ATTR_VOLUME_NAME = 0x60, // Tên của volume (ổ đĩa)
	ATTR_VOLUME_INFOMATION = 0x70, // Thông tin về volume
	ATTR_DATA = 0x80, // Thuộc tính dữ liệu
	ATTR_INDEX_ROOT = 0x90, // Gốc chỉ mục của thư mục
	ATTR_INDEX_ALLOCATION = 0xA0, // Khu vực lưu chỉ mục mở rộng
	ATTR_BITMAP = 0xB0, // Thuộc tính bitmap
	ATTR_SYMLINK = 0xC0, // Thuộc tính liên kết tượng trưng (symbolic link)
	ATTR_HPFS_EXTENDED_INFO = 0xD0, // Thông tin mở rộng của hệ thống tệp HPFS
	ATTR_HPFS_EXTENDED = 0xE0, // Thuộc tính mở rộng của HPFS
	ATTR_PROPERTY = 0xF0, // Thuộc tính thiết lập quyền sở hữu
	ATTR_LOG_STREAM = 0x100 // Thuộc tính luồng nhật ký (log stream)
};

typedef struct _NTFS_Data_Run
{
	UINT64 lcn;      // Số cụm logic (Logical Cluster Number - LCN), chỉ vị trí dữ liệu thực tế trên ổ đĩa.
    UINT64 vcn;      // Số cụm ảo (Virtual Cluster Number - VCN), thể hiện thứ tự logic của dữ liệu trong file.
    UINT64 length;   // Độ dài (số cụm) mà đoạn dữ liệu này chiếm.
    _NTFS_Data_Run *next;  // Con trỏ đến data run tiếp theo trong danh sách liên kết.
	_NTFS_Data_Run()
	{
		lcn = 0;
		vcn = 0;
		length = 0;
		next = NULL;
	}
} Ntfs_Data_Run;

#pragma pack(pop)

class NTFSParser {
    public:
        NTFSParser(DiskManager &d);
        bool getBasicInfo();
        void printBootSectorInfo();
        UINT32 GetAttrValue(NTFS_ATTRDEF prmAttrTitle, BYTE prmBuf[], BYTE *prmAttrValue);
        void GetMFTRunList();
        void GetDataRunList(UCHAR *prmBuf, UINT16 prmRunListOffset, Ntfs_Data_Run **prmList);
    private:
        UINT64 MFTStartCluster;
        UINT64 bytesPerSector;       
        UINT64 sectorsPerCluster;
        UINT64 clustersPerMFTRecord;
        DiskManager &diskManager;
        Ntfs_Data_Run *MFTRecordList;
};
