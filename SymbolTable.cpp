#include "SymbolTable.h"
#include "util.h"
Item::Item(string name, Sym type, Kind kind) {
    this->name = name;
    this->type = type;
    this->kind = kind;
}

VariableItem::VariableItem(string name, Sym type, int length)
    : Item(name, type, VARIABLE) {
    this->arrayLength = length;
}

ConstantItem::ConstantItem(string name, Sym type, int value)
    : Item(name, type, CONSTANT) {
    this->value = value;
}

FunctionItem::FunctionItem(string name, Sym type)
    : Item(name, type, FUNCTION) {
    //this->size = 0;
}

bool FunctionItem::containsName(string& name) {
    for (auto i = parameters.begin(); i != parameters.end(); i++) {
        if ((*i)->getName() == name) {
            return true;
        }
    }
    return (varTable.find(name) != varTable.end() || constTable.find(name) != constTable.end());
}

void FunctionItem::addItem(Item* item) {
    if (item->getKind() == CONSTANT) {
        this->constTable[item->getName()] = (ConstantItem*)item;
    } else if (item->getKind() == VARIABLE) {
        VariableItem* var = (VariableItem*)item;
        this->varTable[item->getName()] = var;
        //if (var->getArrayLength() > 0) {
        //    if (var->getType() == INTTK) {
        //        size += var->getArrayLength() * 4;
        //    } else {
        //        size += (var->getArrayLength() / 4 + 1) * 4;
        //    }
        //} else {
        //    size += 4;
        //}
        //offsetTable[var->getName()] = size;
    }
}

void FunctionItem::addParameter(VariableItem* item) {
    parameters.push_back(item);
    //if (parameters.size() > 3) {
    //    size += 4;
    //    offsetTable[item->getName()] = size;
    //}
}

void SymbolTable::addItem(Item* item) {
    if (nowFunction != NULL && item->getKind() != FUNCTION) {
        nowFunction->addItem(item);
    } else {
        Kind kind = item->getKind();
        if (kind == FUNCTION) {
            addFunction((FunctionItem*)item);
        } else if (kind == CONSTANT) {
            addConstant((ConstantItem*)item);
        } else if (kind == VARIABLE) {
            addVariable((VariableItem*)item);
        }
    }
}

bool SymbolTable::containsName(string& name) {
    return (this->constTable.find(name) != this->constTable.end()
        || this->varTable.find(name) != this->varTable.end()
        || this->funcTable.find(name) != this->funcTable.end());
}

bool FunctionItem::containsConstant(string& name) {
    return constTable.find(name) != constTable.end();
}

bool SymbolTable::containsConstant(string& name) {
    return constTable.find(name) != constTable.end();
}

bool SymbolTable::checkRedefine(string& name) {
    if (nowFunction != NULL) {
        if (nowFunction->containsName(name)) {
            errorHandler->addErrorInfo(Redefine);
            return false;
        } else {
            return true;
        }
    }
    if (this->containsName(name)) {
        errorHandler->addErrorInfo(Redefine);
        return false;
    }
    return true;
}

bool SymbolTable::checkUndefine(string& name) {
    bool res = false;
    if (nowFunction != NULL) {
        res = (nowFunction->containsName(name));
    }
    if (!res && !this->containsName(name)) {
        errorHandler->addErrorInfo(Undefine);
        return false;
    }
    return true;
}

void SymbolTable::addParameter(VariableItem* item) {
    nowFunction->addParameter(item);
}

Sym SymbolTable::getType(string& name) {
    return getItem(name)->getType();
}

Item* FunctionItem::getItem(string& name) {
    for (int i = 0; i < (int)parameters.size(); i++) {
        Item* item = parameters[i];
        if (item->getName() == name) {
            return item;
        }
    }
    if (varTable.find(name) != varTable.end()) {
        return varTable[name];
    } else if (constTable.find(name) != constTable.end()) {
        return constTable[name];
    } else {
        return new Item();
    }
}

Item* SymbolTable::getItem(string& name) {
    if (nowFunction != NULL && nowFunction->containsName(name)) {
        return nowFunction->getItem(name);
    } else {
        if (varTable.find(name) != varTable.end()) {
            return varTable.at(name);
        } else if (constTable.find(name) != constTable.end()) {
            return constTable.at(name);
        } else if (funcTable.find(name) != funcTable.end()) {
            return funcTable.at(name);
        } else {
            return new Item();
        }
    }
}

bool SymbolTable::checkIsConstant(string& name) {
    bool pass = checkUndefine(name);
    if (pass) {
        bool res = false;
        if (nowFunction != NULL) {
            res = nowFunction->containsConstant(name);
        }
        if (res || this->containsConstant(name)) {
            errorHandler->addErrorInfo(ChangeConstant);
            return false;
        }
    }
    return true;
}
//bool SymbolTable::checkIsConstant(string& name) {
//    bool pass = checkUndefine(name);
//    if (pass) {
//        if (nowFunction != NULL) {
//            if (nowFunction->containsConstant(name)) {
//                errorHandler->addErrorInfo(ChangeConstant);
//                return false;
//            }
//            return true;
//        }
//        if (this->containsConstant(name)) {
//            errorHandler->addErrorInfo(ChangeConstant);
//            return false;
//        }
//    }
//    return true;
//}

int FunctionItem::checkParametes(vector<Sym> values) {
    if (values.size() != this->parameters.size()) {
        return (int)ParameterNumNOTMatch;
    }
    int length = (int)parameters.size();
    for (int i = 0; i < length; i++) {
        if (parameters[i]->getType() != values[i]) {
            return (int)ParameterTypeNOTMatch;
        }
    }
    return -1;
}

bool SymbolTable::checkParameters(string& funcName, vector<Sym>& values) {
    bool pass = checkUndefine(funcName);
    if (pass) {
        int res = funcTable[funcName]->checkParametes(values);
        if (res > 0) {
            errorHandler->addErrorInfo((ErrorType)res);
            return false;
        }
    }
    return true;
}

bool SymbolTable::checkReturnType(Sym type) {
    if (nowFunction != NULL) {
        int returnType = nowFunction->getType();
        if (type != returnType) {
            if (returnType == VOIDTK) {
                errorHandler->addErrorInfo(VoidFuncWithReturn);
            } else {
                errorHandler->addErrorInfo(ReturnError);
            }
            return false;
        }
    }
    return true;
}

bool SymbolTable::isLocalVariable(string varName) {
    if (varName.length() == 0) {
        return true;
    }
    if (isTempVariable(varName) || isNumberValue(varName) || isStringName(varName)) {
        return true;
    }
    if (nowFunction == NULL) {
        return false;
    }
    return (nowFunction->containsName(varName));
}