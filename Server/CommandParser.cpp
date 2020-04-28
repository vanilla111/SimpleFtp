//
// Created by wang on 2020/4/25.
//
#include "headers/CommandParser.h"
#include "headers/FtpException.h"

using namespace std;

/**
 * 解析CD命令，计算下一个"当前"目录的绝对路径，结果存放在secondParameter
 * 如果是 'cd -',则交换对应的路径
 * firstParameter 当前目录（相对于工作目录的绝对路径）
 * secondParameter 目标目录
 * @param command
 */
void CommandParser::parseCD(Command *command, char *lastDir) {
    if (command->type == CD) {
        if (!command->secondParameter.empty() && command->secondParameter == "-") {
            command->secondParameter = lastDir;
            strcpy(lastDir, command->firstParameter.c_str());
        } else {
            strcpy(lastDir, command->firstParameter.c_str());
            command->secondParameter = CommandParser::computeNextPath(command->firstParameter, command->secondParameter);
        }
    }
}

/**
 * 例如currentDir =  /home/wang
 * parameter = /local/otherDir（根目录出发）
 *  testDir（非根目录出发）
 * 1/2/../../3/../4 （带有多个返回上级目录，但是最多只能回到设定的根目录）
 * @param currentDir
 * @param parameter
 * @return
 */
string CommandParser::computeNextPath(string currentDir, string parameter) {
    char current[MAX_ABS_PATH_LEN];
    char next[MAX_ABS_PATH_LEN];
    strcpy(current, currentDir.c_str());
    strcpy(next, parameter.c_str());
    int c_offset = currentDir.length();
    int n_offset = 0;
    CommandParser::currentPathStat(current, &c_offset, next, &n_offset);
    current[c_offset] = '\0';
    string res = current;
    return res;
}

void CommandParser::currentPathStat(char *current, int *c_offset, char *next, int *n_offset) {
    // 最初调用时，c_offset在current最后一个'/'的后面，n_offset指向next的第一个
    char nextChar = next[*n_offset];
    current[*c_offset] = nextChar;
    (*c_offset)++; (*n_offset)++;
    if (nextChar == '\0') {
        return;
    } else if ((*n_offset) == 1 && nextChar == '/') {
        CommandParser::rootStat(current, c_offset, next, n_offset);
    } else if (nextChar == '/') {
        (*c_offset)--;
        while(next[*n_offset] == '/') (*n_offset)++;
        CommandParser::currentPathStat(current, c_offset, next, n_offset);
    } else if (nextChar == '.') {
        CommandParser::oneDotStat(current, c_offset, next, n_offset);
    } else {
        CommandParser::centralStat(current, c_offset, next, n_offset);
    }
}

void CommandParser::rootStat(char *current, int *c_offset, char *next, int *n_offset) {
    *c_offset = 1;
    while (next[*n_offset] == '/') (*n_offset)++;
    char nextChar = next[*n_offset];
    current[*c_offset] = nextChar;
    (*c_offset)++; (*n_offset)++; // 全部移动到将要访问的下一位
    if (nextChar == '\0') {
        return;
    } else if (nextChar == '.') {
        CommandParser::oneDotStat(current, c_offset, next, n_offset);
    } else {
        CommandParser::centralStat(current, c_offset, next, n_offset);
    }
}

void CommandParser::oneDotStat(char *current, int *c_offset, char *next, int *n_offset) {
    char nextChar = next[*n_offset];
    current[*c_offset] = nextChar;
    (*c_offset)++; (*n_offset)++;
    if (nextChar == '\0') {
        (*c_offset) -= 2;
    } else if (nextChar == '/') {
        (*c_offset) -= 2;
        CommandParser::currentPathStat(current, c_offset, next, n_offset);
    } else if (nextChar == '.') {
        CommandParser::twoDotStat(current, c_offset, next, n_offset);
    } else {
        CommandParser::centralStat(current, c_offset, next, n_offset);
    }
}

