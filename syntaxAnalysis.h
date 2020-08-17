#pragma once
#ifndef syntaxAnalysis_H
#define syntaxAnalysis_H
#include <vector>
#include "IntermediateCode.h"

void isPrograme();

//＜字符串＞   ::=  "｛十进制编码为32,33,35-126的ASCII字符｝"
void isString();

//＜常量说明＞ ::=  const＜常量定义＞;{ const＜常量定义＞;}
void constantExplanation();

/*
＜常量定义＞ ::= int＜标识符＞＝＜整数＞{,＜标识符＞＝＜整数＞}
                 | char＜标识符＞＝＜字符＞{ ,＜标识符＞＝＜字符＞ }
*/
void constantDefinition();

//＜变量说明＞  ::= ＜变量定义＞;{＜变量定义＞;}
void variableExplanation();
/*
＜变量定义＞  ::= ＜类型标识符＞(＜标识符＞|＜标识符＞'['＜无符号整数＞']'){,(＜标识符＞|＜标识符＞'['＜无符号整数＞']' )}
                 //＜无符号整数＞表示数组元素的个数，其值需大于0
*/
bool variableDefinition();

//＜无符号整数＞  ::= ＜非零数字＞｛＜数字＞｝| 0
bool isUnsignedInteger();

//＜整数＞::= ［＋｜－］＜无符号整数＞
bool isInteger(long& value);

//＜有返回值函数定义＞  ::=  ＜声明头部＞'('＜参数表＞')' '{'＜复合语句＞'}'
void functionWithReturnDef();
//＜无返回值函数定义＞  ::= void＜标识符＞'('＜参数表＞')''{'＜复合语句＞'}'
void functionVoidDef();
//＜复合语句＞::=［＜常量说明＞］［＜变量说明＞］＜语句列＞
bool compoundStatement();

//＜参数表＞::=  ＜类型标识符＞＜标识符＞{,＜类型标识符＞＜标识符＞}| ＜空＞
void parameterTable();
//＜主函数＞    ::= void main‘(’‘)’ ‘{’＜复合语句＞‘}’
void mainFunction();

////＜表达式＞ ::=［＋｜－］＜项＞{＜加法运算符＞＜项＞}   //[+|-]只作用于第一个<项>
//Sym expression(string& resultVar);
//
////＜项＞     ::= ＜因子＞{＜乘法运算符＞＜因子＞}
//Sym term(string& resultVar);
//
////＜因子＞::= ＜标识符＞｜＜标识符＞'['＜表达式＞']'|'('＜表达式＞')'｜＜整数＞|＜字符＞｜＜有返回值函数调用语句＞
//Sym factor(string& resultVar);
//＜表达式＞ ::=［＋｜－］＜项＞{＜加法运算符＞＜项＞}   //[+|-]只作用于第一个<项>
Sym expression();

//＜项＞     ::= ＜因子＞{＜乘法运算符＞＜因子＞}
Sym term();

//＜因子＞::= ＜标识符＞｜＜标识符＞'['＜表达式＞']'|'('＜表达式＞')'｜＜整数＞|＜字符＞｜＜有返回值函数调用语句＞
Sym factor();

//＜赋值语句＞   ::=  ＜标识符＞＝＜表达式＞|＜标识符＞'['＜表达式＞']'=＜表达式＞
void assignmentStatement(string name);

//＜条件语句＞  ::= if '('＜条件＞')'＜语句＞［else＜语句＞］
bool conditionalStatement();
/*
＜条件＞::=  ＜表达式＞＜关系运算符＞＜表达式＞ //整型表达式之间才能进行关系运算
            ｜＜表达式＞    //表达式为整型，其值为0条件为假，值不为0时条件为真
*/
void isCondition();

/*
＜循环语句＞   ::=  while '('＜条件＞')'＜语句＞
                | do＜语句＞while '('＜条件＞')'
                |for'('＜标识符＞＝＜表达式＞;＜条件＞;＜标识符＞＝＜标识符＞(+|-)＜步长＞')'＜语句＞
*/
bool loopStatement();

void functionCall(string funcName);
/*
＜语句＞    ::= ＜条件语句＞｜＜循环语句＞| '{'＜语句列＞'}'| ＜有返回值函数调用语句＞;
                |＜无返回值函数调用语句＞;｜＜赋值语句＞;｜＜读语句＞;｜＜写语句＞;｜＜空＞;|＜返回语句＞;
*/
bool statement();

//＜值参数表＞   ::= ＜表达式＞{,＜表达式＞}｜＜空＞
std::vector<Sym> valueParameterTable(string funcName);

//＜语句列＞   ::= ｛＜语句＞｝
bool statementCloumn();

/*
＜读语句＞    ::=  scanf '('＜标识符＞{,＜标识符＞}')'
FSM:
    scanf-->[s1]--'('-->[s2]--IDENFR-->[s3]--')'-->[s4]-->END
                          |				|
                          |---<<----','-|
*/
void scanfStatement();

//＜写语句＞::= printf '(' ＜字符串＞,＜表达式＞ ')'| printf '('＜字符串＞ ')'| printf '('＜表达式＞')'
void printfStatement();

//＜返回语句＞::=  return['('＜表达式＞')']
void returnStatement();
#endif // !syntaxAnalysis_H
