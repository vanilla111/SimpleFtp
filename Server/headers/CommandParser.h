//
// Created by wang on 2020/4/25.
//

#ifndef FTP_COMMAND_H
#define FTP_COMMAND_H

#include <iostream>
#include <string>

#define MAX_ABS_PATH_LEN 1024 /* 定义绝对路径的长度最大字节 */
#define MAX_COMMAND_LENGTH 16 /* 命令的最大长度 */

using namespace std;

enum CommandType {
    OPEN, LS, CD, GET, PUT
};

struct Command {
    CommandType type;
    string firstParameter;
    string secondParameter;
};

class CommandParser {
public:
    static Command *parseCommandString(const char *line, int length);
    static void parseCD(Command *command, char *lastDir);

public:
    static string computeNextPath(string currentDir, string parameter);

protected:
    static void currentPathStat(char *current, int *c_offset, char *next, int *n_offset);
    static void rootStat(char *current, int *c_offset, char *next, int *n_offset);
    static void oneDotStat(char *current, int *c_offset, char *next, int *n_offset);
    static void twoDotStat(char *current, int *c_offset, char *next, int *n_offset);
    static void centralStat(char *current, int *c_offset, char *next, int *n_offset);

    static char * strToLower(char *str);
};

#endif //FTP_COMMAND_H
