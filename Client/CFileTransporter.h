//
// Created by wang on 2020/4/28.
//

#ifndef FTP_CFILETRANSPORTER_H
#define FTP_CFILETRANSPORTER_H

#include <iostream>

using namespace std;

#define BUFFER_SIZE 4096

class CFileTransporter {
public:
    CFileTransporter(string ip, int port, string filePath);
    ~CFileTransporter();
    bool runForPut();
    bool runForGet(off_t fileSize);

protected:
    // void connectServer();

private:
    int m_intSockfd;
    string m_strFilePath;
    int m_iPort;
    char *m_strIP;
};

#endif //FTP_CFILETRANSPORTER_H
