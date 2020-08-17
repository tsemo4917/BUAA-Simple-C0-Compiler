#include <sstream>
#include <iostream>
#include <map>
#include <algorithm>
#include <utility>
#include <set>
#include "SymbolTable.h"
#include "lexicalAnalysis.h"
#include "IntermediateCode.h"
#include "CreateASM.h"
#include "util.h"

using std::map;
using std::to_string;
using std::stoi;
using std::pair;
using std::cout;
using std::endl;

const mipsReg MAX_REG = reg_t8;
const mipsReg tempReg1 = reg_t9;
const mipsReg tempReg2 = reg_v1;

const mipsReg FIRST_PARA_REG = reg_a1;
const int PARA_REG_CNT = 3;
const string MAIN_FUNC_NAME = "main";
extern SymbolTable symbolTable;

vector<string> CreateASM::dataCode;
vector<TargetCode> CreateASM::mipsCode;
bool CreateASM::save = false;
map<string, functionInfo> CreateASM::funcInfoTable;

static map<string, int> globalVar2offset;

static map<string, mipsReg> var2reg;
static map<string, int> var2offset;
static int functionOffset = 0;
static set<mipsReg> usedRegSet;

static mipsReg getReg(string name) {
    mipsReg reg = var2reg.at(name);
    //if (!isTempVariable(name)) {
    //    usedRegSet.insert(reg);
    //}
    if (reg >= FIRST_PARA_REG) {
        usedRegSet.insert(reg);
    }
    return reg;
}

void CreateASM::loadInfo(string funcName) {
    var2reg = funcInfoTable[funcName].var2reg;
    var2offset = funcInfoTable[funcName].var2offset;
    functionOffset = funcInfoTable[funcName].functionOffset;
    //usedRegSet = funcInfoTable[funcName].usedRegs;
    usedRegSet.clear();
    for (auto reg : funcInfoTable[funcName].usedRegs) {
        if (FIRST_PARA_REG <= reg && reg <= reg_a3) {
            usedRegSet.insert(reg);
        }
    }
}

void CreateASM::saveInfo(string funcName, bool isLeaf) {
    funcInfoTable[funcName].var2reg = var2reg;
    funcInfoTable[funcName].var2offset = var2offset;
    funcInfoTable[funcName].usedRegs = usedRegSet;
    funcInfoTable[funcName].functionOffset = functionOffset;
    funcInfoTable[funcName].isLeaf = isLeaf;
    var2reg.clear();
    var2offset.clear();
    usedRegSet.clear();
    functionOffset = 0;
}

string CreateASM::getPosition(string var) {
    if (var2offset.find(var) != var2offset.end()) {
        return to_string(functionOffset- var2offset[var]);
    } else {
        return var;
    }
}

void CreateASM::increaseCount(map<string, int>& var2count, string name) {
    if (name.length() == 0) {
        return;
    }
    if (isTempVariable(name)) {
        if (name != func_RetVar) {
            var2count[name]++;
        }
    } else {
        if (symbolTable.isLocalVariable(name)) {
            Item* item = symbolTable.getItem(name);
            if (item->getKind() == VARIABLE) {
                VariableItem* varItem = (VariableItem*)item;
                if (varItem->getArrayLength() == 0
                    && var2reg.find(name) == var2reg.end()) {
                    var2count[name]++;
                }
            }
        }
    }
}

