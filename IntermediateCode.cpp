#include <sstream>
#include <map>
#include <algorithm>
#include <climits>
#include <iostream>
#include <set>
#include "IntermediateCode.h"
#include "lexicalAnalysis.h"
#include "util.h"

using std::map;
using std::ostringstream;
using std::swap;
using std::stoi;
using std::to_string;
using std::cout;
using std::endl;
using std::set;

int FourItemExpr::id = 0;

static map<Sym, string> type2string = {
    {INTTK, "int"}, {CHARTK, "char"}, {VOIDTK, "void"}
};

vector<FourItemExpr> IntermediateCode::imExprList;
map< string, vector<FourItemExpr> > IntermediateCode::func2im;
map<string, string> IntermediateCode::str2name;
unsigned int IntermediateCode::tempVarMark = 0;
bool IntermediateCode::skip = false;

extern SymbolTable symbolTable;

void IntermediateCode::addConstantDef(ConstantItem& item) {
    if (item.isEmpty()) {
        return;
    }
    ostringstream ostr;
    Sym type = item.getType();
    if (type == CHARTK) {
        ostr << '\'' << (char)item.getValue() << '\'';
    } else {
        ostr << item.getValue();
    }
    add4ItemExpr(opConst, type2string.at(type), item.getName(), ostr.str());
}

void IntermediateCode::add4ItemExpr(IMOper imop, string result, string first, string second) {
    if (skip) {
        return;
    }
    FourItemExpr imExpr(imop, result, first, second);
    if (imExprList.size() > 0) {
        FourItemExpr lastExpr = *(imExprList.end() - 1);
        IMOper lastOp = lastExpr.op;
        if ((lastOp == opAdd || lastOp == opSub || lastOp == opMul || lastOp == opDiv || lastOp == opLoadArray)
            && (imExpr.op == opAssign && imExpr.first == lastExpr.result)
            && (isTempVariable(lastExpr.result) 
                || (imExpr.result == func_RetVar && symbolTable.isLocalVariable(lastExpr.result)))) {
            lastExpr.result = imExpr.result;
            imExpr = lastExpr;
            imExprList.pop_back();
        }
    }
    imExprList.push_back(imExpr);
}

void IntermediateCode::addFunctionDef(FunctionItem& item) {
    if (item.isEmpty()) {
        return;
    }
    symbolTable.setNowFunctionItem(item.getName());
    string type = type2string[item.getType()];
    add4ItemExpr(opFunc, type, item.getName());
}

void IntermediateCode::addVariableDef(VariableItem& item) {
    if (item.isEmpty()) {
        return;
    }
    string type = type2string[item.getType()];
    if (item.getArrayLength() > 0) {
        add4ItemExpr(opVar, type, item.getName(), "[" + std::to_string(item.getArrayLength()) + "]");
    } else {
        add4ItemExpr(opVar, type, item.getName());
    }
}

void IntermediateCode::addParameter(vector<VariableItem*>& parameters) {
    for (auto i = parameters.begin(); i != parameters.end(); i++) {
        VariableItem item = **i;
        string type = type2string[item.getType()];
        add4ItemExpr(opPara, type, item.getName());
    }
}

void IntermediateCode::scanfFunction(Item& item) {
    if (item.isEmpty()) {
        return;
    }
    string type = type2string[item.getType()];
    add4ItemExpr(opScanf, item.getName(), type);
}

void IntermediateCode::printfFunction(string str, string result, string type) {
    if (str.length() > 0 && result.length() > 0) {
        add4ItemExpr(opPrintf, str2name[str], result, type);
    } else if (str.length() > 0) {
        add4ItemExpr(opPrintf, str2name[str]);
    } else {
        add4ItemExpr(opPrintf, result, type);
    }
}

void IntermediateCode::functionReturn(string result) {
    if (result.length() > 0) {
        add4ItemExpr(opAssign, func_RetVar, result);
        add4ItemExpr(opReturn, "");
        return;
    }
    add4ItemExpr(opReturn, result);
}

void IntermediateCode::callFunction(string funcName) {
    add4ItemExpr(opCall, funcName);
}

void IntermediateCode::getReturnValue(string result) {
    add4ItemExpr(opGetRetVal, result);
}

