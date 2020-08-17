#include <string>
#include <vector>
#include <cctype>
#include <cstring>
#include "lexicalAnalysis.h"
#include "OutputHandler.h"
#include "ErrorHandler.h"
#include "SymbolTable.h"
#include "util.h"

using std::string;
using std::vector;

const map<string, Sym> reserver2symbol = {
    {"const", CONSTTK}, {"int", INTTK}, {"char", CHARTK}, {"void", VOIDTK},
    {"main", MAINTK}, {"if", IFTK}, {"else", ELSETK},
    {"do", DOTK}, {"while", WHILETK}, {"for", FORTK},
    {"scanf", SCANFTK}, {"printf", PRINTFTK}, {"return", RETURNTK}
};

const map<char, Sym> char2symbol = {
    {'+', PLUS}, {'-', MINU}, {'*', MULT}, {'/', DIV},
    {'(', LPARENT}, {')', RPARENT}, {'[', LBRACK}, {']', RBRACK}, {'{', LBRACE}, {'}', RBRACE},
    {',', COMMA}, {';',SEMICN}
};

extern char* content;
extern OutputHandler* outputHandler;
extern ErrorHandler* errorHandler;

string token;
Sym symbol;
long constantValue;

static char character;
static int codeIndex = 0;

static int lineAt = 1;
static string tokenBuffer = "";
static Sym symm;
static int symbolIndex = 0;
static vector<Sym> symbolList;
static vector<string> tokenList;
static vector<int> lineList;
static vector<long> valueList;

static void getCharacter() {
    if (codeIndex >= (int)strlen(content)) {
        character = '\0';
    } else {
        character = content[codeIndex++];
        if (character == '\n') {
            lineAt++;
            errorHandler->increaseErrorLine();
        }
    }
}

static void catTokenBuffer() { tokenBuffer.push_back(character); }

static void retract() {
    if (character == '\n') {
        lineAt--;
        errorHandler->decreaseErrorLine();
    }
    codeIndex--;
    character = content[codeIndex];
}

static int isLetter() { return (isalpha(character) || character == '_'); }

static int isDigit() { return (isdigit(character)); }

static int isGreater() { return (character == '>'); }

static int isLess() { return (character == '<'); }

static int isNot() { return (character == '!'); }

static int isEqual() { return (character == '='); }

static int isPlus() { return (character == '+'); }

static int isMinus() { return (character == '-'); }

static int isDivi() { return (character == '/'); }

static int isMult() { return (character == '*'); }

static int isSingleQuotation() { return (character == '\''); }

static int isDoubleQuotation() { return (character == '\"'); }

static void dealWithCompare(Sym equ, Sym unequ) {
    getCharacter();
    if (isEqual()) {
        catTokenBuffer();
        symm = equ;
    } else {
        symm = unequ;
        retract();
    }
}

void error() {
    //outputHandler->print2Console();
    //outputHandler->print2File();
    puts("\n-----------------------------------------");
    printf("error at line %d\n", getPrevSymLine());
    printf("The programe get token:[%s]\n", token.c_str());
    printf("There is something wrong with your code!\n");
    exit(0);
}

int getPrevSymLine() {
    if (symbolIndex == 0) {
        return *(lineList.end() - 1);
    } else if (symbolIndex < 2) {
        return 1;
    } else {
        return lineList.at(symbolIndex - 2);
    }
}

static string tempBuffer = "";
void backtrace(int n) {
    for (int i = 0; i < n; i++) {
        if (symbolIndex > 0) {
            symbolIndex--;
        }
        if (i > 0) {
            outputHandler->backtrace();
        }
    }
    tempBuffer.clear();
}

bool getsym() {
    if (symbolIndex >= (int)symbolList.size()) {
        outputHandler->addString(tempBuffer);
        tempBuffer.clear();
        symbol = (Sym)-1;
        return false;
    }
    symbol = symbolList.at(symbolIndex);
    token = tokenList.at(symbolIndex);
    constantValue = valueList.at(symbolIndex);
    symbolIndex++;

    if (tempBuffer.length() > 0) {
        outputHandler->addString(tempBuffer);
    }
    tempBuffer.clear();
    tempBuffer.append(sym2str(symbol));
    tempBuffer.append(" " + token);
    return true;
}

bool nextSym() {
    tokenBuffer.clear();
    do {
        getCharacter();
    } while (isspace(character));
    long value = 0;
    if (character == '\0') {
        return false;
    }
    if (isLetter()) {
        while (isLetter() || isDigit()) {
            catTokenBuffer();
            getCharacter();
        }
        retract();
        if (reserver2symbol.find(tokenBuffer) != reserver2symbol.end()) {
            symm = reserver2symbol.at(tokenBuffer);
        } else {
            symm = IDENFR;
        }
    } else if (isDigit()) {
        while (isDigit()) {
            catTokenBuffer();
            getCharacter();
        }
        retract();
        value = std::stol(tokenBuffer);
        symm = INTCON;
    } else {
        catTokenBuffer();
        if (char2symbol.find(character) != char2symbol.end()) {
            symm = char2symbol.at(character);
        } else if (isEqual()) {
            dealWithCompare(EQL, ASSIGN);
        } else if (isGreater()) {
            dealWithCompare(GEQ, GRE);
        } else if (isLess()) {
            dealWithCompare(LEQ, LSS);
        } else if (isNot()) {
            getCharacter();
            if (isEqual()) {
                catTokenBuffer();
                symm = NEQ;
            } else {
                errorHandler->addErrorInfo(IllegalChar);
                symm = (Sym)0;
            }
            //dealWithCompare(NEQ, (Sym)0);
        } else if (isSingleQuotation()) {
            getCharacter();
            if (isLetter() || isDigit() || isPlus() || isMinus() || isMult() || isDivi()) {
                catTokenBuffer();
            } else {
                errorHandler->addErrorInfo(IllegalChar);
            }
            getCharacter();
            if (!isSingleQuotation()) {
                errorHandler->addErrorInfo(IllegalChar);
            }
            tokenBuffer.erase(0, 1);
            value = (long)tokenBuffer[0];
            symm = CHARCON;
        } else if (isDoubleQuotation()) {
            getCharacter();
            while (character == 32 || character == 33
                || (35 <= character && character <= 126)) {
                catTokenBuffer();
                getCharacter();
            }
            if (!isDoubleQuotation()) {
                //error();
                errorHandler->addErrorInfo(IllegalChar);
            }
            tokenBuffer.erase(0, 1);
            symm = STRCON;
        } else {
            //error();
            errorHandler->addErrorInfo(IllegalChar);
        }
    }
    symbolList.push_back(symm);
    tokenList.push_back(tokenBuffer);
    valueList.push_back(value);
    lineList.push_back(lineAt);
    return true;
}