void CreateASM::allocateRegisterStack(map<string, int>& var2count, FunctionItem& item) {
    vector< pair<string, int> > list;
    set<string> paraSet;
    auto paraList = item.parameters;
    for (auto i = paraList.begin(); i < paraList.end(); i++) {
        paraSet.insert((*i)->getName());
    }
    for (auto i = var2count.begin(); i != var2count.end(); i++) {
        list.push_back(make_pair(i->first, i->second));
    }
    std::sort(list.begin(), list.end(), 
        [](const pair<string, int>& a, const pair<string, int>& b)->bool {
            return a.second > b.second; 
        }
    );
#ifdef OPT_ALLOCATE_REG
    // allocate register
    mipsReg reg = reg_t0;
    for (auto i = list.begin(); i < list.end() && reg <= MAX_REG; i++) {
        string varName = i->first;
        if (paraSet.find(varName) != paraSet.end()) {
            continue;
        }
        var2reg[varName] = reg;
        usedRegSet.insert(reg);
        reg = (mipsReg)(reg + 1);
    }
#endif // ALLOCATE_REG
    // allocate stack space
    // for other parameters
    for (auto i = paraList.begin(); i < paraList.end(); i++) {
        VariableItem& item = **i;
        if (var2reg.find(item.getName()) == var2reg.end()) {
            functionOffset += 4;
            var2offset[item.getName()] = functionOffset;
        }
    }
    for (auto i = list.begin(); i < list.end(); i++) {
        string varName = i->first;
        if (paraSet.find(varName) != paraSet.end()) {
            continue;
        }
        if (var2reg.find(varName) == var2reg.end()) {
            functionOffset += 4;
            var2offset[varName] = functionOffset;
        }
    }
    // for local array
    for (auto i = item.varTable.begin(); i != item.varTable.end(); i++) {
        VariableItem& varItem = *(i->second);
        if (varItem.getArrayLength() > 0) {
            if (varItem.getType() == INTTK) {
                functionOffset += varItem.getArrayLength() * 4;
            } else {
                functionOffset += (varItem.getArrayLength() / 4 + 1) * 4;
            }
            var2offset[varItem.getName()] = functionOffset;
        }
    }
}

void CreateASM::createFunctionInfo(string funcName) {
    bool isLeaf = true;

    symbolTable.setNowFunctionItem(funcName);
    FunctionItem& item = *(FunctionItem*)symbolTable.getItem(funcName);

    vector<FourItemExpr>& funcImList = IntermediateCode::func2im[funcName];
    // for return value
    if (item.getType() != VOIDTK) {
        var2reg[func_RetVar] = reg_v0;
    }

    // allocate function parameters to [reg_a1, reg_a3]
    vector<VariableItem*>& para = item.parameters;
    for (int i = 0; i < PARA_REG_CNT && i < (int)para.size(); i++) {
        var2reg[para[i]->getName()] = (mipsReg)(FIRST_PARA_REG + i);
        usedRegSet.insert((mipsReg)(FIRST_PARA_REG + i));
    }
    map<string, int> var2count;
    for (auto i = funcImList.begin(); i != funcImList.end(); i++) {
        if (i->op == opPara || i->op == opVar || i->op == opConst) {
            continue;
        }
        if (i->op == opPush || i->op == opCall) {
            isLeaf = false;
        }
        increaseCount(var2count, i->result);
        increaseCount(var2count, i->first);
        increaseCount(var2count, i->second);
    }
    allocateRegisterStack(var2count, item);
    if (!isLeaf && funcName != MAIN_FUNC_NAME) {
        functionOffset += 4;
    }
#ifdef DEBUG
    cout << funcName << endl;
    for (auto i = var2reg.begin(); i != var2reg.end(); i++) {
        cout << "\t" << i->first << "\t" << reg2str(i->second) << endl;
    }
    cout << functionOffset << endl;
    for (auto i = var2offset.begin(); i != var2offset.end(); i++) {
        cout << "\t" << i->first << "\t" << functionOffset - i->second << endl;
    }
#endif // DEBUG
    saveInfo(funcName, isLeaf);
}

void CreateASM::handleFunction(string funcName) {
    symbolTable.setNowFunctionItem(funcName);
    loadInfo(funcName);
    vector<FourItemExpr>& funcImList = IntermediateCode::func2im[funcName];
    for (auto i = funcImList.begin(); i != funcImList.end(); i++) {
#ifdef DEBUG
        addMipsCode(instr_note, i->toString());
#endif // DEBUG
        handleImExpr(*i);
    }
}

mipsReg CreateASM::calArrayAddrReg(mipsReg addrReg, string index, Sym type, mipsReg base) {
    mipsReg indexReg = addrReg;
    if (type == CHARTK) {
        if (isRegVar(index)) {
            indexReg = getReg(index);
        } else {
            loadReg(indexReg, index);
        }
    } else {
        if (isRegVar(index)) {
            I_instr(instr_sll, indexReg, getReg(index), 2);
        } else {
            loadReg(indexReg, index);
            I_instr(instr_sll, indexReg, indexReg, 2);
        }
    }
    R_instr(instr_addu, addrReg, indexReg, base);
    return addrReg;
}