void IntermediateCode::pushParameter(string result, string funcName) {
    add4ItemExpr(opPush, result, funcName);
}

void IntermediateCode::setLabel(string label) {
    add4ItemExpr(opSetLabel, label);
}

void IntermediateCode::addString(string str) {
    static unsigned int cnt = 0;
    if (str2name.find(str) == str2name.end()) {
        string name = string("STRING_" + std::to_string(cnt++));
        str2name[str] = name;
    }
}

void IntermediateCode::toFunctionBlock() {
    string funcName = "";
    for (auto i = imExprList.begin(); i != imExprList.end(); i++) {
        if (i->op == opFunc) {
            funcName = i->first;
            func2im[funcName].clear();
        }
        if (funcName.length() > 0) {
            func2im[funcName].push_back(*i);
        }
    }
}

void IntermediateCode::toTotalList() {
    imExprList.clear();
    for (auto i = func2im.begin(); i != func2im.end(); i++) {
        auto& func2im = i->second;
        for (auto j = func2im.begin(); j < func2im.end(); j++) {
            imExprList.push_back(*j);
        }
    }
}

void IntermediateCode::removeUseless() {
    for (auto i = func2im.begin(); i != func2im.end(); i++) {
        symbolTable.setNowFunctionItem(i->first);
        set<string> localVarUse;
        auto& list = i->second;
        for (auto j = list.begin(); j < list.end(); j++) {
            if (j->op == opFunc || j->op == opVar || j->op == opConst || j->op == opPara) {
                continue;
            }
            if (j->op == opScanf || j->op == opPush || j->op == opStore2Array) {
                localVarUse.insert(j->result);
            } else if (j->op == opPrintf) {
                if (symbolTable.isLocalVariable(j->result)) {
                    localVarUse.insert(j->result);
                }
                if (symbolTable.isLocalVariable(j->first)) {
                    localVarUse.insert(j->first);
                }
            } else {
                if (symbolTable.isLocalVariable(j->first)) {
                    localVarUse.insert(j->first);
                }
                if (symbolTable.isLocalVariable(j->second)) {
                    localVarUse.insert(j->second);
                }
            }
            //if (j->op == opDiv && j->result == "#TEMP_VAR_1" && j->first == "j" && j->second == "i") {
            //    j = list.erase(j);
            //}
        }
        for (auto j = list.begin(); j < list.end(); j++) {
            if (!(j->op == opAdd || j->op == opSub || j->op == opMul || j->op == opDiv || j->op == opMod
                || j->op == opAssign || j->op == opGetRetVal)) {
                continue;
            }
            if (symbolTable.isLocalVariable(j->result) 
                && !isTempVariable(j->result)
                && !isNumberValue(j->result)
                && localVarUse.find(j->result) == localVarUse.end()) {
                j = list.erase(j);
            }
        }
    }
}

