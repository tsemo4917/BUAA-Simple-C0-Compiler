#include <cstdio>
#include <string>
#include <vector>
#include <stack>
#include "lexicalAnalysis.h"
#include "syntaxAnalysis.h"
#include "OutputHandler.h"
#include "ErrorHandler.h"
#include "SymbolTable.h"
#include "IntermediateCode.h"
#include "util.h"

using std::string;
using std::vector;
using std::stack;
using std::to_string;
using std::stoi;

extern Sym symbol;
extern string token;
extern long constantValue;

extern OutputHandler* outputHandler;
extern ErrorHandler* errorHandler;

SymbolTable symbolTable;

void isPrograme() {
    symbolTable.setErrorHandler(errorHandler);
    if (symbol == CONSTTK) {
        constantExplanation();
    }
    if (symbol == INTTK || symbol == CHARTK) {
        getsym();
        if (symbol != IDENFR) {
            error();
        } else {
            getsym();
            Sym temp = symbol;
            backtrace(3);
            getsym();
            if (temp != LPARENT && (symbol == INTTK || symbol == CHARTK)) {
                variableExplanation();
            }
        }
    }
    while (true) {
        if (symbol == INTTK || symbol == CHARTK) {
            functionWithReturnDef();
        } else if (symbol == VOIDTK) {
            getsym();
            if (symbol == IDENFR) {
                functionVoidDef();
            }
        } else {
            break;
        }
    }
    mainFunction();
    outputHandler->addString("<程序>");
}

void functionWithReturnDef() {
    if (symbol != INTTK && symbol != CHARTK) { error(); }
    Sym type = symbol;
    getsym();
    if (symbol != IDENFR) {
        error();
    } else {
        string funcName = token;
        symbolTable.checkRedefine(token);
        FunctionItem* item = new FunctionItem(funcName, type);
        symbolTable.setNowFunctionItem(item);
        getsym();
        outputHandler->addString("<声明头部>");
        if (symbol != LPARENT) {
            error();
        } else {
            getsym();
            parameterTable();
            symbolTable.addItem(item);
            IntermediateCode::addFunctionDef(*item);
            IntermediateCode::addParameter(item->parameters);
            if (symbol != RPARENT) {
                errorHandler->addErrorInfo(RightParenthesisExpected);
            } else {
                getsym();
            }
            if (symbol != LBRACE) {
                error();
            } else {
                getsym();
                bool hasReturn = compoundStatement();
                if (!hasReturn) {
                    errorHandler->addErrorInfo(ReturnError);
                }
                if (symbol != RBRACE) {
                    error();
                } else {
                    getsym();
                }
            }
        }
    }
    symbolTable.releaseNowFunctionItem();
    outputHandler->addString("<有返回值函数定义>");
}

void functionVoidDef() {
    if (symbol != IDENFR) { error(); }
    string funcName = token;
    symbolTable.checkRedefine(funcName);
    FunctionItem* item = new FunctionItem(funcName, VOIDTK);
    symbolTable.setNowFunctionItem(item);
    getsym();
    if (symbol != LPARENT) {
        error();
    } else {
        getsym();
        parameterTable();
        symbolTable.addItem(item);
        IntermediateCode::addFunctionDef(*item);
        IntermediateCode::addParameter(item->parameters);
        if (symbol != RPARENT) {
            errorHandler->addErrorInfo(RightParenthesisExpected);
        } else {
            getsym();
        }
        if (symbol != LBRACE) {
            error();
        } else {
            getsym();
            compoundStatement();
            if (symbol != RBRACE) {
                error();
            } else {
                getsym();
            }
        }
    }
    IntermediateCode::functionReturn("");
    symbolTable.releaseNowFunctionItem();
    outputHandler->addString("<无返回值函数定义>");
}

void mainFunction() {
    if (symbol != MAINTK) { error(); }
    string funcName = token;
    symbolTable.checkRedefine(funcName);
    FunctionItem* item = new FunctionItem(funcName, VOIDTK);
    symbolTable.addItem(item);
    symbolTable.setNowFunctionItem(item);
    IntermediateCode::addFunctionDef(*item);
    getsym();
    if (symbol != LPARENT) {
        error();
    } else {
        getsym();
        if (symbol != RPARENT) {
            errorHandler->addErrorInfo(RightParenthesisExpected);
        } else {
            getsym();
        }
        if (symbol != LBRACE) {
            error();
        } else {
            getsym();
            compoundStatement();
            if (symbol != RBRACE) {
                error();
            } else {
                getsym();
            }
        }
    }
    IntermediateCode::functionReturn("");
    symbolTable.releaseNowFunctionItem();
    outputHandler->addString("<主函数>");
}

