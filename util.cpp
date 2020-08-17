#include <cstring>
#include <cctype>
#include "util.h"

bool isNumberValue(string& var) {
    if (var.length() == 0) {
        return false;
    }
    unsigned int i = 0;
    if (var[0] == '+' || var[0] == '-') {
        i = 1;
    }
    for (; i < var.length(); i++) {
        if (!isdigit(var[i])) {
            return false;
        }
    }
    return true;
}

bool isTempVariable(string var) {
    return (var.length() > 1 && var[0] == '#');
}

bool isStringName(string& stringName) {
    int len = strlen("STRING_");
    return (int)stringName.length() > len && stringName.substr(0, len) == "STRING_";
}

string sym2str(Sym sym) {
    const static string symStringArray[] = {
        "IDENFR", "INTCON", "CHARCON", "STRCON", "CONSTTK", "INTTK", "CHARTK", "VOIDTK", "MAINTK",
        "IFTK", "ELSETK", "DOTK", "WHILETK", "FORTK", "SCANFTK", "PRINTFTK", "RETURNTK", "PLUS",
        "MINU", "MULT", "DIV", "LSS", "LEQ", "GRE", "GEQ", "EQL", "NEQ", "ASSIGN", "SEMICN", "COMMA",
        "LPARENT", "RPARENT", "LBRACK", "RBRACK", "LBRACE", "RBRACE"
    };
    return (symStringArray[(int)sym]);
}

IMOper str2imop(string str) {
    const static map<string, IMOper> string2op = {
        {"+", opAdd}, {"-", opSub}, {"*", opMul}, {"/", opDiv},
        {"=", opAssign}, {"[]", opLoadArray}, {"[]=", opStore2Array},
        {"setLabel", opSetLabel}, {"push", opPush}, {"call", opCall},
        {"<", opLss}, {"<=", opLeq}, {">", opGre}, {">=", opGeq}, {"==", opEql}, {"!=", opNeq},
        {"goto", opGoto}, {"BNZ", opBNZ}, {"BZ", opBZ},
        {"scanf", opScanf}, {"printf", opPrintf}, {"return", opReturn}, {"retValue", opGetRetVal},
        {"function", opFunc} , {"var", opVar}, {"para", opPara}, {"const", opConst}
    };
    return string2op.at(str);
}