void IntermediateCode::optimization() {
    smallAdjust();
#ifdef OPT_REDUNDANT_JUMP
    removeRedundantJump();
#endif // REDUNDANT_JUMP
    toFunctionBlock();
#ifdef OPT_INLINE
    map<string, bool> func2inline;
    for (auto i = func2im.begin(); i != func2im.end(); i++) {
        symbolTable.setNowFunctionItem(i->first);
        auto& imList = i->second;
        bool canBeInline = true;
        for (auto j = imList.begin(); j < imList.end(); j++) {
            switch (j->op) {
            case opAdd: case opSub: case opMul: case opDiv: 
            case opMod: case opAssign:
                if (!symbolTable.isLocalVariable(j->result)
                    || !symbolTable.isLocalVariable(j->first)
                    || !symbolTable.isLocalVariable(j->second)) {
                    canBeInline = false;
                }
                break;
            //case opLss: case opLeq: case opGre: case opGeq: case opEql: case opNeq:
            //case opBeq: case opBne: case opBlt: case opBle: case opBgt: case opBge:
            //case opSetLabel: case opGoto: case opBNZ: case opBZ: 
            case opPush: case opCall: case opGetRetVal:
            case opScanf: case opVar: 
            case opStore2Array: case opLoadArray:
                canBeInline = false;
                break;
            case opPrintf: 
                if (!symbolTable.isLocalVariable(j->result)) {
                    canBeInline = false;
                }
                if (j->second.length() > 0 && !symbolTable.isLocalVariable(j->first)) {
                    canBeInline = false;
                }
                break;
            case opReturn:
            case opFunc: case opPara: case opConst:
            default:
                break;
            }
        }
        if (symbolTable.getParaCount(i->first) > INLINE_PARA_CNT_LIMIT) {
            canBeInline = false;
        }
        func2inline[i->first] = canBeInline;
#ifdef DEBUG
        cout << i->first << "\t" << canBeInline << "\n";
#endif // DEBUG
    }
    const string INLINE_VAR = "INLINE_";
    for (auto i = func2im.begin(); i != func2im.end(); i++) {
        auto& imList = i->second;
        if (func2inline[i->first]) {
            continue;
        }
        for (auto j = imList.begin(); j < imList.end(); j++) {
            auto start = j;
            if (j->op == opCall || j->op == opPush) {
                string inlineFunc = (j->op == opCall ? j->result : j->first);
                if (!func2inline[inlineFunc]) {
                    continue;
                }
                map<string, string> formal2real;
                auto& formalParas = ((FunctionItem*)symbolTable.getItem(inlineFunc))->parameters;
                for (; j->op != opCall; j++) {
                    formal2real[formalParas[formal2real.size()]->getName()] = j->result;
                }
                j++; // now, j->op == opCall
                if (j->op == opGetRetVal) { // use the result variable to replace func_RetVar
                    formal2real[func_RetVar] = j->result;
                    j++;
                } else {
                    //if (symbolTable.getType(inlineFunc) != VOIDTK) {
                    //    //j--;
                    //    continue;
                    //}
                }
                j = imList.erase(start, j); // delete call and return code
                releaseTempVar();
                string inlineEndLabel = getNewLabel();
                for (FourItemExpr k : func2im[inlineFunc]) {
                    FourItemExpr temp = k;
                    auto op = temp.op;
                    if (op == opPara || op == opFunc || op == opConst || op == opReturn) {
                        if (op == opReturn) {
                            j = imList.insert(j, FourItemExpr(opGoto, inlineEndLabel)) + 1;
                        }
                        continue;
                    }
                    // rename label
                    if (op == opGoto || op == opBNZ || op == opBZ || op == opSetLabel
                        || op == opBeq || op == opBne || op == opBlt
                        || op == opBle || op == opBgt || op == opBge) {
                        if (formal2real.find(temp.result) == formal2real.end()) {
                            formal2real[temp.result] = getNewLabel();
                        }
                    }
                    if (op == opPrintf) {
                        if (isStringName(temp.result)) {
                            formal2real[temp.result] = temp.result;
                        }
                    }
                    string* codeVar[] = { &temp.first, &temp.second, &temp.result };
                    for (int p = 0; p < 3; p++) {
                        if (isTempVariable(*codeVar[p]) && *codeVar[p] != func_RetVar) {
                            codeVar[p]->insert(1, INLINE_VAR);
                        } else { 
                            // when modify a para but the real para is a number
                            // replace the number by a temp var
                            if (p == 2 && isNumberValue(formal2real[*codeVar[p]])
                                && op != opPrintf) {
                                string inlineTempVar = getTempVar().insert(1, "INLINE_PARA_");
                                formal2real[*codeVar[p]] = inlineTempVar;
                            }
                            if (formal2real.find(*codeVar[p]) != formal2real.end()
                                && !isNumberValue(*codeVar[p])) {
                                *codeVar[p] = formal2real[*codeVar[p]];
                            }
                        }
                    }
                    j = imList.insert(j, temp) + 1;
                }
                j = imList.insert(j, FourItemExpr(opSetLabel, inlineEndLabel));
            }
        }
    }
    toTotalList();
    removeRedundantJump();
    smallAdjust();
    removeRedundantJump();
    smallAdjust();
    removeRedundantJump();
    smallAdjust();
    toFunctionBlock();
    //removeUseless();
    toTotalList();
#endif // DO_INLINE
}