void constantExplanation() {
    if (symbol != CONSTTK) {
        error();
    } else {
        do {
            getsym();
            constantDefinition();
            if (symbol != SEMICN) {
                errorHandler->addErrorInfo(SemicolonExpected);
            } else {
                getsym();
            }
        } while (symbol == CONSTTK);
        outputHandler->addString("<常量说明>");
    }
}

void constantDefinition() {
    if (symbol != INTTK && symbol != CHARTK) { error(); }
    Sym type = symbol;
    do {
        getsym();
        if (symbol != IDENFR) {
            error();
        } else {
            string name = token;
            symbolTable.checkRedefine(name);
            getsym();
            if (symbol != ASSIGN) {
                error();
            }
            getsym();
            long value = 0;
            if (type == INTTK) {
                if (!isInteger(value)) {
                    errorHandler->addErrorInfo(OnlyIntCharAssignConstant);
                    getsym();
                }
            } else if (type == CHARTK) {
                if (symbol != CHARCON) {
                    errorHandler->addErrorInfo(OnlyIntCharAssignConstant);
                }
                value = constantValue;
                getsym();
            } else {
                error();
            }
            ConstantItem* item = new ConstantItem(name, type, value);
            symbolTable.addItem(item);
            IntermediateCode::addConstantDef(*item);
        }
    } while (symbol == COMMA);
    outputHandler->addString("<常量定义>");
}

void variableExplanation() {
    do {
        bool res = variableDefinition();
        if (!res) {
            backtrace(4);
            getsym();
        }
        if (symbol != SEMICN) {
            errorHandler->addErrorInfo(SemicolonExpected);
        } else {
            getsym();
            if (!res) {
                break;
            }
        }
    } while (symbol == INTTK || symbol == CHARTK);
    outputHandler->addString("<变量说明>");
}

bool variableDefinition() {
    const int s1 = 1, s2 = 2, s3 = 3, s4 = 4, s5 = 5, sEnd = 6, sError = 7;
    int state = s1;
    string name;
    int arrayLength = 0;
    Sym type = symbol;
    getsym();
    ErrorType errorType = OtherError;
    do {
        switch (state) {
        case s1:
            if (symbol == IDENFR) {
                name = token;
                state = s2;
                getsym();
            } else {
                state = sError;
                errorType = OtherError;
            }
            break;
        case s2:
            if (symbol == LBRACK) {
                state = s3;
                getsym();
            } else if (symbol == COMMA) {
                symbolTable.checkRedefine(name);
                VariableItem* item = new VariableItem(name, type, arrayLength);
                symbolTable.addItem(item);
                IntermediateCode::addVariableDef(*item);
                state = s1;
                getsym();
            } else if (symbol == LPARENT) {
                return false;
            } else {
                state = sEnd;
            }
            break;
        case s3:
            if (symbol == INTCON) {
                arrayLength = constantValue;
                state = s4;
                isUnsignedInteger();
            } else {
                state = sError;
                errorType = SubscriptNotInt;
            }
            break;
        case s4:
            if (symbol == RBRACK) {
                state = s5;
                getsym();
            } else {
                state = sError;
                errorType = RightBracketsExpected;
            }
            break;
        case s5:
            if (symbol == COMMA) {
                symbolTable.checkRedefine(name);
                VariableItem* item = new VariableItem(name, type, arrayLength);
                symbolTable.addItem(item);
                IntermediateCode::addVariableDef(*item);
                state = s1;
                getsym();
            } else {
                state = sEnd;
            }
            break;
        default:
            break;
        }
    } while (state != sEnd && state != sError);
    if (state == sError) {
        errorHandler->addErrorInfo(errorType);
    }
    symbolTable.checkRedefine(name);
    VariableItem* item = new VariableItem(name, type, arrayLength);
    symbolTable.addItem(item);
    IntermediateCode::addVariableDef(*item);
    outputHandler->addString("<变量定义>");
    return true;
}

