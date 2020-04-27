//
// Created by wang on 2020/4/26.
//
#include <sstream>

#include "headers/CommandParser.h"
#include "headers/CommandExecutor.h"
#include "headers/FileOperations.h"
#include "headers/FtpException.h"

#include <utility>

using namespace std;

CommandExecutor::CommandExecutor(string workDir) {
    m_strWorkDir = std::move(workDir);
    m_strCurrentDir = "/";
    strcpy(m_strLastDir, m_strCurrentDir.c_str());
}

CommandExecutor::~CommandExecutor() {
    // do something
}

Response * CommandExecutor::executeCommand(Command *command) {
    auto *response = new Response;
    response->type = SUCCESS;
    if (command->type == CD) {
        executeCD(command, response);
    } else if (command->type == LS) {
        executeLS(response);
    } else if (command->type == GET) {
        executeGET(command, response);
    } else if (command->type == PUT) {
        executePUT(command, response);
    } else {
        response->type = FAILED;
        string msg = "Unknown command";
        strcpy(response->message, msg.c_str());
    }
    return response;
}

void CommandExecutor::executeCD(Command *command, Response *response) {
    command->firstParameter = m_strCurrentDir;
    CommandParser::parseCD(command, m_strLastDir);
    if (command->secondParameter[command->secondParameter.length() - 1] != '/')
        m_strCurrentDir = command->secondParameter + '/';
    else
        m_strCurrentDir = command->secondParameter;
    string msg = "Change directory success, and current directory is: " + m_strCurrentDir;
    strcpy(response->message, msg.c_str());
    // TODO 此处应该判断，路径是否存在，是否是文件夹，并且判断是否有x权限，即进入权限
}

void CommandExecutor::executeLS(Response *response) {
    FileList res;
    try {
        res = FileOperations::getAllFilesWhichInOneDirectory(m_strWorkDir + m_strCurrentDir);
        // TODO 此处应该判断该文件夹是否可读
        stringstream ss;
        if (res.empty()) {
            ss << "Empty Directory";
        } else {
            ss << "file_type" << "\t" << "time_write" << "\t" << "file_size(Byte)" << "\t" << "file_name" << endl;
            for (auto iter : res) {
                if (iter->is_dir)
                    ss << "\tD\t\t";
                else
                    ss << "\t-\t\t";
                ss << iter->time_write << "\t" << iter->size << "\t" << iter->name << endl;
                delete iter;
            }
        }
        strcpy(response->message, ss.str().c_str());
    } catch (FtpException &e) {
        response->type = FAILED;
        strcpy(response->message, e.what());
    }
}

/**
 * 执行GET命令，首先检查文件是否合法
 * 若合法，response.messsage为文件大小，否则为非法的原因
 * 之后开启一个线程启动服务，等待客户端连接和文件传输
 * @param command
 * @param response
 */
void CommandExecutor::executeGET(Command *command, Response *response) {
    // 得到文件所在的绝对路径
    string easyName = CommandParser::computeNextPath(m_strCurrentDir, command->firstParameter);
    string fileName = m_strWorkDir + easyName;
    // 文件属性
    struct stat fileStat{};
    bool valid;
    stringstream ss;
    if ((stat(fileName.c_str(), &fileStat)) < 0) {
        valid = false;
        ss << "File not found: " << easyName;
    } else {
        if (S_ISDIR(fileStat.st_mode)) {
            valid = false;
            ss << "Directory doesn't support to download: " << easyName;
        } else if (fileStat.st_mode & S_IROTH) {
            // 用户拥有读该文件的权限
            valid = true;
            ss << fileStat.st_size;
        } else {
            valid = false;
            ss << "Permission denied";
        }
    }
    strcpy(response->message, ss.str().c_str());
    if (valid) {
        response->type = SUCCESS;
    } else {
        response->type = FAILED;
    }
}

/**
 * 执行PUT命令，需要检查存储路径是否合法
 * 即文件夹是否具有 rwx，严谨的还需要检查剩余空间大小等
 * 之后开启一个线程启动服务，等待客户端连接和文件传输
 * @param command
 * @param response
 */
void CommandExecutor::executePUT(Command *command, Response *response) {
    // 得到文件所在的绝对路径
    string easyName = CommandParser::computeNextPath(m_strCurrentDir, command->secondParameter);
    string fileName = m_strWorkDir + easyName;
    // 文件属性
    struct stat fileStat{};
    bool valid = true;
    stringstream ss;
    if ((stat(fileName.c_str(), &fileStat)) < 0) {
        valid = false;
        ss << "Directory not found: " << easyName;
    } else {
        if (!S_ISDIR(fileStat.st_mode)) {
            valid = false;
            ss << "This is not a valid storage path: " << easyName <<
            "\n The storage path must be a directory.";
        } else if (!(fileStat.st_mode & S_IXOTH)) {
            valid = false;
            ss << "Don't have X permission";
        } else if (!(fileStat.st_mode & S_IROTH)) {
            valid = false;
            ss << "Don't have R permission";
        } else if (!(fileStat.st_mode & S_IWOTH)) {
            valid = false;
            ss << "Don't have W permission";
        }
    }
    strcpy(response->message, ss.str().c_str());
    if (valid) {
        response->type = SUCCESS;
    } else {
        response->type = FAILED;
    }
}

//int main() {
//    /* test serialize */
//    Response *res = new Response;
//    res->type = SUCCESS;
//    string tmp = "successful";
//    strcpy(res->message, tmp.c_str());
//    char test[RESPONSE_BUFFER_SIZE + sizeof(ResponseType)];
//    memcpy(test, (char*)res, RESPONSE_BUFFER_SIZE + sizeof(ResponseType));
//    Response *res_2 = (Response*)test;
//    cout << (res_2->type == SUCCESS ? "true" : "false") << endl;
//    cout << res_2->message << endl;
//
//    return 0;
//}