void IntermediateCode::removeRedundantJump() {
    map<string, int> label2addr;
    map<string, int> label2cnt;
    map<FourItemExpr, int> code2addr;
    int addr = 0;
    for (auto i = imExprList.begin(); i != imExprList.end(); i++) {
        if (i->op == opSetLabel) {
            label2addr[i->result] = addr;
        } else {
            if (i->op == opGoto || i->op == opBZ || i->op == opBNZ
                || i->op == opBeq || i->op == opBne || i->op == opBlt
                || i->op == opBle || i->op == opBgt || i->op == opBge) {
                label2cnt[i->result]++;
            }
            code2addr[*i] = addr++;
        }
    }
    for (auto i = imExprList.begin(); i != imExprList.end();) {
        if ((i->op == opGoto || i->op == opBZ || i->op == opBNZ
            || i->op == opBeq || i->op == opBne || i->op == opBlt
            || i->op == opBle || i->op == opBgt || i->op == opBge)
            && code2addr[*i] + 1 == label2addr[i->result]) {
            label2cnt[i->result]--;
            i = imExprList.erase(i);
        } else {
            i++;
        }
    }
    for (auto i = imExprList.begin(); i != imExprList.end(); i++) {
        if (i->op == opSetLabel && label2cnt[i->result] == 0) {
            i = imExprList.erase(i);
        }
    }
}

IMOper reverseOp(IMOper op) {
    switch (op) {
    case opLss:
        return opGre;
        break;
    case opLeq:
        return opGeq;
        break;
    case opGre:
        return opLss;
        break;
    case opGeq:
        return opLeq;
        break;
    case opEql:
    case opNeq:
        return op;
        break;
    default:
        return op;
        break;
    }
}

