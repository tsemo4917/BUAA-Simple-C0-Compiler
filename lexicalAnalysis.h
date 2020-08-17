#pragma once
#ifndef lexicalAnalysis_H
#define lexicalAnalysis_H

#include <map>
#include <string>

using std::map;
using std::string;

enum Sym {
    IDENFR, INTCON, CHARCON, STRCON,
    // reserved words
    CONSTTK, INTTK, CHARTK, VOIDTK, MAINTK, IFTK, ELSETK, DOTK, WHILETK, FORTK, SCANFTK, PRINTFTK, RETURNTK,
    PLUS, MINU, MULT, DIV, LSS, LEQ, GRE, GEQ, EQL, NEQ, ASSIGN, SEMICN, COMMA,
    LPARENT, RPARENT, LBRACK, RBRACK, LBRACE, RBRACE
};

//static void getCharacter();
//static int isLetter();
//static int isDigit();
//static int isGreater();
//static int isLess();
//static int isNot();
//static int isEqual();
//static int isPlus();
//static int isMinus();
//static int isDivi();
//static int isMult();
//static int isSingleQuotation();
//static int isDoubleQuotation();
//static void catTokenBuffer();
//static void retract();
//static void dealWithCompare(Sym equ, Sym unequ);

void error();
bool getsym();

bool nextSym();
int getPrevSymLine();
void backtrace(int n);
#endif // lexicalAnalysis_H