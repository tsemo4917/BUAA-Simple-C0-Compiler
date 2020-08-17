#pragma once
#ifndef CreateASM_H
#define CreateASM_H
#include <cstdio>
#include <vector>
#include <string>
#include <set>
#include <map>
#include "IntermediateCode.h"
#include "TargetCode.h"

using std::string;
using std::vector;
using std::set;
using std::map;

class functionInfo {
public:
    int functionOffset;
    map<string, int> var2offset;
    map<string, mipsReg> var2reg;
    set<mipsReg> usedRegs;
    bool isLeaf;
};

class CreateASM {
private:
    static vector<string> dataCode;

    static vector<TargetCode> mipsCode;

    static bool save;
    static map<string, functionInfo> funcInfoTable;
    
    static void saveInfo(string funcName, bool isLeaf);

    static void loadInfo(string funcName);

    static void createFunctionInfo(string funcName);

    static string getPosition(string var);

    static void increaseCount(map<string, int>& var2count, string name);

    static void allocateRegisterStack(map<string, int>& var2count, FunctionItem& item);

    static void createStringGlobalVar();

    static bool isRegVar(string var);

    static mipsReg calArrayAddrReg(mipsReg addrReg, string index, Sym type, mipsReg base);

    static void syscallPrintfValue(string str, string type);

    static void syscallPrintfString(string& strName);

    static void functionReturn(string funcName);

    static void storeReg(mipsReg reg, string var);
    
    static void loadReg(mipsReg reg, string var);

    static void loadImmediate(mipsReg reg, int imm);

    static void loadRegImm(mipsReg& reg, string var);

    static void moveReg(mipsReg dst, mipsReg src);

    static void handleImExpr(FourItemExpr& expr);

    static void handleFunction(string funcName);

    static void handleLoadArray(FourItemExpr expr, Sym type);

    static void handleStore2Array(FourItemExpr expr, Sym type);

    static void saveAllRegs(string callFunction);

    static void loadAllRegs(string callFunction);

    static void R_instr(mipsInstr instr, mipsReg dst, mipsReg r1, mipsReg r2);

    static void I_instr(mipsInstr instr, mipsReg dst, mipsReg r1, int imm);

    static void addMipsCode(mipsInstr instr, string result = "", string var1 = "", string var2 = "");

public:
    static void optimization();

    static void generateASM();

    static void dump2file(FILE* file);
};

#endif // !CreateASM_H