void IntermediateCode::smallAdjust() {
    set<string> localVarUse;
    for (auto i = imExprList.begin(); i != imExprList.end(); i++) {
        if ((i->op == opAdd || i->op == opMul || i->op == opEql || i->op == opNeq)
            && isNumberValue(i->first)) {
            swap(i->first, i->second);
        }
        if ((i->op == opGre || i->op == opGeq || i->op == opLss || i->op == opLeq)
            && isNumberValue(i->first)) {
            swap(i->first, i->second);
            i->op = reverseOp(i->op);
        }
        if (i->op == opSub && isNumberValue(i->second)) {
            int value = stoi(i->second);
            if (value > INT_MIN) {
                i->op = opAdd;
                i->second = to_string(-value);
            }
        }
        if (i->op == opAssign && i->result == i->first) {
            i = imExprList.erase(i);
        }
        if (symbolTable.isLocalVariable(i->first)) {
            localVarUse.insert(i->first);
        }
        if (symbolTable.isLocalVariable(i->second)) {
            localVarUse.insert(i->second);
        }
    }
    for (auto i = imExprList.begin() + 1; i != imExprList.end(); i++) {
        FourItemExpr& prev = *(i - 1);
        FourItemExpr& now = *i;
        if (prev.op == opFunc) {
            symbolTable.setNowFunctionItem(prev.first);
        }
        if (!isTempVariable(prev.result)) {
            continue;
        }
        if ((prev.op == opAdd || prev.op == opMul) 
            && prev.op == now.op && prev.result == now.first
            && isNumberValue(prev.second) && isNumberValue(now.second)) {
            int num1 = stoi(prev.second);
            int num2 = stoi(now.second);
            int result;
            if (prev.op == opAdd) {
                result = num1 + num2;
            } else {
                result = num1 * num2;
            }
            i->first = prev.first;
            if (result != 0) {
                i->second = to_string(result);
            } else {
                i->op = opAssign;
                i->second = "";
            }
            i = imExprList.erase(i - 1);
        }
        if ((prev.op == opEql || prev.op == opNeq|| prev.op == opLss || prev.op == opLeq
            || prev.op == opGre || prev.op == opGeq) && prev.result == now.first
            && (now.op == opBZ || now.op == opBNZ)) {
            IMOper op = prev.op;
            switch (op) {
            case opLss:
                if (now.op == opBZ) { now.op = opBge; } else { now.op = opBlt; }
                break;
            case opLeq:
                if (now.op == opBZ) { now.op = opBgt; } else { now.op = opBle; }
                break;
            case opGre:
                if (now.op == opBZ) { now.op = opBle; } else { now.op = opBgt; }
                break;
            case opGeq:
                if (now.op == opBZ) { now.op = opBlt; } else { now.op = opBge; }
                break;
            case opEql:
                if (now.op == opBZ) { now.op = opBne; } else { now.op = opBeq; }
                break;
            case opNeq:
                if (now.op == opBZ) { now.op = opBeq; } else { now.op = opBne; }
                break;
            default:
                break;
            }
            now.first = prev.first;
            now.second = prev.second;
            i = imExprList.erase(i - 1);
        }
    }
    for (auto i = imExprList.begin() + 1; i != imExprList.end(); i++) {
        if (isNumberValue(i->first) && isNumberValue(i->second)) {
            int first = stoi(i->first);
            int second = stoi(i->second);
            switch (i->op) {
            case opAdd:
                *i = FourItemExpr(opAssign, i->result, to_string(first + second));
                break;
            case opSub:
                *i = FourItemExpr(opAssign, i->result, to_string(first - second));
                break;
            case opMul:
                *i = FourItemExpr(opAssign, i->result, to_string(first * second));
                break;
            case opDiv:
                if (second != 0) {
                    *i = FourItemExpr(opAssign, i->result, to_string(first / second));
                }
                break;
            case opMod:
                if (second != 0) {
                    *i = FourItemExpr(opAssign, i->result, to_string(first % second));
                }
                break;
            case opBeq:
                if (first == second) {
                    *i = FourItemExpr(opGoto, i->result);
                } else {
                    i = imExprList.erase(i);
                }
                break;
            case opBne:
                if (first != second) {
                    *i = FourItemExpr(opGoto, i->result);
                } else {
                    i = imExprList.erase(i);
                }
                break;
            case opBlt:
                if (first < second) {
                    *i = FourItemExpr(opGoto, i->result);
                } else {
                    i = imExprList.erase(i);
                }
                break;
            case opBle:
                if (first <= second) {
                    *i = FourItemExpr(opGoto, i->result);
                } else {
                    i = imExprList.erase(i);
                }
                break;
            case opBgt:
                if (first > second) {
                    *i = FourItemExpr(opGoto, i->result);
                } else {
                    i = imExprList.erase(i);
                }
                break;
            case opBge:
                if (first >= second) {
                    *i = FourItemExpr(opGoto, i->result);
                } else {
                    i = imExprList.erase(i);
                }
                break;
            default:
                break;
            }
        }
    }
    for (auto i = imExprList.begin() + 1; i < imExprList.end(); i++) {
        auto prev = i - 1;
        auto now = i;
        if (prev->op == opFunc) {
            symbolTable.setNowFunctionItem(prev->first);
        }
        if ((prev->op == opAdd || prev->op == opMul || prev->op == opDiv || prev->op == opSub 
            || prev->op == opMod || prev->op == opGetRetVal || prev->op == opAssign)
            && now->op == opAssign && prev->result == now->first
            && isTempVariable(prev->result)) {
            prev->result = now->result;
            i = imExprList.erase(i);
        }
    }
    //#TEMP_VAR_0 = x / y
    //#TEMP_VAR_1 = #TEMP_VAR_0 * y
    //var = x - #TEMP_VAR_1
    // result: var = x % y
#ifdef OPT_MOD
    for (auto i = imExprList.begin() + 2; i < imExprList.end(); i++) {
        auto expr1 = i - 2;
        auto expr2 = i - 1;
        auto expr3 = i;
        if (expr1->op == opFunc) {
            symbolTable.setNowFunctionItem(expr1->first);
        }
        if (!symbolTable.isLocalVariable(expr1->result)
            || !symbolTable.isLocalVariable(expr1->first)
            || !symbolTable.isLocalVariable(expr1->second)
            || !symbolTable.isLocalVariable(expr2->result)
            || !symbolTable.isLocalVariable(expr2->first)
            || !symbolTable.isLocalVariable(expr2->second)
            || !symbolTable.isLocalVariable(expr3->result)
            || !symbolTable.isLocalVariable(expr3->first)
            || !symbolTable.isLocalVariable(expr3->second)) {
            continue;
        }
        if ((!isTempVariable(expr1->result) || !isTempVariable(expr2->result))
            && expr3->result != func_RetVar) {
            continue;
        }
        if (expr1->op == opDiv && expr2->op == opMul && expr3->op == opSub) {
            string quotient = expr1->result;
            string dividend = expr1->first;
            string divisor = expr1->second;
            if (((quotient == expr2->first && divisor == expr2->second)
                || (quotient == expr2->second && divisor == expr2->first))
                && expr3->first == dividend
                && expr3->second == expr2->result) {
                expr1->op = opMod;
                expr1->result = expr3->result;
                i = imExprList.erase(i - 1, i + 1);
            }
        }
    }
#endif // OPT_MOD
}