void CreateASM::handleLoadArray(FourItemExpr expr, Sym type) {
    string arrayName = expr.first;
    string index = expr.second;
    mipsReg resReg = isRegVar(expr.result) ? getReg(expr.result) : tempReg1;
    mipsReg addrReg = tempReg2;
    mipsInstr instr = (type == INTTK ? instr_lw : instr_lb);
    string addr = getPosition(arrayName);
    int offset;
    mipsReg baseReg;
    if (isNumberValue(addr)) {
        // local array
        offset = stoi(addr);
        baseReg = reg_fp;
    } else {
        // global array
        offset = globalVar2offset[arrayName];
        baseReg = reg_gp;
    }
    if (isNumberValue(index)) {
        offset += (type == INTTK ? 4 * stoi(index) : stoi(index));
        addrReg = baseReg;
    } else {
        addrReg = calArrayAddrReg(addrReg, index, type, baseReg);
    }
    addMipsCode(instr, reg2str(resReg), to_string(offset), reg2str(addrReg));
    if (!isRegVar(expr.result)) {
        storeReg(resReg, expr.result);
    }
}

void CreateASM::handleStore2Array(FourItemExpr expr, Sym type) {
    string arrayName = expr.first;
    string index = expr.second;
    mipsReg resReg = tempReg1, addrReg = tempReg2;
    string addr = getPosition(arrayName);
    mipsInstr instr = type == INTTK ? instr_sw : instr_sb;
    loadRegImm(resReg, expr.result);
    int offset;
    mipsReg baseReg;
    if (isNumberValue(addr)) {
        // local array
        offset = stoi(addr);
        baseReg = reg_fp;
    } else {
        // global array
        offset = globalVar2offset[arrayName];
        baseReg = reg_gp;
    }
    if (isNumberValue(index)) {
        offset += (type == INTTK ? 4 * stoi(index) : stoi(index));
        addrReg = baseReg;
    } else {
        addrReg = calArrayAddrReg(addrReg, index, type, baseReg);
    }
    addMipsCode(instr, reg2str(resReg), to_string(offset), reg2str(addrReg));
}

static int fastLog2(int x) {
    unsigned long ix, exp;
    float fx = (float)x;
    ix = *(unsigned long*)& fx;
    exp = (ix >> 23) & 0xFF;
    return exp - 127;
}

#define paraInitOffset ((int)((PARA_REG_CNT - 1) * 4))

