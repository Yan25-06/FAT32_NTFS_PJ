#include "utils.h"

void printMenu() {
    cout << "\n===== MENU =====\n";
    cout << "1. Xem loai he thong tap tin\n";
    cout << "2. Hien thi thong tin Boot Sector\n";
    cout << "3. Liet ke danh sach file\n";
    cout << "4. Liet ke danh sach file da xoa\n";
    cout << "5. Khoi phuc file\n";
    cout << "6. Thoat\n";
    cout << "================\n";
    cout << "Chon: ";
}

string extractFileName(const DirectoryEntry *entries, char firstChar, size_t index)
{
    if (index > 0 && entries[index - 1].attr == 0x0F)
        return extractLFN(entries, index);
    return extractSFN(entries, firstChar, index);
}

string utf16le_to_utf8(const wstring &wstr) {
    wstring_convert<codecvt_utf8_utf16<wchar_t>> conv;
    return conv.to_bytes(wstr);
}

string extractSFN(const DirectoryEntry *entries, char firstChar, size_t index)
{
    char recoveredName[SFN_SIZE] = {0};
    recoveredName[0] = firstChar;
    memcpy(&recoveredName[1], &entries[index].name[1], 10); // Bỏ ký tự đầu
    return string(recoveredName);
}

string extractLFN(const DirectoryEntry *entries, size_t index) {
    wstring longFileName;

    while (index > 0) {
        index--; // Duyệt lùi vì LFN đi trước entry chính
        if (entries[index].attr != 0x0F) {
            break; 
        }
      
        LFNEntry *lfn = (LFNEntry *)&entries[index];
        for (int i = 0; i < 5; i++) { // name1 (10 byte)
            wchar_t ch = (lfn->name1[i * 2] | (lfn->name1[i * 2 + 1] << 8));
            if (ch == 0xFFFF || ch == 0x0000) break;
            longFileName += ch;
        }
        for (int i = 0; i < 6; i++) { // name2 (12 byte)
            wchar_t ch = (lfn->name2[i * 2] | (lfn->name2[i * 2 + 1] << 8));
            if (ch == 0xFFFF || ch == 0x0000) break;
            longFileName += ch;
        }
        for (int i = 0; i < 2; i++) { // name3 (4 byte)
            wchar_t ch = (lfn->name3[i * 2] | (lfn->name3[i * 2 + 1] << 8));
            if (ch == 0xFFFF || ch == 0x0000) break;
            longFileName += ch;
        }
    }
    return utf16le_to_utf8(longFileName);
}

string convertToSFN(const string &filename) {
    if (filename.size() > SFN_SIZE)
        return filename;
    string namePart, extPart;
    size_t dotPos = filename.find_last_of('.');

    if (dotPos != string::npos) {
        namePart = filename.substr(0, dotPos); // Lấy phần tên trước dấu chấm
        extPart = filename.substr(dotPos + 1); // Lấy phần mở rộng
    } else {
        namePart = filename; // Không có phần mở rộng
    }

    // Chuyển thành chữ in hoa
    transform(namePart.begin(), namePart.end(), namePart.begin(), ::toupper);
    transform(extPart.begin(), extPart.end(), extPart.begin(), ::toupper);

    // Cắt tên chính thành tối đa 8 ký tự
    if (namePart.length() > 8) {
        namePart = namePart.substr(0, 8);
    }

    // Cắt phần mở rộng thành tối đa 3 ký tự
    if (extPart.length() > 3) {
        extPart = extPart.substr(0, 3);
    }

    // Canh lề để đủ 8 ký tự cho tên chính và 3 ký tự cho phần mở rộng
    while (namePart.length() < 8) {
        namePart += ' ';
    }
    while (extPart.length() < 3) {
        extPart += ' ';
    }

    return namePart + extPart;
}
void trim(string &s) {
    // Xóa khoảng trắng đầu chuỗi
    s.erase(s.begin(), find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !isspace(ch);
    }));

    // Xóa khoảng trắng cuối chuỗi
    s.erase(find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !isspace(ch);
    }).base(), s.end());
}

string ConvertWCharToString(const wchar_t* wStr) {
    if (!wStr) return "";

    int len = WideCharToMultiByte(CP_ACP, 0, wStr, -1, NULL, 0, NULL, NULL);
    if (len <= 0) return "";

    string str(len - 1, 0); // Tạo std::string có đúng số ký tự cần thiết
    WideCharToMultiByte(CP_ACP, 0, wStr, -1, &str[0], len, NULL, NULL);
    
    return str;
}

bool WriteVector_ToFile(string filename, const vector<BYTE>& data) {
    ofstream outFile(filename, std::ios::binary);
    if (!outFile) {
        return false; // Mở tệp thất bại
    }
    outFile.write(reinterpret_cast<const char*>(data.data()), data.size());
    return outFile.good();
}

void WriteHex_ToFile(const char *filename, vector<BYTE> buf) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        printf("Khong mo duoc file!\n");
        return;
    }
    int size = buf.size();
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

bool checkAllIsZero(vector<BYTE> content) {
    int n = content.size();
    for (int i = 0; i < n; i++) {
        if (content[i] != 0)
            return 0;
    }
    return 1;
}