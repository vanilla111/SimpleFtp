//
// Created by wang on 2020/4/28.
//
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>

#include <cinttypes>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <utility>

#include "headers/FileTransporter.h"
#include "headers/FtpException.h"
#include "headers/ThreadPool.h"

#define BUFFER_SIZE 4096

const int kReadEvent =  1;
const int kWriteEvent = 2;

FileTransporter::FileTransporter(string filePath) {
    m_strFilePath = std::move(filePath);
    properties = new Properties();
    if (!properties->hasLoadedProperties()) {
        FtpException e("加载配置文件失败");
        throw e;
    }
    m_iPort = atoi(properties->getPropertiesByName("trans_port").c_str());
    m_iListenQueueLen = 5;
    string ip = properties->getPropertiesByName("bind_ip");
    int len = ip.length();
    m_strIP = new char[len + 1];
    memcpy(m_strIP, ip.c_str(), len + 1);
    initialSuccess = false;
    initSocketFd();
}

FileTransporter::~FileTransporter() {
    delete [] m_strIP;
    delete properties;
    close(m_iListenFd);
}

void FileTransporter::run(CommandType type) {
    // 创建kqueue，类似于epoll类型
    int epollfd = kqueue();
    exit_if(epollfd < 0, "kqueue failed");
    setNonBlocking(m_iListenFd);
    // 开启ET模式
    updateEvents(epollfd, m_iListenFd, kReadEvent | kWriteEvent, true);
    // TODO 为该线程设置等待时间，过期自动结束
    if (type == GET) {
        for (;;)
            if (runForGet(epollfd, 50000))
                break;
    } else {
        for (;;)
            if (runForPut(epollfd, 50000))
                break;
    }
}

bool FileTransporter::runForGet(int epollfd, int waiteTime) {
    struct timespec timeout{};
    timeout.tv_sec = waiteTime / 1000;
    timeout.tv_nsec = (waiteTime % 1000) * 1000 * 1000;
    struct kevent activeEvs[2];
    int n = kevent(epollfd, nullptr, 0, activeEvs,2, &timeout);
    for (int i = 0; i < n; ++i) {
        struct kevent kev = activeEvs[i];
        int fd = (int)(intptr_t)kev.udata;
        int events = activeEvs[i].filter;
        // 当客户端接入时就开始传输文件
        if ((kev.flags & EV_EOF) || events == EVFILT_READ) {
            if (fd == m_iListenFd) {
                struct sockaddr_in clientAddr{};
                socklen_t rsz = sizeof(clientAddr);
                int cfd = accept(fd, (struct sockaddr *) &clientAddr, &rsz);
                assert(cfd >= 0);
                sockaddr_in peer{};
                socklen_t alen = sizeof(peer);
                int r = getpeername(cfd, (sockaddr *) &peer, &alen);
                cout << "客户端连接成功: " << inet_ntoa(clientAddr.sin_addr)
                    << "\n即将开始传输文件..." << endl;
                assert(r >= 0);
                setNonBlocking(cfd);
                // 使用零拷贝发送文件
                int filefd = open(m_strFilePath.c_str(), O_RDONLY);
                assert(filefd > 0);
                struct stat statBuf;
                fstat(filefd, &statBuf);
                cout << "文件大小: stat.st_size = " << statBuf.st_size << " Bytes" << endl;
                off_t sendLength = statBuf.st_size;
                off_t hasSendLength = 0;
                while (hasSendLength < statBuf.st_size) {
                    ::sendfile(filefd, cfd, hasSendLength, &sendLength, nullptr, 0);
                    hasSendLength += sendLength;
                    sendLength = statBuf.st_size - hasSendLength;
                    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                }
                cout << "成功发送字节数: " << hasSendLength << endl;
                ::close(filefd);
                ::close(cfd);
                return true;
            }
        }
    }
    return false;
}

