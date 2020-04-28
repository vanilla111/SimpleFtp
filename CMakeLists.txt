cmake_minimum_required(VERSION 3.15)
project(FTP)

set(CMAKE_CXX_STANDARD 14)

add_executable(FTP main.cpp)

add_executable(Properties Server/Properties.cpp)

add_executable(FileOperations
        Server/Properties.cpp
        Server/FileOperations.cpp)

add_executable(Server
        Server/Properties.cpp
        Server/CommandParser.cpp
        Server/FileOperations.cpp
        Server/CommandExecutor.cpp
        Server/Server.cpp)

add_executable(CommandParser
        Server/CommandParser.cpp)

add_executable(CommandExecutor
        Server/CommandParser.cpp
        Server/FileOperations.cpp
        Server/CommandExecutor.cpp)

add_executable(FileTransporter
        Server/Properties.cpp
        Server/FileTransporter.cpp)

add_executable(GET_PUT_TEST
        CLient/CFileTransporter.cpp)