void CreateASM::handleImExpr(FourItemExpr& expr) {
    static string funcName = "";
    static int parameterOffset = paraInitOffset;

    IMOper op = expr.op;
    string result = expr.result;
    string var1 = expr.first;
    string var2 = expr.second;
    mipsReg resReg;
    if (isRegVar(expr.result)) {
        resReg = getReg(expr.result);
    } else {
        resReg = tempReg1;
    }
    mipsReg t1 = tempReg1, t2 = tempReg2;
    switch (op) {
    case opAdd:
        loadRegImm(t1, var1);
        if (isNumberValue(var2)) {
            I_instr(instr_addiu, resReg, t1, stoi(var2));
        } else {
            loadRegImm(t2, var2);
            R_instr(instr_addu, resReg, t1, t2);
        }
        if (!isRegVar(result)) {
            storeReg(resReg, result);
        }
        break;
    case opSub:
        if (isNumberValue(var1) && stoi(var1) == 0) {
            t1 = reg_0;
        } else {
            loadRegImm(t1, var1);
        }
        loadRegImm(t2, var2);
        R_instr(instr_subu, resReg, t1, t2);
        if (!isRegVar(result)) {
            storeReg(resReg, result);
        }
        break;
    case opMul:
        loadRegImm(t1, var1);
        if (isNumberValue(var2)) {
            int value = stoi(var2);
            if (value > 1 && ((value & (value - 1)) == 0)) {
                I_instr(instr_sll, resReg, t1, fastLog2(value));
            } else {
                I_instr(instr_mul, resReg, t1, value);
            }
        } else {
            loadRegImm(t2, var2);
            R_instr(instr_mul, resReg, t1, t2);
        }
        if (!isRegVar(result)) {
            storeReg(resReg, result);
        }
        break;
    case opDiv:
        loadRegImm(t1, var1);
        if (isNumberValue(var2) && stoi(var2) > 1 && ((stoi(var2) & (stoi(var2) - 1)) == 0)) {
            I_instr(instr_sra, resReg, t1, fastLog2(stoi(var2)));
        } else {
            loadRegImm(t2, var2);
            addMipsCode(instr_div, "", reg2str(t1), reg2str(t2));
            addMipsCode(instr_mflo, reg2str(resReg));
        }
        if (!isRegVar(result)) {
            storeReg(resReg, result);
        }
        break;
    case opMod:
        loadRegImm(t1, var1);
        loadRegImm(t2, var2);
        addMipsCode(instr_div, "", reg2str(t1), reg2str(t2));
        addMipsCode(instr_mfhi, reg2str(resReg));
        if (!isRegVar(result)) {
            storeReg(resReg, result);
        }
        break;
    case opAssign:
        if (!isRegVar(var1)) {
            loadRegImm(resReg, var1);
        } else {
            moveReg(resReg, getReg(var1));
        }
        if (!isRegVar(result)) {
            storeReg(resReg, result);
        }
        break;
    case opLoadArray:
        handleLoadArray(expr, symbolTable.getType(var1));
        break;
    case opStore2Array:
        handleStore2Array(expr, symbolTable.getType(var1));
        break;
    case opPush: {
        saveAllRegs(var1);
        if (parameterOffset < 0) {
            if (!isRegVar(result)) {
                loadRegImm(resReg, result);
            }
            storeReg(resReg, to_string(parameterOffset));
        } else {
            mipsReg paraReg = (mipsReg)(FIRST_PARA_REG + (paraInitOffset - parameterOffset) / 4);
            if (!isRegVar(result)) {
                loadRegImm(paraReg, result);
            } else {
                moveReg(paraReg, resReg);
            }
        }
        parameterOffset -= 4;
        break;
    }
    case opCall: {
        parameterOffset = paraInitOffset;
        saveAllRegs(result);
        addMipsCode(instr_jal, result);
        loadAllRegs(result);
        break;
    }
    case opLss:
        if (isNumberValue(var1) && stoi(var1) == 0) {
            t1 = reg_0;
        } else {
            loadRegImm(t1, var1);
        }
        if (isNumberValue(var2)) {
            I_instr(instr_slti, resReg, t1, stoi(var2));
        } else {
            loadRegImm(t2, var2);
            R_instr(instr_slt, resReg, t1, t2);
        }
        if (!isRegVar(expr.result)) {
            storeReg(resReg, expr.result);
        }
        break;
    case opLeq: case opGre: case opGeq: case opEql: case opNeq: {
        mipsInstr instruction;
        if (op == opLeq) {
            instruction = instr_sle;
        } else if (op == opGre) {
            instruction = instr_sgt;
        } else if (op == opGeq) {
            instruction = instr_sge;
        } else if (op == opEql) {
            instruction = instr_seq;
        } else {
            instruction = instr_sne;
        }
        if (isNumberValue(var1) && stoi(var1) == 0) {
            t1 = reg_0;
        } else {
            loadRegImm(t1, var1);
        }
        if (isNumberValue(var2)) {
            if (stoi(var2) == 0) {
                R_instr(instruction, resReg, t1, reg_0);
            } else {
                I_instr(instruction, resReg, t1, stoi(var2));
            }
        } else {
            loadRegImm(t2, var2);
            R_instr(instruction, resReg, t1, t2);
        }
        if (!isRegVar(expr.result)) {
            storeReg(resReg, expr.result);
        }
        break;
    }
    case opBNZ: case opBZ: 
        loadRegImm(t1, var1);
        if (op == opBNZ) {
            addMipsCode(instr_bne, result, reg2str(t1), reg2str(reg_0));
        } else {
            addMipsCode(instr_beq, result, reg2str(t1), reg2str(reg_0));
        }
        break;
    case opScanf: 
        if (var1 == "int") {
            loadImmediate(reg_v0, 5);
        } else {
            loadImmediate(reg_v0, 12);
        }
        addMipsCode(instr_syscall);
        if (isRegVar(result)) {
            moveReg(resReg, reg_v0);
        } else {
            storeReg(reg_v0, result);
        }
        break;
    case opPrintf:
        if (isStringName(result)) {
            syscallPrintfString(result);
        } else {
            var2 = var1;
            var1 = result;
        }
        if (var1.length() > 0) {
            syscallPrintfValue(var1, var2);
        }
        syscallPrintfValue(to_string(((int)'\n')), "char");
        break;
    case opReturn: {
        //mipsReg retValReg = reg_v0;
        //if (result.length() > 0) {
        //    if (isRegVar(result)) {
        //        moveReg(retValReg, resReg);
        //    } else {
        //        loadRegImm(retValReg, result);
        //    }
        //}
        functionReturn(funcName);
        break;
    }
    case opGetRetVal:
        if (!isRegVar(result)) {
            storeReg(reg_v0, result);
        } else {
            moveReg(resReg, reg_v0);
            //usedRegSet.insert(resReg);
        }
        break;
    case opGoto:
        addMipsCode(instr_j, result);
        break;
    case opSetLabel:
        addMipsCode(instr_label, result);
        break;
    case opFunc:
        funcName = var1;
        if (functionOffset > 0) {
            I_instr(instr_addiu, reg_sp, reg_sp, -functionOffset);
            moveReg(reg_fp, reg_sp);
        }
        if (!funcInfoTable[funcName].isLeaf && funcName != MAIN_FUNC_NAME) {
            storeReg(reg_ra, to_string(0));
        }
        break;
    case opBeq: case opBne: case opBlt: case opBle: case opBgt: case opBge:{
        mipsInstr instr;
        if (op == opBeq) {
            instr = instr_beq;
        } else if (op == opBne) {
            instr = instr_bne;
        } else if (op == opBlt) {
            instr = instr_blt;
        } else if (op == opBle) {
            instr = instr_ble;
        } else if (op == opBgt) {
            instr = instr_bgt;
        } else { // if (op == opBge)
            instr = instr_bge;
        }
        if (isNumberValue(var1) && stoi(var1) == 0) {
            t1 = reg_0;
        } else {
            loadRegImm(t1, var1);
        }
        if (isNumberValue(var2)) {
            if (stoi(var2) == 0) {
                addMipsCode(instr, result, reg2str(t1), reg2str(reg_0));
            } else {
                addMipsCode(instr, result, reg2str(t1), var2);
            }
        } else {
            loadRegImm(t2, var2);
            addMipsCode(instr, result, reg2str(t1), reg2str(t2));
        }
        break;
    }
    case opVar:
        break;
    case opPara:
        break;
    case opConst:
        break;
    default:
        break;
    }
}