bool FileTransporter::runForPut(int epollfd, int waiteTime) {
    struct timespec timeout{};
    timeout.tv_sec = waiteTime / 1000;
    timeout.tv_nsec = (waiteTime % 1000) * 1000 * 1000;
    struct kevent activeEvs[2];
    int n = kevent(epollfd, nullptr, 0, activeEvs,2, &timeout);
    for (int i = 0; i < n; ++i) {
        struct kevent kev = activeEvs[i];
        int fd = (int)(intptr_t)kev.udata;
        int events = activeEvs[i].filter;
        // 当客户端接入时就开始传输文件
        if ((kev.flags & EV_EOF) || events == EVFILT_READ) {
            if (fd == m_iListenFd) {
                struct sockaddr_in clientAddr{};
                socklen_t rsz = sizeof(clientAddr);
                int cfd = accept(fd, (struct sockaddr *) &clientAddr, &rsz);
                assert(cfd >= 0);
                sockaddr_in peer{};
                socklen_t alen = sizeof(peer);
                int r = getpeername(cfd, (sockaddr *) &peer, &alen);
                cout << "客户端连接成功: " << inet_ntoa(clientAddr.sin_addr)
                     << "\n即将开始传输文件..." << endl;
                assert(r >= 0);
                setNonBlocking(cfd);
                updateEvents(epollfd, cfd, kReadEvent, true);
            } else {
                // TODO 这种做法要求对端必须一次性发送完所有的数据，否则会重复覆盖已接收的数据
                char buffer[BUFFER_SIZE];
                bzero(buffer, BUFFER_SIZE);
                FILE *fp = fopen(m_strFilePath.c_str(), "w");
                if (fp == nullptr) {
                    throw_exception("文件打开失败：" + m_strFilePath);
                }
                int length = 0;
                cout << "开始接收文件..." << endl;
                off_t hasRecvLength = 0;
                while ((length = ::recv(fd, buffer, BUFFER_SIZE, 0)) > 0) {
                    if (fwrite(buffer, sizeof(char), length, fp) < length) {
                        fclose(fp);
                        throw_exception("文件写入失败");
                    }
                    hasRecvLength += length;
                    bzero(buffer, BUFFER_SIZE);
                }
                // 接受成功后关闭文件和socket
                cout << "成功接收文件，存储路径为: " << m_strFilePath << endl;
                fclose(fp);
                return true;
            }
        }
    }
    return false;
}

bool FileTransporter::isInitialSuccess() {
    return initialSuccess;
}

int FileTransporter::setNonBlocking(int fd) {
    int oldOpt = fcntl(fd, F_GETFL);
    int newOpt = oldOpt | O_NONBLOCK;
    fcntl(fd, F_SETFL, newOpt);
    return oldOpt;
}

void FileTransporter::initSocketFd() {
    int ret = 0;
    int reuse = 1;
    struct sockaddr_in serverAddress{};
    bzero(&serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    inet_pton(AF_INET, m_strIP, &serverAddress.sin_addr);
    serverAddress.sin_port = htons(m_iPort);

    m_iListenFd = socket(PF_INET, SOCK_STREAM, 0);
    if (m_iListenFd < 0) return;

    ::setsockopt(m_iListenFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    ret = ::bind(m_iListenFd, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    if (ret == -1) return;

    ret = listen(m_iListenFd, m_iListenQueueLen);
    if (ret == -1) return;

    initialSuccess = true;
}

void FileTransporter::updateEvents(int epollfd, int fd, int events, bool enableET) {
    struct timespec now{};
    now.tv_nsec = 0;
    now.tv_sec = 0;
    struct kevent ev[2];
    int n = 0;
    if (events & kReadEvent) {
        EV_SET(&ev[n++], fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, (void *) (intptr_t) fd);
    } else if ((events & kReadEvent) && enableET) {
        EV_SET(&ev[n++], fd, EVFILT_READ, EV_CLEAR, 0, 0, (void *) (intptr_t) fd);
    }
    if (events & kWriteEvent) {
        EV_SET(&ev[n++], fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, (void *) (intptr_t) fd);
    } else if ((events & kWriteEvent) && enableET) {
        EV_SET(&ev[n++], fd, EVFILT_WRITE, EV_CLEAR, 0, 0, (void *) (intptr_t) fd);
    }
    int ret = kevent(epollfd, ev, n, nullptr, 0, &now);
    exit_if(ret, "kevent failed\n ");
}


//int main() {
//    FileTransporter *ft;
//    try {
//        Command *cmd = new Command;
//        cmd->type = PUT;
//        ft = new FileTransporter("/Users/wang/CLionProjects/FTP/Server/work_dir/test.txt");
//        ft->run(cmd);
//    } catch (FtpException &e) {
//        cout << e.what() << endl;
//    }
//
//    for (int i = 0; i < 100; i++) {
//        cout << "test" << endl;
//        sleep(10);
//    }
//    return 0;
//}