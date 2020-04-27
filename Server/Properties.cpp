//
// Created by wang on 2020/4/21.
//

#include <iostream>
#include <fstream>
#include "headers/Properties.h"

using namespace std;

const string Properties::configFilePath = "/Users/wang/CLionProjects/FTP/Server/config.txt";
const int Properties::configLineLength = 100;

Properties::Properties() {
    hasLoaded = false;
    if (loadProperties())
        hasLoaded = true;
}

Properties::~Properties() {
    kv.clear();
}

string Properties::getPropertiesByName(string key) {
    if (!hasLoaded) return nullptr;
    auto iter = kv.find(key);
    if (iter != kv.end())
        return iter->second;
    cout << "未找到属性 " << key << " 对应的配置；" << endl;
    return nullptr;
}

bool Properties::refreshProperties() {
    return loadProperties();
}

bool Properties::hasLoadedProperties() {
    return hasLoaded;
}

bool Properties::loadProperties() {
    if (!kv.empty()) kv.clear();
    ifstream inputFile;
    inputFile.open(configFilePath, ios::in);
    char *tmp = new char[configLineLength];
    while(inputFile.getline(tmp, configLineLength)) {
        pair<string, string> ss = parseLine(tmp);
        kv.insert(ss);
    }
    delete[] tmp;
    inputFile.close();
    return true;
}

pair<string, string> Properties::parseLine(char *line) {
    char *str = line;
    while(*str == ' ') str++;
    pair<string, string> ss;
    ss.first = "NONE"; ss.second = "NONE";
    if (*str == '#') return ss;
    char *tmp = new char[configLineLength];
    int i = 0;
    while (*str != '=') {
        if (*str != ' ')
            tmp[i++] = *str;
        str++;
    }
    if (i >= configLineLength - 1) {
        cerr << "错误的配置格式: " << line << endl;
        return ss;
    }
    tmp[i] = '\0';
    ss.first = tmp;
    i = 0;
    str++;
    while (*str != '\0') {
        if (*str != ' ')
            tmp[i++] = *str;
        str++;
    }
    tmp[i] = '\0';
    ss.second = tmp;
    delete []tmp;
    return ss;
}

//int main()
//{
//    Properties *properties = new Properties();
//    cout << properties->getPropertiesByName("work_directory") << endl;
//    cout << properties->getPropertiesByName("upload_enable") << endl;
//    cout << properties->getPropertiesByName("listen_port") << endl;
//    cout << properties->getPropertiesByName("bind_ip") << endl;
//    cout << properties->getPropertiesByName("listen_queue_length") << endl;
//    return 0;
//}