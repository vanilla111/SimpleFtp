//
// Created by wang on 2020/4/25.
//

#ifndef FTP_COMMANDEXECUTOR_H
#define FTP_COMMANDEXECUTOR_H

#include "Response.h"
#include "CommandParser.h"

class CommandExecutor {
public:
    CommandExecutor(string workDir);
    ~CommandExecutor();
    Response * executeCommand(Command *command);

protected:
    void executeLS(Response *response);
    void executeCD(Command *command, Response *response);
    void executeGET(Command *command, Response *response);
    void executePUT(Command *command, Response *response);
    static string getFileName(string path);

private:
    string m_strWorkDir;
    string m_strCurrentDir;
    char m_strLastDir[MAX_ABS_PATH_LEN];

};

#endif //FTP_COMMANDEXECUTOR_H
