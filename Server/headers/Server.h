//
// Created by wang on 2020/4/22.
//

#ifndef FTP_SERVER_H
#define FTP_SERVER_H

#include <iostream>
#include "Properties.h"
#include "CommandExecutor.h"

using namespace std;

class Server
{
public:
    Server();
    ~Server();
    void run();
    bool isInitialSuccess();

protected:
    int setNonBlocking(int fd);
    // void enableET(int epollfd, int fd);
    void handleAccept(int epollfd, int fd);
    void handleRead(int eppllfd, int fd);
    void handleWrite(int epollfd, int fd);
    void updateEvents(int epollfd, int fd, int events, bool enableET);
    void loopOnce(int epollfd, int waiteTime);

private:
    int m_iListenFd;
    int m_iPort;
    char* m_strIP;
    int m_iListenQueueLen;
    Properties *properties;
    bool initialSuccess;
    CommandExecutor *executor;
};

#endif //FTP_SERVER_H
