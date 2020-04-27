//
// Created by wang on 2020/4/21.
//

#ifndef FTP_PROPERTIES_H
#define FTP_PROPERTIES_H

#include <map>
#include <vector>

using namespace std;

typedef map<string, string> ConfigMap;

class Properties
{
public:
    ConfigMap kv;
    const static string configFilePath;
    const static int configLineLength;

public:
    Properties();
    ~Properties();
    string getPropertiesByName(string key);
    bool refreshProperties();
    bool hasLoadedProperties();

private:
    bool hasLoaded;
    bool loadProperties();
    pair<string ,string> parseLine(char str[]);
};

#endif //FTP_PROPERTIES_H
