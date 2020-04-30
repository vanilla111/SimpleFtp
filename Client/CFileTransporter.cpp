//
// Created by wang on 2020/4/28.
//
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <fcntl.h>

#include "TransException.h"
#include "CFileTransporter.h"
#include "ThreadPool.h"

using namespace std;

CFileTransporter::CFileTransporter(string ip, int port, string filePath) {
    m_strFilePath = std::move(filePath);
    m_iPort = port;
    int len = ip.length();
    m_strIP = new char[len + 1];
    memcpy(m_strIP, ip.c_str(), len + 1);
}

CFileTransporter::~CFileTransporter() {
    delete [] m_strIP;
    ::close(m_intSockfd);
}

bool CFileTransporter::runForGet(off_t fileSize) {
    int reuse = 1;
    int ret = 0;
    struct sockaddr_in targetServer{};
    bzero(&targetServer, sizeof(targetServer));
    targetServer.sin_family = AF_INET;
    targetServer.sin_port = htons(m_iPort);
    inet_pton(AF_INET, m_strIP, &targetServer.sin_addr);

    m_intSockfd = socket(PF_INET, SOCK_STREAM, 0);
    setsockopt(m_intSockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    ret = connect(m_intSockfd, (struct sockaddr*)&targetServer, sizeof(targetServer));
    assert(ret >= 0);
    char buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);

    FILE *fp = fopen(m_strFilePath.c_str(), "w");
    if (fp == nullptr) {
        throw_exception("文件打开失败：" + m_strFilePath);
    }
    int length = 0;
    cout << "开始下载文件..." << endl;
    off_t hasRecvLength = 0;
    while ((length = ::recv(m_intSockfd, buffer, BUFFER_SIZE, 0)) > 0) {
        if (fwrite(buffer, sizeof(char), length, fp) < length) {
            fclose(fp);
            throw_exception("文件写入失败");
        }
        hasRecvLength += length;
        // 打印进度条
        double tmp = (hasRecvLength * 100.0) / (fileSize * 1.0);
        stringstream  ss;
        ss << fixed << setprecision(2) << tmp << "%\r";
        cout << ss.str();
        //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        bzero(buffer, BUFFER_SIZE);
    }
    cout << endl;
    // 接受成功后关闭文件和socket
    cout << "成功接收文件，存储路径为: " << m_strFilePath << endl;
    fclose(fp);
    return true;
}

bool CFileTransporter::runForPut() {
    int reuse = 1;
    int ret = 0;
    struct sockaddr_in targetServer;
    bzero(&targetServer, sizeof(targetServer));
    targetServer.sin_family = AF_INET;
    targetServer.sin_port = htons(m_iPort);
    inet_pton(AF_INET, m_strIP, &targetServer.sin_addr);

    m_intSockfd = socket(PF_INET, SOCK_STREAM, 0);
    setsockopt(m_intSockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    ret = connect(m_intSockfd, (struct sockaddr*)&targetServer, sizeof(targetServer));
    assert(ret >= 0);
    char buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);

    int filefd = open(m_strFilePath.c_str(), O_RDONLY);
    if (filefd < 0) {
        throw_exception("文件打开失败：" + m_strFilePath);
    }
    cout << "开始上传文件..." << endl;
    struct stat statBuf;
    fstat(filefd, &statBuf);
    cout << "文件大小: stat.st_size = " << statBuf.st_size << " Bytes" << endl;
    off_t sendLength = statBuf.st_size;
    //off_t fileSize = statBuf.st_size;
    off_t hasSendLength = 0;
    while (hasSendLength < statBuf.st_size) {
        ::sendfile(filefd, m_intSockfd, hasSendLength, &sendLength, nullptr, 0);
        hasSendLength += sendLength;
        sendLength = statBuf.st_size - hasSendLength;
        //std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        // 打印进度条
//        double tmp = (hasSendLength * 100.0) / (fileSize * 1.0);
//        stringstream  ss;
//        ss << fixed << setprecision(2) << tmp << "%\r";
//        cout << ss.str();
    }
    cout << "上传成功,成功发送字节数: " << hasSendLength << endl;
    ::close(filefd);
    return true;
}

//int main()
//{
//    auto *transporter = new CFileTransporter("127.0.0.1", 7001, "/Users/wang/CLionProjects/FTP/test.txt");
//    try {
//        off_t fileSize = 10485760;
//        transporter->runForGet(fileSize);
//
//        //transporter->runForPut();
//    } catch (TransException &e) {
//        cerr << e.what() << endl;
//    }
//    delete transporter;
//
//    return 0;
//}