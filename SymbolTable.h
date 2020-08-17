#pragma once
#ifndef SymbolTable_H
#define SymbolTable_H

#include <string>
#include <map>
#include <vector>
#include "lexicalAnalysis.h"
#include "ErrorHandler.h"

using std::string;
using std::map;
using std::vector;

enum Kind {
    CONSTANT, VARIABLE, FUNCTION
};

class Item;
class VariableItem;
class ConstantItem;
class FunctionItem;

typedef map<string, ConstantItem*> ConstantTable;
typedef map<string, VariableItem*> VariableTable;
typedef map<string, FunctionItem*> FunctionTable;

//class ProgramBlock {
//public:
//    virtual bool containsName();
//    virtual void addItem(Item item);
//    virtual Item getItem(string& name);
//};

class Item {
private:
    string name;
    Sym type; // only VOIDTK, INTTK, CHARTK
    Kind kind;
public:
    Item(string name, Sym type, Kind kind);

    Item() {
        name = "";
        type = VOIDTK;
        kind = CONSTANT;
    }

    Item(const Item& item) {
        this->name = item.name;
        this->type = item.type;
        this->kind = item.kind;
    }

    ~Item() {}

    string getName() {
        return this->name;
    }

    Sym getType() {
        return this->type;
    }

    Kind getKind() {
        return this->kind;
    }

    bool isEmpty() {
        return (name == "");
    }
};

class VariableItem : public Item {
private:
    int arrayLength;
public:

    VariableItem(string name, Sym type, int length = 0);

    VariableItem() {
        arrayLength = 0;
    }

    ~VariableItem() {}

    int getArrayLength() {
        return this->arrayLength;
    }

    bool isArray() {
        return (this->arrayLength > 0);
    }
};

class ConstantItem : public Item {
public:
    int value;

    ConstantItem(string name, Sym type, int value);

    ConstantItem() {
        value = 0;
    }

    ~ConstantItem() {}

    int getValue() {
        return this->value;
    }
};

class FunctionItem : public Item {
public:
    vector<VariableItem*> parameters;
    ConstantTable constTable;
    VariableTable varTable;
    map<string, int> offsetTable;
    //int size;
    //void addConstant(ConstantItem& item);
    //void addVariable(VariableItem& item);

    FunctionItem(string name, Sym type);

    //FunctionItem() {}

    ~FunctionItem() {
        //for (auto i = 0; i < parameters.size(); i++) {
        //    delete parameters[i];
        //}
    }

    void addParameter(VariableItem* item);

    //int getSize() {
    //    return size;
    //}

    //map<string, int> getOffsetTable() {
    //    return offsetTable;
    //}

    void addItem(Item* item);
    Item* getItem(string& name);

    bool containsName(string& name);
    bool containsConstant(string& name);

    int checkParametes(vector<Sym> values);
};

class SymbolTable {
private:
    ConstantTable constTable;
    VariableTable varTable;
    FunctionTable funcTable;

    FunctionItem* nowFunction;

    ErrorHandler* errorHandler;

    void addConstant(ConstantItem* item) {
        this->constTable[item->getName()] = item;
        //this->constTable.insert(ConstantTable::value_type(item.getName(), item));
    }
    void addVariable(VariableItem* item) {
        this->varTable[item->getName()] = item;
    }
    void addFunction(FunctionItem* item) {
        this->funcTable[item->getName()] = item;
    }
public:
    SymbolTable() {
        this->nowFunction = NULL;
        this->errorHandler = NULL;
    }
    ~SymbolTable() {}

    void setErrorHandler(ErrorHandler* errorHandler) {
        this->errorHandler = errorHandler;
    }
    void setNowFunctionItem(FunctionItem* func) {
        this->nowFunction = func;
    }
    void setNowFunctionItem(string funcName) {
        this->nowFunction = funcTable[funcName];
    }
    void releaseNowFunctionItem() {
        this->nowFunction = NULL;
    }

    VariableTable& getVarTable() {
        return varTable;
    }

    int getParaCount(string funcName) {
        if (funcTable.find(funcName) != funcTable.end()) {
            return (int)funcTable.at(funcName)->parameters.size();
        } else {
            return 0;
        }
    }

    void addItem(Item* item);
    void addParameter(VariableItem* item);
    Item* getItem(string& name);
    Sym getType(string& name);

    bool isLocalVariable(string varName);
    bool containsName(string& name);
    bool containsConstant(string& name);
    bool checkRedefine(string& name);
    bool checkUndefine(string& name);
    bool checkIsConstant(string& name);
    bool checkParameters(string& funcName, vector<Sym>& values);
    bool checkReturnType(Sym type);
};

#endif // !SymbolTable_H