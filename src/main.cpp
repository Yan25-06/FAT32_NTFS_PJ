#include <iostream>
#include <string>

#include "fat32Recovery.h"

using namespace std;

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

int main() {
    string drive;
    cout << "Nhap ky tu o dia (VD: E:): ";
    cin >> drive;

    DiskManager disk(drive);
    if (!disk.openDrive()) {
        cerr << "Khong the mo o dia!\n";
        return -1;
    }

    Fat32Parser parser(disk);
    parser.printBootSectorInfo();
    Fat32Recovery recovery(parser, disk);

    int choice;
    while (true) {
        printMenu();
        cin >> choice;
        cin.ignore(); // Xoa bo dem phim Enter

        switch (choice) {
            case 1: { // Xem loai he thong tap tin
                string fsType = disk.getFSType();
                cout << "File System Type: " << fsType << endl;
                break;
            }
            case 2: { // Hien thi Boot Sector
                parser.printBootSectorInfo();
                break;
            }
            case 3: {
                recovery.listFiles();
                break;
            }
            case 4: { // Liet ke file da xoa
                recovery.listDeletedFiles();
                break;
            }
            case 5: { // Khoi phuc file
                string filename;
                cout << "Nhap ten file can khoi phuc: ";
                getline(cin, filename);

                if (recovery.recoverFile(filename, drive)) {
                    cout << "File da duoc khoi phuc thanh cong!\n";
                } else {
                    cerr << "Khoi phuc that bai!\n";
                }
                break;
            }
            case 6: { // Thoat chuong trinh
                cout << "Thoat chuong trinh.\n";
                return 0;
            }
            default:
                cout << "Lua chon khong hop le!\n";
        }
    }

    return 0;
}