string FourItemExpr::toString() {
    static map<IMOper, string> op2string = {
    {opAdd,"+" }, {opSub,"-"}, {opMul,"*"}, {opDiv, "/"},
    {opAssign, "="}, {opLoadArray, "[]"}, {opStore2Array, "[]="},
    {opSetLabel, "setLabel"}, {opPush, "push"}, {opCall, "call"},
    {opLss, "<"}, {opLeq, "<="}, {opGre, ">" }, {opGeq,">="}, {opEql, "=="}, {opNeq, "!="},
    {opGoto, "goto"}, {opBNZ, "BNZ"}, {opBZ, "BZ"},
    {opScanf, "scanf"}, {opPrintf, "printf"},
    {opReturn, "return"}, {opGetRetVal, "retValue"},
    {opFunc, "function"} , {opVar, "var"}, {opPara, "para"}, {opConst, "const"},
    {opBeq, "beq"}, {opBne, "bne"},
    {opBlt, "blt"}, {opBle, "ble"}, {opBgt, "bgt"}, {opBge, "bge"},
    {opMod, "%"},
    };
    ostringstream ostr;
    switch (op) {
    case opAdd: case opSub: case opMul: case opDiv: case opAssign: case opMod:
    case opLss: case opLeq: case opGre: case opGeq: case opEql: case opNeq: 
        ostr << "\t";
        if (result.length() > 0) {
            ostr << result << "\t";
        }
        ostr << "=\t";
        ostr << first << "\t";
        if (second.length() > 0) {
            ostr << op2string[op] << "\t";
            ostr << second;
        }
        break;
    case opLoadArray:
        ostr << "\t";
        ostr << result << "\t=\t";
        ostr << first << "[" << second << "]";
        break;
    case opStore2Array:
        ostr << "\t" << first;
        ostr << "[" << second << "]" << "\t=\t";
        ostr << result;
        break;
    case opSetLabel:
        ostr << result << ":";
        break;
    case opBNZ: case opBZ: case opGoto: case opBeq: case opBne:
    case opBlt: case opBle: case opBgt: case opBge:
        ostr << "\t";
        ostr << op2string[op] << "\t";
        if (first.length() > 0) {
            ostr << first << "\t";
        }
        if (second.length() > 0) {
            ostr << second << "\t";
        }
        if (result.length() > 0) {
            ostr << result;
        }
        break;
    case opPush: 
        ostr << "\t" << op2string[op] << "\t" << result;
        break;
    case opCall:
    case opScanf: case opPrintf: case opReturn: case opGetRetVal: 
    case opFunc: case opVar: case opPara: case opConst: 
        if (op != opFunc) {
            ostr << "\t";
        }
        ostr << op2string[op] << "\t";
        if (result.length() > 0) {
            ostr << result << "\t";
        }
        if (first.length() > 0) {
            ostr << first << "\t";
        }
        if (op == opConst) {
            ostr << "=\t";
        }
        if (second.length() > 0) {
            ostr << second;
        }
        if (op == opFunc) {
            ostr << "()";
        }
        break;
    default:
        break;
    }
    return ostr.str();
}

void IntermediateCode::dumpCode2File(FILE* file) {
    for (auto i = imExprList.begin(); i != imExprList.end(); i++) {
        fprintf(file, "%s\n", i->toString().c_str());
    }
}

string IntermediateCode::getTempVar() {
    string var = string("#TEMP_VAR_" + std::to_string(tempVarMark++));
    return var;
}

void IntermediateCode::releaseTempVar() {
    tempVarMark = 0;
}

string IntermediateCode::getNewLabel() {
    static unsigned int cnt = 0;
    string label = string("LABEL_" + std::to_string(cnt++));
    return label;
}