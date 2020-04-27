//
// Created by wang on 2020/4/21.
//

#ifndef FTP_FTPEXCEPTION_H
#define FTP_FTPEXCEPTION_H

#include <exception>

#define exit_if(r, ...)                                                \
    if (r) {                                                           \
        printf(__VA_ARGS__);                                           \
        printf("error no: %d; error msg: %s\n", errno, strerror(errno)); \
        exit(1);                                                       \
    }

#define throw_exception(msg) \
    FtpException e(msg); \
    throw e; \

using namespace std;

struct FtpException
{
public:
    explicit FtpException(const string &msg)
    {
        _msg = msg;
    }

    const char * what() const
    {
        return _msg.c_str();
    }

protected:
    string _msg;
};

#endif //FTP_FTPEXCEPTION_H
