#include "utils.h"

string extractFileName(const DirectoryEntry *entries, string filename, size_t index)
{
    if (filename.size() > SFN_SIZE)
        return extractLFN(entries, index);
    return extractSFN(entries, filename, index);
}

string utf16le_to_utf8(const wstring &wstr) {
    wstring_convert<codecvt_utf8_utf16<wchar_t>> conv;
    return conv.to_bytes(wstr);
}

string extractSFN(const DirectoryEntry *entries, string filename, size_t index)
{
    char recoveredName[12] = {0};
    recoveredName[0] = filename[0];
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