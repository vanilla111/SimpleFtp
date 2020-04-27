//
// Created by wang on 2020/4/21.
//
#include <sstream>
#include <string>
#include <ctime>

#include "headers/FileOperations.h"
#include "headers/FtpException.h"
#include "headers/Properties.h"

using namespace std;

FileList FileOperations::getAllFilesWhichInOneDirectory(const string& dirname) {
    DIR *dp;
    struct dirent *dirp;
    struct stat statBuf{};
    FileList  files;
    if ((dp = opendir(dirname.c_str())) == nullptr) {
        FtpException e("错误的文件夹路径: " + dirname);
        throw e;
    }
    while((dirp = readdir(dp)) != nullptr) {
        char subdir[256];
        auto *fa = new FileAttributes;
        sprintf(subdir, "%s/%s", dirname.c_str(), dirp->d_name);
        lstat(subdir, &statBuf);
        if (strcmp(".", dirp->d_name) == 0 ||
            strcmp("..", dirp->d_name) == 0) {
            continue;
        }
        fa->name = dirp->d_name;
        fa->time_write = timestampToDate((long)statBuf.st_mtimespec.tv_sec);
        fa->size = statBuf.st_size;
        if (S_ISDIR(statBuf.st_mode))
            fa->is_dir = true;
        else
            fa->is_dir = false;
        files.push_back(fa);
    }
    closedir(dp);
    return files;
}

//struct stat {
//    dev_t      st_dev;    /* device inode resides on */
//    ino_t      st_ino;    /* inode's number */
//    mode_t     st_mode;   /* inode's mode */
//    nlink_t    st_nlink;  /* number of hard links to the file */
//    uid_t      st_uid;    /* user ID of owner */
//    gid_t      st_gid;    /* group ID of owner */
//    dev_t      st_rdev;   /* device type, for special file inode */
//    struct timespec st_atimespec;  /* time of last access */
//    struct timespec st_mtimespec;  /* time of last data modification */
//    struct timespec st_ctimespec;  /* time of last file status change */
//    off_t      st_size;   /* file size, in bytes */
//    int64_t    st_blocks; /* blocks allocated for file */
//    u_int32_t  st_blksize;/* optimal file sys I/O ops blocksize */
//    u_int32_t  st_flags;  /* user defined flags for file */
//    u_int32_t  st_gen;    /* file generation number */
//};
string FileOperations::timestampToDate(long timestamp) {
    time_t tt(timestamp);
    struct tm *localTime = localtime(&tt);
    int year = localTime->tm_year + 1900;
    int month = localTime->tm_mon + 1;
    int day = localTime->tm_mday;
    int hour = localTime->tm_hour;
    int min = localTime->tm_min;
    int sec = localTime->tm_sec;
    stringstream ss;
    ss << year << "/" << month << "/" << day << " " << hour << ":" << min << ":" << sec;
    return ss.str();
}

void FileOperations::printDirectoryContent(const FileList& res) {
    if (res.empty()) cout << "Empty directory" << endl;
    cout << "file_type" << "\t" << "time_write" << "\t" << "file_size(Byte)" << "\t" << "file_name" << endl;
    for (auto iter : res) {
        if (iter->is_dir)
            cout << "D\t";
        else
            cout << "-\t";
        cout << iter->time_write << "\t" << iter->size << "\t" << iter->name << endl;
        // 注意释放内存
    }
}


//int main()
//{
//    auto *pro = new Properties();
//    vector<struct FileAttributes*> res;
//    try {
//        res = FileOperations::getAllFilesWhichInOneDirectory(pro->getPropertiesByName("work_directory"));
//    } catch (FtpException &e) {
//        cerr << e.what() << endl;
//    }
//
//    FileOperations::printDirectoryContent(res);
//
//    delete pro;
//    return 0;
//}