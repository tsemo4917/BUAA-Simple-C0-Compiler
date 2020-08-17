#pragma once
#ifndef OutputHandler_H
#define OutputHandler_H

#include <cstdio>
#include <vector>
#include <string>

using std::vector;
using std::string;

class OutputHandler
{
public:
    FILE* outputFile;
    vector<string> outputBuffer;

    OutputHandler(FILE* file);

    ~OutputHandler();
    
    void insertBuffer(int k, string str);
    void addString(string str);
    void backtrace();
    void print2File();
    void print2Console();
};


#endif // !OutputHandler_H