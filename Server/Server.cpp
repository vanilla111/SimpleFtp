//
// Created by wang on 2020/4/22.
//

#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cinttypes>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <map>

#include "headers/Server.h"
#include "headers/FtpException.h"
#include "headers/CommandParser.h"
#include "headers/Response.h"

#define MAX_EVENT_NUMBER 20
#define BUFFER_SIZE 4096

using namespace std;

const int kReadEvent =  1;
const int kWriteEvent = 2;

Server::Server() {
    // 从配置文件中初始化必要参数
    properties = new Properties();
    if (!properties->hasLoadedProperties()) {
        FtpException e("加载配置文件失败，程序将退出。");
        throw e;
    }
    m_iPort = atoi(properties->getPropertiesByName("listen_port").c_str());
    m_iListenQueueLen = atoi(properties->getPropertiesByName("listen_queue_length").c_str());
    string ip = properties->getPropertiesByName("bind_ip");
    int len = ip.length();
    m_strIP = new char[len + 1];
    memcpy(m_strIP, ip.c_str(), len + 1);
    executor = new CommandExecutor(properties->getPropertiesByName("work_directory"));
}

Server::~Server() {
    delete [] m_strIP;
    delete properties;
    delete executor;
    close(m_iListenFd);
}

void Server::run() {
    int ret = 0;
    int reuse = 1;
    struct sockaddr_in serverAddress{};
    bzero(&serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    inet_pton(AF_INET, m_strIP, &serverAddress.sin_addr);
    serverAddress.sin_port = htons(m_iPort);

    m_iListenFd = socket(PF_INET, SOCK_STREAM, 0);
    assert(m_iListenFd >= 0);

    ::setsockopt(m_iListenFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    ret = ::bind(m_iListenFd, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    assert(ret != -1);

    ret = listen(m_iListenFd, m_iListenQueueLen);
    assert(ret != -1);

    // 创建kqueue，类似于epoll类型
    int epollfd = kqueue();
    exit_if(epollfd < 0, "kqueue failed");
    setNonBlocking(m_iListenFd);
    // 开启ET模式
    updateEvents(epollfd, m_iListenFd, kReadEvent | kWriteEvent, true);

    for(;;) {
        loopOnce(epollfd, 50000);
    }
}

int Server::setNonBlocking(int fd) {
    int oldOpt = fcntl(fd, F_GETFL);
    int newOpt = oldOpt | O_NONBLOCK;
    fcntl(fd, F_SETFL, newOpt);
    return oldOpt;
}

bool Server::isInitialSuccess() {
    return initialSuccess;
}

void Server::handleAccept(int epollfd, int fd) {
    struct sockaddr_in clientAddr{};
    socklen_t rsz = sizeof(clientAddr);
    int cfd = accept(fd, (struct sockaddr *) &clientAddr, &rsz);
    assert(cfd >= 0);
    sockaddr_in peer{};
    socklen_t alen = sizeof(peer);
    int r = getpeername(cfd, (sockaddr *) &peer, &alen);
    assert(r >= 0);
    cout << "accept a connection from " << inet_ntoa(clientAddr.sin_addr) << endl;
    setNonBlocking(cfd);
    updateEvents(epollfd, cfd, kReadEvent | kWriteEvent, true);
}

void Server::handleRead(int eppllfd, int fd) {
    char buf[BUFFER_SIZE];
    int onceReadLen = 0;  // 一次性读取的数据字节长度
    int offset = 0;  // 下一次读取数据在buffer中存放的偏移量
    // ET 模式下，如果数据一次性读不完，必须多次读直到读完，否则不再通知
    while(offset < BUFFER_SIZE && (onceReadLen = ::read(fd, buf + offset, BUFFER_SIZE - offset)) > 0) {
        offset += onceReadLen;
    }
    bool valid = (errno == EAGAIN || errno == EWOULDBLOCK);
    if (offset >= BUFFER_SIZE) {
        // 命令长度过长，告知客户端错误
    }
    buf[offset] = '\0';
    if (offset > 0) {
        cout << "接收到[" << offset << "]字节的数据：" << buf << endl;
        // 根据命令准备不同的写数据，返回给客户端
        Response *response;
        try {
            Command *command = CommandParser::parseCommandString(buf, offset);
            response = executor->executeCommand(command);
        } catch (FtpException &e) {
            response = new Response;
            response->type = FAILED;
            strcpy(response->message, e.what());
        }
        int sendLen = sizeof(Response);
        char sendBuf[sendLen];
        memcpy(sendBuf, (char*)response, sendLen);
        int r = ::write(fd, sendBuf, sendLen);
        delete response;
        // 实际应用中，写出数据可能会返回EAGAIN，此时应当监听可写事件，当可写时再把数据写出
        exit_if(r <= 0, "write error");
    }
    // 对于非阻塞I/O，下面条件成立表示数据已经被全部读取完毕
    if (onceReadLen < 0 && valid)
        return;
    exit_if(onceReadLen < 0, "read error\n");  // 实际应用中，onceReadLen<0应当检查各类错误，如EINTR
    cout << "fd [" << fd << "] closed." << endl; // 对端关闭连接
    close(fd);
}

void Server::handleWrite(int epollfd, int fd) {
    // 实际应用应当实现可写时写出数据，无数据可写才关闭可写事件
    //updateEvents(epollfd, fd, kReadEvent, true);
}

// flags
//#define EV_ONESHOT          0x0010		/* only report one occurrence */
//#define EV_CLEAR            0x0020		/* clear event state after reporting(ET模式) */
void Server::updateEvents(int epollfd, int fd, int events, bool enableET) {
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

void Server::loopOnce(int epollfd, int waiteTime) {
    struct timespec timeout{};
    timeout.tv_sec = waiteTime / 1000;
    timeout.tv_nsec = (waiteTime % 1000) * 1000 * 1000;
    struct kevent activeEvs[MAX_EVENT_NUMBER];
    int n = kevent(epollfd, nullptr, 0, activeEvs, MAX_EVENT_NUMBER, &timeout);
    for (int i = 0; i < n; ++i) {
        struct kevent kev = activeEvs[i];
        int fd = (int)(intptr_t)kev.udata;
        int events = activeEvs[i].filter;
        // 仅处理读写事件
        if ((kev.flags & EV_EOF) || events == EVFILT_READ) {
            if (fd == m_iListenFd)
                handleAccept(epollfd, fd);
            else
                handleRead(epollfd, fd);
        } else if (!(kev.flags & EV_EOF) && events == EVFILT_WRITE) {
            handleWrite(epollfd, fd);
        } else {
            exit_if(1, "Unknown event\n");
        }
    }
}

int main()
{
    try {
        auto *server = new Server;
        server->run();
    } catch (FtpException &e) {
        cerr << e.what() << endl;
    }

    return 0;
}