bool isUnsignedInteger() {
    if (symbol != INTCON) {
        return false;
    } else {
        getsym();
    }
    outputHandler->addString("<无符号整数>");
    return true;
}

bool isInteger(long& value) {
    int coef = 1;
    if (symbol == PLUS || symbol == MINU) {
        if (symbol == MINU) {
            coef = -1;
        }
        getsym();
    }
    value = constantValue;
    bool result = isUnsignedInteger();
    value *= coef;
    outputHandler->addString("<整数>");
    return result;
}

void isString() {
    if (symbol != STRCON) {
        error();
    } else {
        string str = token;
        IntermediateCode::addString(str);
        getsym();
    }
    outputHandler->addString("<字符串>");
}

bool compoundStatement() {
    if (symbol == CONSTTK) {
        constantExplanation();
    }
    if (symbol == INTTK || symbol == CHARTK) {
        variableExplanation();
    }
    bool hasReturn = statementCloumn();
    outputHandler->addString("<复合语句>");
    return hasReturn;
}
//＜参数表＞::=  ＜类型标识符＞＜标识符＞{,＜类型标识符＞＜标识符＞}| ＜空＞
void parameterTable() {
    if (symbol == INTTK || symbol == CHARTK) {
        Sym type = symbol;
        getsym();
        if (symbol != IDENFR) {
            error();
        }
        string name = token;
        symbolTable.checkRedefine(name);
        VariableItem* item = new VariableItem(name, type);
        symbolTable.addParameter(item);
        getsym();
        while (symbol == COMMA) {
            getsym();
            if (symbol != INTTK && symbol != CHARTK) {
                error();
            }
            type = symbol;
            getsym();
            if (symbol != IDENFR) {
                error();
            }
            name = token;
            symbolTable.checkRedefine(name);
            VariableItem* item = new VariableItem(name, type);
            symbolTable.addParameter(item);
            getsym();
        }
    }
    outputHandler->addString("<参数表>");
}
string popTop(stack<string>& s) {
    string ret = s.top();
    s.pop();
    return ret;
}
static stack<string> ns;
Sym expression() {
    string resultVar;
    string op = "+";
    if (symbol == PLUS || symbol == MINU) {
        op = token;
        getsym();
    }
    Sym retType = term();
    if (op == "-") {
        string num1 = popTop(ns);
        if (isNumberValue(num1)) {
            resultVar = std::to_string(-std::stol(num1));
        } else {
            if (isTempVariable(num1)) {
                resultVar = num1;
            } else {
                resultVar = IntermediateCode::getTempVar();
            }
            IntermediateCode::add4ItemExpr(str2imop(op), resultVar, "0", num1);
        }
        ns.push(resultVar);
    }
    while (symbol == PLUS || symbol == MINU) {
        op = token;
        getsym();
        term();
        retType = INTTK;
        string num2 = popTop(ns);
        string num1 = popTop(ns);
        if (isNumberValue(num2) && isNumberValue(num1)) {
            long b = std::stol(num2);
            long a = std::stol(num1);
            if (op == "-") {
                resultVar = std::to_string(a - b);
            } else {
                resultVar = std::to_string(a + b);
            }
        } else {
            if (isNumberValue(num2) && std::stol(num2) == 0) {
                resultVar = num1;
            } else if (isNumberValue(num1) && op == "+" && std::stol(num1) == 0) {
                resultVar = num2;
            } else {
                if (isTempVariable(num1)) {
                    resultVar = num1;
                } else if (isTempVariable(num2)) {
                    resultVar = num2;
                } else {
                    resultVar = IntermediateCode::getTempVar();
                }
                IntermediateCode::add4ItemExpr(str2imop(op), resultVar, num1, num2);
            }
        }
        ns.push(resultVar);
    }
    outputHandler->addString("<表达式>");
    return retType;
}

