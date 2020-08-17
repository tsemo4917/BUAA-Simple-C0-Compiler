#pragma once
#ifndef ErrorHandler_H

#include <cstdio>
#include <vector>

using std::vector;

enum ErrorType {
    IllegalChar,                // a ok 非法符号或不符合词法
    Redefine,                   // b ok 名字重定义
    Undefine,                   // c ok 未定义的名字
    ParameterNumNOTMatch,       // d ok 函数参数个数不匹配
    ParameterTypeNOTMatch,      // e ok 函数参数类型不匹配
    IllegalTypeInCondition,     // f OK 条件判断中出现不合法的类型
    VoidFuncWithReturn,         // g OK 无返回值的函数存在不匹配的return语句
    ReturnError,                // h ok 有返回值的函数缺少return语句或存在不匹配的return语句
    SubscriptNotInt,            // i ok 数组元素的下标只能是整型表达式
    ChangeConstant,             // j OK
    SemicolonExpected,          // k OK
    RightParenthesisExpected,   // l ok
    RightBracketsExpected,      // m ok
    WhileExpected,              // n ok
    OnlyIntCharAssignConstant,  // o ok
    OtherError,
};

class ErrorHandler {
    class ErrorInfo {
    public:
        int lineAt;
        ErrorType errorType;
        ErrorInfo(int lineAt, ErrorType errorType) {
            this->lineAt = lineAt;
            this->errorType = errorType;
        }
        bool operator < (const ErrorInfo& e) const {
            return this->lineAt < e.lineAt;
        }
    };
public:
    FILE* errorFile;
    int errorLine;
    int prevSymbolLine;
    vector<ErrorInfo> infoList;

    ErrorHandler(FILE* file);
    ~ErrorHandler();
    bool isErrorOccured();
    void increaseErrorLine();
    void decreaseErrorLine();
    void error2File();
    void error2Console();
    void addErrorInfo(ErrorType errorType);
    void updatePrevSymbolLine(int lineNo);
};

#endif // !ErrorHandler_H