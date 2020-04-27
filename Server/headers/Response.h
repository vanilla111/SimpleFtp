//
// Created by wang on 2020/4/26.
//

#ifndef FTP_RESPONSE_H
#define FTP_RESPONSE_H

#define RESPONSE_BUFFER_SIZE 2048

enum ResponseType {
    SUCCESS, FAILED, SERVER_ERROR
};

struct Response {
    ResponseType type;
    char message[RESPONSE_BUFFER_SIZE];
};

#endif //FTP_RESPONSE_H