Sym term() {
    string resultVar = "";
    Sym retType = factor();
    while (symbol == MULT || symbol == DIV) {
        string op = token;
        getsym();
        factor();
        retType = INTTK;
        string num2 = popTop(ns);
        string num1 = popTop(ns);
        if (isNumberValue(num2) && isNumberValue(num1)) {
            long b = std::stol(num2);
            long a = std::stol(num1);
            if (op == "/") {
                resultVar = std::to_string(a / b);
            } else {
                resultVar = std::to_string(a * b);
            }
        } else {
            if ((isNumberValue(num1) && std::stol(num1) == 0)
                || (isNumberValue(num2) && op == "*" && std::stol(num2) == 0)) {
                resultVar = to_string(0);
            } else if (isNumberValue(num2) && std::stol(num2) == 1) {
                resultVar = num1;
            } else if (isNumberValue(num1) && op == "*" && std::stol(num1) == 1) {
                resultVar = num2;
            } else {
                if (isTempVariable(num1)) {
                    resultVar = num1;
                } else if (isTempVariable(num2)) {
                    resultVar = num2;
                } else {
                    resultVar = IntermediateCode::getTempVar();
                }
                IntermediateCode::add4ItemExpr(str2imop(op), resultVar, num1, num2);
            }
        }
        ns.push(resultVar);
    }
    outputHandler->addString("<项>");
    return retType;
}

Sym factor() {
    string resultVar = "";
    Sym retType = VOIDTK;
    if (symbol == INTCON || symbol == PLUS || symbol == MINU) {
        retType = INTTK;
        long value = 0;
        isInteger(value);
        resultVar = std::to_string(value);
    } else if (symbol == CHARCON) {
        retType = CHARTK;
        resultVar = std::to_string(constantValue);
        getsym();
    } else if (symbol == LPARENT) {
        getsym();
        expression();
        resultVar = popTop(ns);
        if (symbol != RPARENT) {
            errorHandler->addErrorInfo(RightParenthesisExpected);
        } else {
            getsym();
        }
        retType = INTTK;
    } else if (symbol == IDENFR) {
        string name = token;
        getsym();
        if (symbol == LPARENT) {
            retType = symbolTable.getType(name);
            functionCall(name);
            resultVar = IntermediateCode::getTempVar();
            IntermediateCode::getReturnValue(resultVar);
        } else {
            bool pass = symbolTable.checkUndefine(name);
            if (pass) {
                retType = symbolTable.getType(name);
            }
            if (symbol == LBRACK) {
                getsym();
                Sym subscriptType = expression();
                string subscript = popTop(ns);
                resultVar = IntermediateCode::getTempVar();
                IntermediateCode::add4ItemExpr(str2imop("[]"), resultVar, name, subscript);
                if (subscriptType != INTTK) {
                    errorHandler->addErrorInfo(SubscriptNotInt);
                }
                if (symbol != RBRACK) {
                    errorHandler->addErrorInfo(RightBracketsExpected);
                } else {
                    getsym();
                }
            } else {
                if (symbolTable.getItem(name)->getKind() == CONSTANT) {
                    ConstantItem* item = (ConstantItem*)symbolTable.getItem(name);
                    resultVar = std::to_string(item->getValue());
                } else {
                    resultVar = name;
                }
            }
        }
    } else { error(); }
    ns.push(resultVar);
    outputHandler->addString("<因子>");
    return retType;
}

void assignmentStatement(string name) {
    string subscript = "";
    if (symbol == LBRACK) {
        getsym();
        if (expression() != INTTK) {
            errorHandler->addErrorInfo(SubscriptNotInt);
        }
        subscript = popTop(ns);
        if (symbol != RBRACK) {
            errorHandler->addErrorInfo(RightBracketsExpected);
        } else {
            getsym();
        }
    }
    string result = "";
    if (symbol != ASSIGN) {
        error();
    } else {
        getsym();
        expression();
        result = popTop(ns);
    }
    if (subscript.length() > 0) {
        IntermediateCode::add4ItemExpr(opStore2Array, result, name, subscript);
    } else {
        IntermediateCode::add4ItemExpr(opAssign, name, result);
    }
    outputHandler->addString("<赋值语句>");
}