void CreateASM::generateASM() {
    map< string, vector<FourItemExpr> >& func2imcode = IntermediateCode::func2im;
    // record the information (used regs, stack offset, etc.) to funcInfoTable
    for (auto i = func2imcode.begin(); i != func2imcode.end(); i++) {
        createFunctionInfo(i->first);
    }
    // translate intermediate code to assembly
    createStringGlobalVar();

    string funcName = MAIN_FUNC_NAME;
    addMipsCode(instr_label, funcName);
    handleFunction(funcName);
    for (auto i = func2imcode.begin(); i != func2imcode.end(); i++) {
        funcName = i->first;
        if (funcName == MAIN_FUNC_NAME) {
            continue;
        }
        addMipsCode(instr_label, funcName);
        handleFunction(funcName);
    }
}

void CreateASM::createStringGlobalVar() {
    dataCode.push_back(".data");
    std::ostringstream ostr;
    map<string, string>& str2name = IntermediateCode::str2name;
    for (auto i = str2name.begin(); i != str2name.end(); i++) {
        //dataCode.push_back(".align 2");
        string str = i->first;
        ostr << '\t' << i->second << ":\t.asciiz\t";
        string temp = "";
        for (auto j = str.begin(); j != str.end(); j++) {
            temp += *j;
            if (*j == '\\') {
                temp += '\\';
            }
        }
        ostr << '"' << temp << '"';
        dataCode.push_back(ostr.str());
        ostr.str("");
    }
    VariableTable& varTable = symbolTable.getVarTable();
    int gpOffset = 0;
    for (auto i = varTable.begin(); i != varTable.end(); i++) {
        string name = i->first;
        VariableItem& item = *i->second;
        if (item.getArrayLength() > 0) {
            if (item.getType() == INTTK) {
                gpOffset -= item.getArrayLength() * 4;
            } else {
                gpOffset -= (item.getArrayLength() / 4 + 1) * 4;
            }
        } else {
            gpOffset -= 4;
        }
        globalVar2offset[name] = gpOffset;
#ifdef DEBUG
        cout << "Global:\t" << name << "\t" << gpOffset << endl;
#endif // DEBUG
    }
    dataCode.push_back(".text");
}

