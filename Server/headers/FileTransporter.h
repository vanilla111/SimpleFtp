//
// Created by wang on 2020/4/28.
//

#ifndef FTP_FILETRANSPORTER_H
#define FTP_FILETRANSPORTER_H

#include <iostream>
#include "Properties.h"
#include "CommandParser.h"

using namespace std;

class FileTransporter {
public:
    FileTransporter(string filePath);
    ~FileTransporter();
    void run(CommandType type);
    bool isInitialSuccess();

protected:
    bool runForGet(int epollfd, int waiteTime);
    bool runForPut(int epollfd, int waiteTime);
    int setNonBlocking(int fd);
    void initSocketFd();
    void updateEvents(int epollfd, int fd, int events, bool enableET);

private:
    string m_strFilePath;
    int m_iListenFd;
    int m_iPort;
    char* m_strIP;
    int m_iListenQueueLen;
    bool initialSuccess;
    Properties *properties;
};

#endif //FTP_FILETRANSPORTER_H