bool conditionalStatement() {
    bool hasReturn = false;
    string endLabel = IntermediateCode::getNewLabel();
    string elseLabel = IntermediateCode::getNewLabel();
    bool skipIf = false, skipElse = false;
    if (symbol != IFTK) { error(); } 
    getsym();
    if (symbol != LPARENT) {
        error();
    } else {
        getsym();
        isCondition();
        string result = popTop(ns);
        if (isNumberValue(result)) {
            if (stoi(result) == 0) {
                skipIf = true;
            } else {
                skipElse = true;
            }
        }
        if (skipIf) {
            IntermediateCode::skip = true;
        }
        if (!isNumberValue(result)) {
            IntermediateCode::add4ItemExpr(opBZ, elseLabel, result);
        }
        if (symbol != RPARENT) {
            errorHandler->addErrorInfo(RightParenthesisExpected);
        } else {
            getsym();
        }
        hasReturn = statement();
        if (!isNumberValue(result)) {
            IntermediateCode::add4ItemExpr(opGoto, endLabel);
            IntermediateCode::setLabel(elseLabel);
        }
        if (skipIf) {
            IntermediateCode::skip = false;
        }
        if (symbol == ELSETK) {
            if (skipElse) {
                IntermediateCode::skip = true;
            }
            getsym();
            bool temp = statement();
            if (!temp && !hasReturn) {
                hasReturn = false;
            }
            if (skipElse) {
                IntermediateCode::skip = false;
            }
        }
    }
    IntermediateCode::setLabel(endLabel);
    outputHandler->addString("<条件语句>");
    return hasReturn;
}

void isCondition() {
    Sym type = expression();
    string var1 = popTop(ns);
    bool isIntExpr = (type == INTTK);
    string resultVar;
    if (symbol == LSS || symbol == LEQ || symbol == GRE || symbol == GEQ
        || symbol == EQL || symbol == NEQ) {
        Sym opSym = symbol;
        string op = token;
        getsym();
        type = expression();
        isIntExpr = (isIntExpr && (type == INTTK));
        string var2 = popTop(ns);
        if (isNumberValue(var1) && isNumberValue(var2)) {
            int num1 = std::stoi(var1);
            int num2 = std::stoi(var2);
            resultVar = std::to_string(opSym == LSS ? num1 < num2 :
                opSym == LEQ ? num1 <= num2 :
                opSym == GRE ? num1 > num2 :
                opSym == GEQ ? num1 >= num2 :
                opSym == EQL ? num1 == num2 :
                num1 != num2);
        } else {
            resultVar = IntermediateCode::getTempVar();
            IntermediateCode::add4ItemExpr(str2imop(op), resultVar, var1, var2);
        }
    } else {
        resultVar = var1;
    }
    if (!isIntExpr) {
        errorHandler->addErrorInfo(IllegalTypeInCondition);
    }
    ns.push(resultVar);
    outputHandler->addString("<条件>");
}

