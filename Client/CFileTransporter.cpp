//
// Created by wang on 2020/4/28.
//
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstdlib>

#include "TransException.h"
#include "CFileTransporter.h"

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
        long long tmp = hasRecvLength / fileSize * 100;
        cout << "(" << hasRecvLength << "/" << fileSize << ")";
        for (int i = 0; i < 50; i++) {
            if (i <= (tmp / 2))
                cout << ">";
            else
                cout << " ";
        }
        cout << " " << tmp << "%\r";
        bzero(buffer, BUFFER_SIZE);
    }
    cout << endl;
    // 接受成功后关闭文件和socket
    cout << "成功接受文件，存储路径为: " << m_strFilePath << endl;
    fclose(fp);
}

int main()
{
    auto *transporter = new CFileTransporter("127.0.0.1", 7001, "/Users/wang/CLionProjects/FTP/test.txt");
    try {
        off_t fileSize = 131072;
        transporter->runForGet(fileSize);
    } catch (TransException &e) {
        cerr << e.what() << endl;
    }
    delete transporter;

    return 0;
}