void CommandParser::twoDotStat(char *current, int *c_offset, char *next, int *n_offset) {
    char nextChar = next[*n_offset];
    current[*c_offset] = nextChar;
    (*c_offset)++; (*n_offset)++;
    if (nextChar == '\0' || nextChar == '/') {
        // 返回上一级目录
        if (*c_offset - 5 > 0) {
            (*c_offset) -= 4;
            while (current[(*c_offset) - 1] != '/')
                (*c_offset)--;
        } else {
            (*c_offset) -= 3;
        }
        if (nextChar == '\0') (*n_offset)--;
        CommandParser::currentPathStat(current, c_offset, next, n_offset);
    } else {
        CommandParser::centralStat(current, c_offset, next, n_offset);
    }
}

void CommandParser::centralStat(char *current, int *c_offset, char *next, int *n_offset) {
    char nextChar;
    do {
        nextChar = next[*n_offset];
        current[*c_offset] = nextChar;
        (*c_offset)++; (*n_offset)++;
    } while (nextChar != '/' && nextChar != '\0');
    if (nextChar == '\0') {
        return;
    } else {
        while (next[*n_offset] == '/') (*n_offset)++;
        CommandParser::currentPathStat(current, c_offset, next, n_offset);
    }
}

/**
 * 解析客户端传输过来的字符串，将其变为Command对象
 * 命令    参数一    参数二
 * CD     路径       -
 * LS     -         -
 * GET    目标文件   存储路径
 * PUT    本地文件   存储路径
 * OPEN   ip地址     端口（默认7001）
 * @param line
 * @param length
 * @return
 */
Command *CommandParser::parseCommandString(const char *line, int length) {
    if (length < 2) return nullptr; // 太短了，直接算错误
    int offset = 0;
    char *commandStr = new char[length + 1]; // 最长的一个命令长度为4（OPEN）
    char firstParameter[length];
    char secondParameter[length];
    // 跳过空格 读取命令
    int i = 0;
    while ((offset < length) && (line[offset] == ' ')) offset++;
    while (offset < length && line[offset] != ' ') commandStr[i++] = line[offset++];
    commandStr[i] = '\0';

    // 读取第一个参数
    i = 0;
    while (offset < length && line[offset] == ' ') offset++;
    while (offset < length && line[offset] != ' ') firstParameter[i++] = line[offset++];
    firstParameter[i] = '\0';

    // 读取第二个参数
    i = 0;
    while (offset < length && line[offset] == ' ') offset++;
    while (offset < length && line[offset] != ' ') secondParameter[i++] = line[offset++] ;
    secondParameter[i] = '\0';

    commandStr = CommandParser::strToLower(commandStr);
    Command *command = new Command;
    command->firstParameter = firstParameter;
    command->secondParameter = secondParameter;
    if (strcmp(commandStr, "cd") == 0) {
        command->type = CD;
        command->secondParameter = firstParameter;
    } else if (strcmp(commandStr, "ls") == 0) {
        command->type = LS;
    } else if (strcmp(commandStr, "get") == 0) {
        command->type = GET;
    } else if (strcmp(commandStr, "put") == 0) {
        command->type = PUT;
    } else if (strcmp(commandStr, "open") == 0) {
        command->type = OPEN;
    } else {
        string str = commandStr;
        delete[] commandStr;
        throw_exception("command not found: " + str);
    }
    delete[] commandStr;
    return command;
}

char *CommandParser::strToLower(char *str) {
    char *origin = str;
    for (; *str != '\0'; str++)
        *str = tolower(*str);
    return origin;
}

//int main() {
//    string cmd_1 = "cd /1/../2/3/4/6/7";
//    string cmd_2 = "ls";
//    string cmd_3 = "get readme.md ./download/";
//    string cmd_4 = "put /../test/word.txt /home/wang/";
//    int length = cmd_4.length();
//    char *cmd = new char[length + 1];
//    strcpy(cmd, cmd_4.c_str());
//    Command *command = CommandParser::parseCommandString(cmd, length);
//    cout << command->type << endl;
//    cout << command->firstParameter << endl;
//    cout << command->secondParameter << endl;
//
//    auto *cmd = new Command;
//    cmd->type = CD;
//    cmd->firstParameter = "/1/";
//    cmd->secondParameter = "1/././2";
//    char lastDir[1024];
//    lastDir[0] = '/'; lastDir[1] = '\0';
//    CommandParser::parseCD(cmd, lastDir);
//    cout << cmd->firstParameter << endl;
//    cout << cmd->secondParameter << endl;
//    cout << lastDir << endl;
//    return 0;
//}