void CreateASM::dump2file(FILE* file) {
    for (auto i = dataCode.begin(); i != dataCode.end(); i++) {
        fprintf(file, "%s\n", i->c_str());
    }
    for (auto i = mipsCode.begin(); i != mipsCode.end(); i++) {
        fprintf(file, "%s\n", i->toString().c_str());
    }
}

void CreateASM::R_instr(mipsInstr instr, mipsReg dst, mipsReg r1, mipsReg r2) {
    addMipsCode(instr, reg2str(dst), reg2str(r1), reg2str(r2));
}

void CreateASM::I_instr(mipsInstr instr, mipsReg dst, mipsReg r1, int imm) {
    addMipsCode(instr, reg2str(dst), reg2str(r1), to_string(imm));
}

void CreateASM::syscallPrintfValue(string var, string type) {
    mipsReg reg = reg_a0;
    if (isNumberValue(var)) {
        loadImmediate(reg, stoi(var));
    } else {
        if (isRegVar(var)) {
            moveReg(reg_a0, getReg(var));
        } else {
            loadReg(reg_a0, var);
        }
    }
    if (type == "int") {
        loadImmediate(reg_v0, 1);
    } else {
        loadImmediate(reg_v0, 11);
    }
    addMipsCode(instr_syscall);
}

void CreateASM::syscallPrintfString(string& strName) {
    addMipsCode(instr_la, reg2str(reg_a0), strName);
    loadImmediate(reg_v0, 4);
    addMipsCode(instr_syscall);
}

void CreateASM::functionReturn(string funcName) {
    if (funcName == MAIN_FUNC_NAME) {
        loadImmediate(reg_v0, 10);
        addMipsCode(instr_syscall);
    } else {
        if (functionOffset > 0) {
            I_instr(instr_addiu, reg_sp, reg_sp, functionOffset);
        }
        if (!funcInfoTable[funcName].isLeaf) {
            loadReg(reg_ra, to_string(0));
        }
        addMipsCode(instr_jr, reg2str(reg_ra));
    }
}

void CreateASM::loadReg(mipsReg reg, string var) {
    string offset = getPosition(var);
    mipsReg startReg;
    if (isNumberValue(offset)) {
        if (stoi(offset) < 0) {
            startReg = reg_sp;
        } else {
            startReg = reg_fp;
        }
    } else {
        startReg = reg_gp;
        offset = to_string(globalVar2offset[var]);
    }
    addMipsCode(instr_lw, reg2str(reg), offset, reg2str(startReg));
}

void CreateASM::loadImmediate(mipsReg reg, int imm) {
    addMipsCode(instr_li, reg2str(reg), to_string(imm));
}

void CreateASM::storeReg(mipsReg reg, string var) {
    string offset = getPosition(var);
    mipsReg startReg;
    if (isNumberValue(offset)) {
        if (stoi(offset) < 0) {
            startReg = reg_sp;
        } else {
            startReg = reg_fp;
        }
    } else {
        startReg = reg_gp;
        offset = to_string(globalVar2offset[var]);
    }
    addMipsCode(instr_sw, reg2str(reg), offset, reg2str(startReg));
}