bool loopStatement() {
    bool hasReturn = false;
    if (symbol == DOTK) {
        getsym();
        string startLabel = IntermediateCode::getNewLabel();
        IntermediateCode::setLabel(startLabel);
        hasReturn = statement();
        if (symbol != WHILETK) {
            errorHandler->addErrorInfo(WhileExpected);
        } else {
            getsym();
        }
        if (symbol != LPARENT) {
            error();
        } else {
            getsym();
            isCondition();
            string result = popTop(ns);
            if (isNumberValue(result)) {
                // do while (0) no need to compare
                // do while (1) is dead loop, just jump
                if (stoi(result) != 0) {
                    IntermediateCode::add4ItemExpr(opGoto, startLabel);
                }
            } else {
                IntermediateCode::add4ItemExpr(opBNZ, startLabel, result);
            }
            if (symbol != RPARENT) {
                errorHandler->addErrorInfo(RightParenthesisExpected);
            } else {
                getsym();
            }
        }
    } else if (symbol == WHILETK) {
        string startLabel = IntermediateCode::getNewLabel();
        string endLabel = IntermediateCode::getNewLabel();
        //IntermediateCode::setLabel(startLabel);
        getsym();
        if (symbol != LPARENT) { error(); }
        getsym();
        auto startCondition = IntermediateCode::imExprList.size();
        isCondition();
        vector<FourItemExpr> booleanCode = vector<FourItemExpr>(
            IntermediateCode::imExprList.begin() + startCondition, IntermediateCode::imExprList.end());
        string boolenResult = popTop(ns);
        bool skip = isNumberValue(boolenResult) && stoi(boolenResult) == 0;
        if (skip) {
            IntermediateCode::skip = true;
        }
        if (!isNumberValue(boolenResult)) {
            IntermediateCode::add4ItemExpr(opBZ, endLabel, boolenResult);
        }
        IntermediateCode::setLabel(startLabel);
        if (symbol != RPARENT) {
            errorHandler->addErrorInfo(RightParenthesisExpected);
        } else {
            getsym();
        }
        hasReturn = statement();
        for (auto expr : booleanCode) {
            IntermediateCode::add4ItemExpr(expr.op, expr.result, expr.first, expr.second);
        }
        IntermediateCode::add4ItemExpr(opBNZ, startLabel, boolenResult);
        //IntermediateCode::add4ItemExpr(opGoto, startLabel);
        IntermediateCode::setLabel(endLabel);
        if (skip) {
            IntermediateCode::skip = false;
        }
    } else if (symbol == FORTK) {
        string startLabel = IntermediateCode::getNewLabel();
        string endLabel = IntermediateCode::getNewLabel();
        getsym();
        if (symbol != LPARENT) { error(); }
        getsym();
        if (symbol != IDENFR) { error(); }
        string var1 = token;
        symbolTable.checkUndefine(var1);
        getsym();
        if (symbol != ASSIGN) { error(); }
        getsym();
        expression();
        string result = popTop(ns);
        IntermediateCode::add4ItemExpr(opAssign, var1, result);
        //IntermediateCode::setLabel(startLabel);
        if (symbol != SEMICN) {
            errorHandler->addErrorInfo(SemicolonExpected);
        } else {
            getsym();
        }
        auto startCondition = IntermediateCode::imExprList.size();
        isCondition();
        vector<FourItemExpr> booleanCode = vector<FourItemExpr>(
            IntermediateCode::imExprList.begin() + startCondition, IntermediateCode::imExprList.end());
        string boolenResult = popTop(ns);
        bool skip = isNumberValue(boolenResult) && stoi(boolenResult) == 0;
        if (skip) {
            IntermediateCode::skip = true;
        }
        if (!isNumberValue(boolenResult)) {
            IntermediateCode::add4ItemExpr(opBZ, endLabel, boolenResult);
        }

        IntermediateCode::setLabel(startLabel);

        if (symbol != SEMICN) {
            errorHandler->addErrorInfo(SemicolonExpected);
        } else {
            getsym();
        }
        if (symbol != IDENFR) { error(); }
        string var2 = token;
        symbolTable.checkUndefine(var2);
        getsym();
        if (symbol != ASSIGN) { error(); }
        getsym();
        if (symbol != IDENFR) {
            error();
        }
        string var3 = token;
        symbolTable.checkUndefine(var3);
        getsym();
        string op = token;
        if (symbol != PLUS && symbol != MINU) { error(); }
        getsym();
        string step = token;
        isUnsignedInteger();
        outputHandler->addString("<步长>");
        if (symbol != RPARENT) {
            errorHandler->addErrorInfo(RightParenthesisExpected);
        } else {
            getsym();
        }
        hasReturn = statement();
        IntermediateCode::add4ItemExpr(str2imop(op), var2, var3, step);

        for (auto expr : booleanCode) {
            IntermediateCode::add4ItemExpr(expr.op, expr.result, expr.first, expr.second);
        }
        IntermediateCode::add4ItemExpr(opBNZ, startLabel, boolenResult);
        //IntermediateCode::add4ItemExpr(opGoto, startLabel);
        IntermediateCode::setLabel(endLabel);
        if (skip) {
            IntermediateCode::skip = false;
        }
    }
    outputHandler->addString("<循环语句>");
    return hasReturn;
}

void functionCall(string funcName) {
    if (symbol != LPARENT) {
        error();
    } else {
        getsym();
        vector<Sym> types = valueParameterTable(funcName);
        symbolTable.checkParameters(funcName, types);
        if (symbol != RPARENT) {
            errorHandler->addErrorInfo(RightParenthesisExpected);
        } else {
            getsym();
        }
    }
    IntermediateCode::callFunction(funcName);
    outputHandler->addString(
        symbolTable.getType(funcName) == VOIDTK ? "<无返回值函数调用语句>" : "<有返回值函数调用语句>");
}

