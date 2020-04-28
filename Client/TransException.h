//
// Created by wang on 2020/4/28.
//

#ifndef FTP_TRANSEXCEPTION_H
#define FTP_TRANSEXCEPTION_H

#include <exception>

#define exit_if(r, ...)                                                \
    if (r) {                                                           \
        printf(__VA_ARGS__);                                           \
        printf("error no: %d; error msg: %s\n", errno, strerror(errno)); \
        exit(1);                                                       \
    }

#define throw_exception(msg) \
    TransException e(msg); \
    throw e; \

using namespace std;

struct TransException: public exception
{
public:
    explicit TransException(const string &msg)
    {
        _msg = msg;
    }

    const char * what() const noexcept override
    {
        return _msg.c_str();
    }

protected:
    string _msg;
};

#endif //FTP_TRANSEXCEPTION_H
