#pragma once
#ifndef IntermediateCode_H
#define IntermediateCode_H

#include <cstdio>
#include <vector>
#include <string>
#include <map>

#include "SymbolTable.h"

#define INLINE_PARA_CNT_LIMIT (4)

using std::string;
using std::vector;
using std::map;

const string func_RetVar = "#Ret_Var_v0";

enum IMOper {
    opAdd, opSub, opMul, opDiv, opAssign, opLoadArray, opStore2Array,
    opSetLabel, opPush, opCall, opLss, opLeq, opGre, opGeq, opEql, opNeq,
    opGoto, opBNZ, opBZ, opScanf, opPrintf, opReturn, opGetRetVal,
    opFunc, opVar, opPara, opConst,
    opBeq, opBne, opBlt, opBle, opBgt, opBge,
    opMod,
};

class FourItemExpr {
private:
    static int id;
    int exprId;
public:
    IMOper op;
    string result;
    string first;
    string second;

    FourItemExpr() {
        op = opAdd;
        result = "";
        first = "";
        second = "";
        exprId = id++;
    }

    FourItemExpr(IMOper imop, string result, string first = "", string second = "") {
        this->op = imop;
        this->result = result;
        this->first = first;
        this->second = second;
        this->exprId = id++;
    }

    string toString();

    bool operator < (const FourItemExpr& temp) const {
        if (exprId != temp.exprId) {
            return exprId < temp.exprId;
        } else if (op != temp.op) {
            return op < temp.op;
        } else if (result != temp.result) {
            return result < temp.result;
        } else if (first != temp.first) {
            return first < temp.first;
        } else {
            return second < temp.second;
        }
    }

    bool operator == (const FourItemExpr& temp) const {
        if (exprId != temp.exprId) {
            return false;
        } else if (op != temp.op) {
            return false;
        } else if (result != temp.result) {
            return false;
        } else if (first != temp.first) {
            return false;
        } else {
            return false;
        }
        return true;
    }
};

class IntermediateCode {
private:
    static unsigned int tempVarMark;

    static void removeRedundantJump();

    static void smallAdjust();

    static void toFunctionBlock();

    static void toTotalList();

    static void removeUseless();
public:
    static map<string, string> str2name;
    static vector<FourItemExpr> imExprList;
    static map< string, vector<FourItemExpr> > func2im;
    static bool skip;

    static string getTempVar();

    static void releaseTempVar();

    static string getNewLabel();

    static void setLabel(string label);

    static void addString(string str);

    static void addFunctionDef(FunctionItem& item);

    static void addVariableDef(VariableItem& item);

    static void addConstantDef(ConstantItem& item);

    static void addParameter(vector<VariableItem*>& parameters);

    static void scanfFunction(Item& item);

    static void printfFunction(string str, string result, string type);

    static void functionReturn(string result);

    static void callFunction(string funcName);

    static void pushParameter(string result, string funcName);
    
    static void getReturnValue(string result);

    static void add4ItemExpr(IMOper imop, string result, string first = "", string second = "");

    static void dumpCode2File(FILE* file);

    static void optimization();
};

#endif // !IntermediateCode_H