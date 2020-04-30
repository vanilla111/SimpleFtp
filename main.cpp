#include <sys/socket.h>
#include <netinet/in.h>
#include <memory.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <cerrno>
#include <sstream>
#include <poll.h>

#include "Server/headers/CommandParser.h"
#include "Server/headers/FtpException.h"
#include "Client/CFileTransporter.h"
#include "Client/TransException.h"
#include "Client/Color.h"

#define SEND_BUFFER_SIZE 4096
#define RESPONSE_BUFFER_SIZE 2048

enum ResponseType {
    SUCCESS, FAILED, SERVER_ERROR
};

struct Response {
    ResponseType type;
    char message[RESPONSE_BUFFER_SIZE];
};

using namespace std;

char * strToLower(char *str) {
    char * origin = str;
    for (;*str != '\0'; str++)
        *str = tolower(*str);
    return origin;
}

string getFileName(string path) {
    if (path.empty())
        return std::string();
    return path.substr(path.find_last_of('/') + 1);
}

int main() {
    const char *ip = "127.0.0.1";
    string workDir = "/Users/wang/CLionProjects/FTP/";
    int port = 7000;
    int reuse = 1;
    int ret = 0;
    int sockfd;
    struct sockaddr_in targetServer;
    bzero(&targetServer, sizeof(targetServer));
    targetServer.sin_family = AF_INET;
    targetServer.sin_port = htons(port);
    inet_pton(AF_INET, ip, &targetServer.sin_addr);

    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    ret = connect(sockfd, (struct sockaddr*)&targetServer, sizeof(targetServer));
    assert(ret >= 0);

    char sendBuffer[SEND_BUFFER_SIZE];
    char recvBuffer[sizeof(Response)];
    string line;
    Command *command;
    for (;;) {
        memset(sendBuffer, '\0', sizeof(sendBuffer));
        cout << "ftp> ";
        getline(cin, line);
        strcpy(sendBuffer, line.c_str());
        if (strcmp(strToLower(sendBuffer), "exit") == 0)
            break;
        try {
            command = CommandParser::parseCommandString(sendBuffer, line.length());
        } catch (FtpException &e) {
            setColor(F_RED);
            cout << e.what() << endl;
            resetFColor();
            continue;
        }
        send(sockfd, sendBuffer, strlen(sendBuffer), 0);

        pollfd fds[1];
        fds[0].fd = sockfd;
        fds[0].events = POLLIN | POLLHUP;
        fds[0].revents = 0;

        while (true)
        {
            int ret = ::poll(fds, 1, -1);
            if (ret < 0)
            {
                cout << "poll failure" << endl;
                break;
            }
            if (fds[0].revents & POLLHUP)
            {
                cout << "server close the connection" << endl;
                exit(1);
            }
            else if (fds[0].revents & POLLIN)
            {
                ::recv(sockfd, recvBuffer, sizeof(Response), 0);
                Response *response = (Response*) recvBuffer;
                //cout << "Response: type = " << response->type << ", msg = " << response->message << endl;
                if (response->type == SUCCESS) {
                    // TODO 处理各种错误
                    if (command->type == GET) {
                        cout << "处理GET命令，存储路径为: "<< workDir << getFileName(command->firstParameter) << endl;
                        off_t fileSize = strtoll(response->message, nullptr, 0);
                        auto *transporter = new CFileTransporter("127.0.0.1", 7001, workDir + getFileName(command->firstParameter));
                        try {
                            transporter->runForGet(fileSize);
                        } catch (TransException &e) {
                            setColor(F_RED);
                            cout << "文件传输异常: " << e.what() << endl;
                            resetFColor();
                            delete transporter;
                        }
                    } else if (command->type == PUT) {
                        cout << "处理PUT命令，需要传输文件的存储路径为: "<< workDir << getFileName(command->firstParameter) << endl;
                        auto *transporter = new CFileTransporter("127.0.0.1", 7001, workDir + getFileName(command->firstParameter));
                        try {
                            transporter->runForPut();
                        } catch (TransException &e) {
                            setColor(F_RED);
                            cout << "文件传输异常: " << e.what() << endl;
                            resetFColor();
                            delete transporter;
                        }
                    } else {
                        cout << response->message << endl;
                    }
                } else {
                    setColor(F_RED);
                    cout << response->message << endl;
                    resetFColor();
                }
                break;
            }
        }
        delete command;
    }

    close(sockfd);
    return 0;
}