bool statementCloumn() {
    bool hasReturn = false;
    while (symbol == IFTK || symbol == FORTK || symbol == WHILETK || symbol == DOTK
        || symbol == RETURNTK || symbol == SCANFTK || symbol == PRINTFTK
        || symbol == LBRACE || symbol == IDENFR || symbol == SEMICN) {
        bool temp = statement();
        if (temp) {
            hasReturn = true;
        }
    }
    outputHandler->addString("<语句列>");
    return hasReturn;
}

bool statement() {
    bool hasReturn = false;
    if (symbol == IFTK) {
        hasReturn = conditionalStatement();
    } else if (symbol == FORTK || symbol == WHILETK || symbol == DOTK) {
        hasReturn = loopStatement();
    } else if (symbol == LBRACE) {
        getsym();
        hasReturn = statementCloumn();
        if (symbol != RBRACE) {
            error();
        } else {
            getsym();
        }
    } else {
        if (symbol == RETURNTK) {
            returnStatement();
            hasReturn = true;
        } else if (symbol == SCANFTK) {
            scanfStatement();
        } else if (symbol == PRINTFTK) {
            printfStatement();
        } else if (symbol == IDENFR) {
            string name = token;
            getsym();
            if (symbol == LPARENT) {
                functionCall(name);
            } else {
                symbolTable.checkIsConstant(name);
                assignmentStatement(name);
            }
        }
        if (symbol != SEMICN) {
            errorHandler->addErrorInfo(SemicolonExpected);
        } else {
            getsym();
        }
    }
    IntermediateCode::releaseTempVar();
    outputHandler->addString("<语句>");
    return hasReturn;
}

vector<Sym> valueParameterTable(string funcName) {
    vector<Sym> typesList;
    vector<string> paraList;
    if (symbol == PLUS || symbol == MINU || symbol == INTCON || symbol == CHARCON || symbol == LPARENT || symbol == IDENFR) {
        Sym type = expression();
        string paraStr = popTop(ns);
        typesList.push_back(type);
        paraList.push_back(paraStr);
        while (symbol == COMMA) {
            getsym();
            Sym type = expression();
            paraStr = popTop(ns);
            typesList.push_back(type);
            paraList.push_back(paraStr);
        }
    }
    for (auto i = paraList.begin(); i != paraList.end(); i++) {
        IntermediateCode::pushParameter(*i, funcName);
    }
    outputHandler->addString("<值参数表>");
    return typesList;
}

void scanfStatement() {
    if (symbol != SCANFTK) {
        error();
    } else {
        getsym();
        if (symbol != LPARENT) {
            error();
        } else {
            do {
                getsym();
                if (symbol != IDENFR) {
                    error();
                } else {
                    string name = token;
                    symbolTable.checkUndefine(name);
                    IntermediateCode::scanfFunction(*symbolTable.getItem(name));
                    getsym();
                }
            } while (symbol == COMMA);
            if (symbol != RPARENT) {
                errorHandler->addErrorInfo(RightParenthesisExpected);
            } else {
                getsym();
            }
        }
    }
    outputHandler->addString("<读语句>");
}

void printfStatement() {
    if (symbol != PRINTFTK) {
        error();
    }
    getsym();
    if (symbol != LPARENT) {
        error();
    } else {
        getsym();
        string str = "";
        string result = "";
        Sym type = VOIDTK;
        if (symbol == STRCON) {
            str = token;
            isString();
            if (symbol == COMMA) {
                getsym();
            }
        } 
        if (symbol != RPARENT) {
            type = expression();
            result = popTop(ns);
        }
        string typeStr = "";
        if (type != VOIDTK) {
            typeStr = type == INTTK ? "int" : "char";
        }
        IntermediateCode::printfFunction(str, result, typeStr);
        if (symbol != RPARENT) {
            errorHandler->addErrorInfo(RightParenthesisExpected);
        } else {
            getsym();
        }
    }
    outputHandler->addString("<写语句>");
}

void returnStatement() {
    Sym type = VOIDTK;
    string result = "";
    if (symbol != RETURNTK) {
        error();
    }
    getsym();
    if (symbol == LPARENT) {
        getsym();
        type = expression();
        result = popTop(ns);
        if (symbol != RPARENT) {
            errorHandler->addErrorInfo(RightParenthesisExpected);
        } else {
            getsym();
        }
    }
    IntermediateCode::functionReturn(result);
    symbolTable.checkReturnType(type);
    outputHandler->addString("<返回语句>");
}