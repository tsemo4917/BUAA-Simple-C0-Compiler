#include "ErrorHandler.h"
#include "lexicalAnalysis.h"
#include <algorithm>

ErrorHandler::ErrorHandler(FILE* file) {
    this->errorFile = file;
    this->errorLine = 1;
    this->prevSymbolLine = 1;
}

ErrorHandler::~ErrorHandler() {

}

void ErrorHandler::increaseErrorLine() {
    this->errorLine++;
}

void ErrorHandler::decreaseErrorLine() {
    this->errorLine--;
}

void ErrorHandler::updatePrevSymbolLine(int lineNo) {
    this->prevSymbolLine = lineNo;
}

void ErrorHandler::error2Console() {
    std::sort(infoList.begin(), infoList.end());
    for (auto i = infoList.begin(); i != infoList.end(); i++) {
        printf("%d %c\n", i->lineAt, (int)i->errorType + 'a');
    }
}

void ErrorHandler::error2File() {
    std::sort(infoList.begin(), infoList.end());
    for (auto i = infoList.begin(); i != infoList.end(); i++) {
        fprintf(this->errorFile, "%d %c\n", i->lineAt, (int)i->errorType + 'a');
    }
}

void ErrorHandler::addErrorInfo(ErrorType errorType) {
    if (errorType != OtherError) {
        this->infoList.push_back(ErrorInfo(getPrevSymLine(), errorType));
    }
}

bool ErrorHandler::isErrorOccured() {
    return !infoList.empty();
}