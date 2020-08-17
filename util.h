#pragma once
#ifndef util_H
#define util_H
#include <string>
#include "lexicalAnalysis.h"
#include "IntermediateCode.h"
#include "TargetCode.h"

//#define DEBUG

#define OPT_ALLOCATE_REG

#define OPT_INLINE

#define OPT_LEAF_FUNC

#define OPT_MOD

#define OPT_REDUNDANT_JUMP

using std::string;

bool isNumberValue(string& var);

bool isTempVariable(string var);

bool isStringName(string& stringName);

string sym2str(Sym sym);

IMOper str2imop(string str);

#endif // !util_H
