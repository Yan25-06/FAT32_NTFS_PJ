#include "NTFSParser.h"
#include "fat32Recovery.h"

using namespace std;

void runFat32Menu(Fat32Parser& parser, Fat32Recovery& recovery, DiskManager& disk) {
    int choice;
    while (true) {
        printMenu();
        cin >> choice;

        switch (choice) {
            case 1: {
                cout << "File System Type: FAT32\n";
                break;
            }
            case 2: {
                parser.printBootSectorInfo();
                break;
            }
            case 3: {
                recovery.listFiles();
                break;
            }
            case 4: {
                recovery.listDeletedFiles();
                break;
            }
            case 5: {
                string filename;
                cout << "Nhap ten file can khoi phuc: ";
                cin.ignore();
                getline(cin, filename);

                if (recovery.recoverFile(filename)) {
                    cout << "Khoi phuc thanh cong!\n";
                } else {
                    cerr << "Khoi phuc that bai!\n";
                }
                break;
            }
            case 6: {
                cout << "Thoat chuong trinh.\n";
                return;
            }
            default:
                cout << "Lua chon khong hop le!\n";
        }
    }
}

void runNtfsMenu(NTFSParser& parser, DiskManager& disk) {
    int choice;
    while (true) {
        printMenu();
        cin >> choice;

        switch (choice) {
            case 1: {
                cout << "File System Type: NTFS\n";
                break;
            }
            case 2: {
                parser.printBasicInfo();
                break;
            }
            case 3: {
                parser.readMFT();
                break;
            }
            case 4: {
                parser.getDeletedFileNames();
                break;
            }
            case 5: {
                string filename;
                cout << "Nhap ten file can khoi phuc: ";
                cin.ignore();
                getline(cin, filename);

                if (parser.recoverDeletedFile(filename)) {
                    cout << "Khoi phuc thanh cong!\n";
                } else {
                    cerr << "Khoi phuc that bai!\n";
                }
                break;
            }
            case 6: {
                cout << "Thoat chuong trinh.\n";
                return;
            }
            default:
                cout << "Lua chon khong hop le!\n";
        }
    }
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

    string fsType = disk.getFSType();

    if (fsType == "FAT32") {
        Fat32Parser parser(disk);
        Fat32Recovery recovery(parser, disk);
        runFat32Menu(parser, recovery, disk);
    } else if (fsType == "NTFS") {
        NTFSParser parser(disk);
        runNtfsMenu(parser, disk);
    } else {
        cerr << "He thong tap tin khong duoc ho tro: " << fsType << endl;
    }

    return 0;
}

// int main() {
//     DiskManager disk("F:");
//     if (!disk.openDrive()) {
//         cerr << "Khong the mo o dia!\n";
//         return -1;
//     }
//     NTFSParser parser(disk);
//     // cout << disk.getFSType() << endl;
//     // parser.printBasicInfo();
//     // parser.getFileList();
//     // parser.getDeletedFileNames();
//     BYTE tmpBuf[1024] = {0};
//     // vector<BYTE> tmpContent;
//     // cout << parser.getDeletedFileRecord("Exponential Dist.pdf", tmpBuf) << endl;
//     // cout << parser.getFileContent(tmpBuf, tmpContent) << endl;
//     parser.recoverDeletedFile("23127466_PhanNhuQuynh_PTHDC.pdf");
//     return 0;
// }
// int main() {
//     string drive;
//     cout << "Nhap ky tu o dia (VD: E:): ";
//     cin >> drive;

//     DiskManager disk(drive);
//     if (!disk.openDrive()) {
//         cerr << "Khong the mo o dia!\n";
//         return -1;
//     }
//     Fat32Parser parser(disk);
//     Fat32Recovery recovery(parser, disk);

//     int choice;
//     while (true) {
//         printMenu();
//         cin >> choice;
        
//         switch (choice) {
//             case 1: { // Xem loai he thong tap tin
//                 string fsType = disk.getFSType();
//                 cout << "File System Type: " << fsType << endl;
//                 break;
//             }
//             case 2: { // Hien thi Boot Sector
//                 parser.printBootSectorInfo();
//                 break;
//             }
//             case 3: {
//                 recovery.listFiles();
//                 break;
//             }
//             case 4: { // Liet ke file da xoa
//                 recovery.listDeletedFiles();
//                 break;
//             }
//             case 5: { // Khoi phuc file
//                 string filename;
//                 cout << "Nhap ten file can khoi phuc: ";
//                 cin.ignore(); 
//                 getline(cin, filename);

//                 if (recovery.recoverFile(filename)) {
//                     cout << "File da duoc khoi phuc thanh cong!\n";
//                 } else {
//                     cerr << "Khoi phuc that bai!\n";
//                 }
//                 break;
//             }
//             case 6: { // Thoat chuong trinh
//                 cout << "Thoat chuong trinh.\n";
//                 return 0;
//             }
//             default:
//                 cout << "Lua chon khong hop le!\n";
//         }
//     }

//     return 0;
// }