void CreateASM::loadRegImm(mipsReg& reg, string var) {
    if (isNumberValue(var)) {
        loadImmediate(reg, stoi(var));
    } else {
        if (isRegVar(var)) {
            reg = getReg(var);
        } else {
            loadReg(reg, var);
        }
    }
}

void CreateASM::moveReg(mipsReg dst, mipsReg src) {
    addMipsCode(instr_move, reg2str(dst), reg2str(src));
}

void CreateASM::saveAllRegs(string callFunction) {
    if (!save) {
        vector<mipsReg> saveRegs;
#ifdef OPT_LEAF_FUNC
        if (funcInfoTable[callFunction].isLeaf) {
            auto& callFunctionUsedRegs = funcInfoTable[callFunction].usedRegs;
            std::set_intersection(
                usedRegSet.begin(), usedRegSet.end(),
                callFunctionUsedRegs.begin(), callFunctionUsedRegs.end(),
                std::back_inserter(saveRegs)
            );
        } else {
#endif // OPT_LEAF_FUNC
            saveRegs = vector<mipsReg>(usedRegSet.begin(), usedRegSet.end());
#ifdef OPT_LEAF_FUNC
        }
#endif // OPT_LEAF_FUNC
        //for (auto i = var2reg.begin(); i != var2reg.end(); i++) {
        //    if (isTempVariable(i->first)) {
        //        continue;
        //    }
        //    auto j = std::find(saveRegs.begin(), saveRegs.end(), i->second);
        //    if (j != saveRegs.end()) {
        //        saveRegs.erase(j);
        //    }
        //}
        int offset = -4;
        for (auto i = saveRegs.begin(); i != saveRegs.end(); i++) {
            storeReg(*i, to_string(offset));
            offset -= 4;
        }
        if (saveRegs.size() > 0) {
            I_instr(instr_addiu, reg_sp, reg_sp, saveRegs.size() * -4);
        }
        save = true;
    }
}

void CreateASM::loadAllRegs(string callFunction) {
    if (save) {
        vector<mipsReg> saveRegs;
#ifdef OPT_LEAF_FUNC
        if (funcInfoTable[callFunction].isLeaf) {
            auto& callFunctionUsedRegs = funcInfoTable[callFunction].usedRegs;
            std::set_intersection(
                usedRegSet.begin(), usedRegSet.end(),
                callFunctionUsedRegs.begin(), callFunctionUsedRegs.end(),
                std::back_inserter(saveRegs)
            );
        } else {
#endif // OPT_LEAF_FUNC
            saveRegs = vector<mipsReg>(usedRegSet.begin(), usedRegSet.end());
#ifdef OPT_LEAF_FUNC
        }
#endif // OPT_LEAF_FUNC
        int offset = (int)saveRegs.size() * -4;
        if (saveRegs.size() > 0) {
            I_instr(instr_addiu, reg_sp, reg_sp, saveRegs.size() * 4);
        }
        for (auto i = saveRegs.rbegin(); i != saveRegs.rend(); i++) {
            loadReg(*i, to_string(offset));
            offset += 4;
        }
        save = false;
        if (functionOffset > 0) {
            moveReg(reg_fp, reg_sp);
        }
    }
}

bool CreateASM::isRegVar(string var) {
    return var2reg.find(var) != var2reg.end();
}

void CreateASM::addMipsCode(mipsInstr instr, string result, string var1, string var2) {
    mipsCode.push_back(TargetCode(instr, result, var1, var2));
}

void CreateASM::optimization() {
    for (auto i = mipsCode.begin() + 1; i < mipsCode.end(); i++) {
        TargetCode& prev = *(i - 1);
        TargetCode& now = *i;
        if (prev.isStoreInstr() && now.isLoadInstr()
            && prev.result == now.result && prev.var1 == now.var1 
            && prev.var2 == now.var2) {
            i = mipsCode.erase(i);
        }
    }
}