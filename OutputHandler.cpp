#include "OutputHandler.h"
#include <cstdio>
#include <string>

OutputHandler::OutputHandler(FILE* file) {
    this->outputFile = file;
}

OutputHandler::~OutputHandler() {
    //free(this->outputFile);
}

void OutputHandler::insertBuffer(int k, string str) {
    if (k > 0) {
        this->outputBuffer.insert(this->outputBuffer.begin() + k, str);
    } else {
        this->outputBuffer.insert(this->outputBuffer.end() + k, str);
    }
}

void OutputHandler::addString(string str) {
    this->outputBuffer.push_back(str);
}

void OutputHandler::backtrace() {
    this->outputBuffer.pop_back();
}

void OutputHandler::print2File() {
    vector<string>::iterator it = this->outputBuffer.begin();
    for (; it != this->outputBuffer.end(); it++) {
        fprintf(this->outputFile, "%s\n", it->c_str());
    }
}

void OutputHandler::print2Console() {
    vector<string>::iterator it = this->outputBuffer.begin();
    for (; it != this->outputBuffer.end(); it++) {
        printf("%s\n", it->c_str());
    }
}
