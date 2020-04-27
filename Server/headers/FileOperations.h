//
// Created by wang on 2020/4/21.
//

#ifndef FTP_FILEOPERATIONS_H
#define FTP_FILEOPERATIONS_H

#include <iostream>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

struct FileAttributes {
    bool is_dir;
    string name;
    unsigned long size;
    string time_write;
};

typedef vector<struct FileAttributes*> FileList;

class FileOperations
{
public:
    static FileList getAllFilesWhichInOneDirectory(const string& dirname);
    static void printDirectoryContent(const FileList& res);

private:
    static string timestampToDate(long timestamp);
    //string changeFileSizeForm(unsigned long);
};

#endif //FTP_FILEOPERATIONS_H
