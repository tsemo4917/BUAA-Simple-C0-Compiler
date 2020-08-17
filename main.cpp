#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include "lexicalAnalysis.h"
#include "syntaxAnalysis.h"
#include "OutputHandler.h"
#include "ErrorHandler.h"
#include "SymbolTable.h"
#include "IntermediateCode.h"
#include "CreateASM.h"

char* content;

OutputHandler* outputHandler;
ErrorHandler* errorHandler;

void inputHandler(FILE* file) {
    assert(file != NULL);
    fseek(file, SEEK_SET, SEEK_END);
    unsigned int fileSize = ftell(file) + 1;
    rewind(file);
    content = new char[fileSize](); // use () to initialize the memory
    fread(content, 1, fileSize, file);
}

int main() {
    const char inputFileName[] = "testfile.txt";
    const char outputFileName[] = "output.txt";
    const char errorFileName[] = "error.txt";
    const char mipsFileName[] = "mips.txt";

    FILE* inputFile = fopen(inputFileName, "r");
    if (inputFile == NULL) {
        printf("%s is NOT Found!\n", inputFileName);
        exit(1);
    }

    FILE* outputFile = fopen(outputFileName, "w");
    assert(outputFile != NULL);

    FILE* errorFile = fopen(errorFileName, "w");
    assert(errorFile != NULL);

    FILE* mipsFile = fopen(mipsFileName, "w");
    assert(mipsFile != NULL);

    inputHandler(inputFile);

    outputHandler = new OutputHandler(outputFile);

    errorHandler = new ErrorHandler(errorFile);

    bool res;
    do {
        res = nextSym();
    } while (res);

    getsym();
    isPrograme();


    if (errorHandler->isErrorOccured()) {
        printf("Something wrong with your code.\n");
        errorHandler->error2File();
        errorHandler->error2Console();
        return 0;
    }


    FILE* beforeOptCodeFile = fopen(beforeOpt, "w");
    assert(beforeOptCodeFile != NULL);

    FILE* afterOptCodeFile = fopen(afterOpt, "w");
    assert(afterOptCodeFile != NULL);

    IntermediateCode::optimization();

    CreateASM::generateASM();
    CreateASM::optimization();
    CreateASM::dump2file(mipsFile);

    fclose(inputFile);
    fclose(outputFile);
    fclose(errorFile);
    fclose(mipsFile);

    delete outputHandler;
    delete errorHandler;
    delete[] content;

    return 0;
}