#include "TargetCode.h"
#include <map>
#include <sstream>
#include <set>

using std::map;
using std::set;
using std::ostringstream;

string reg2str(mipsReg reg) {
    const static string reg2strArray[] = {
        "$0", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3",
        "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
        "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
        "$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"
    };
    return reg2strArray[(int)reg];
    //return "$" + std::to_string(int(reg));
}

string TargetCode::toString() {
    static map<mipsInstr, string> instr2str = {
    {instr_addiu, "addiu"}, {instr_sll, "sll"}, {instr_slti,"slti"}, {instr_sra, "sra"},
    {instr_addu, "addu"}, {instr_subu, "subu"}, {instr_mul, "mul"}, {instr_div, "div"},
    {instr_mflo, "mflo"}, {instr_mfhi, "mfhi"},
    {instr_move, "move"},
    {instr_slt, "slt"}, {instr_seq, "seq"}, {instr_sne, "sne"}, {instr_sgt, "sgt"}, {instr_sge, "sge"}, {instr_sle, "sle"},
    {instr_move, "move"},
    {instr_sw, "sw"}, {instr_sb, "sb"},
    {instr_lw, "lw"}, {instr_lb, "lb"}, 
    {instr_li, "li"}, {instr_la, "la"},
    {instr_j, "j"}, {instr_jal, "jal"}, {instr_jr, "jr"},
    {instr_beq, "beq"}, {instr_bne, "bne"}, /*{instr_beqz, "beqz"}, {instr_bnez, "bnez"},*/
    {instr_blt, "blt"}, {instr_ble, "ble"},
    {instr_bgt, "bgt"}, {instr_bge, "bge"},
    {instr_label, ""}, {instr_note, "#"}, {instr_syscall, "syscall"}
    };
    ostringstream ostr;
    if (instr != instr_label) {
        ostr << "\t" << instr2str[instr] << "\t";
    }
    switch (instr) {
    case instr_addiu: case instr_sll: case instr_slti: case instr_sra:
    case instr_addu: case instr_subu: case instr_mul: 
    case instr_slt: case instr_seq: case instr_sne: 
    case instr_sgt: case instr_sge: case instr_sle: {
        ostr << result << "\t" << var1 << "\t" << var2;
        break;
    }
    case instr_move:
        ostr << result << "\t" << var1;
        break;
    /**********************************************************/
    case instr_div: 
        ostr << var1 << "\t" << var2;
        break;
    /**********************************************************/
    case instr_sw: case instr_sb:
    case instr_lw:  case instr_lb:{
        ostr << result << "\t" << var1 << "(" << var2 << ")";
        break;
    }
    case instr_li: case instr_la:
        ostr << result << "\t" << var1;
        break;
    /**********************************************************/
    case instr_mflo: case instr_mfhi:
        ostr << result;
        break;
    case instr_j: case instr_jal: case instr_jr:
        ostr << result;
        break;
    case instr_beq: case instr_bne:
    case instr_blt: case instr_ble:
    case instr_bgt: case instr_bge:
    /*case instr_beqz: case instr_bnez: */{
        ostr << var1 << "\t";
        if (var2.length() == 0) {
            ostr << reg2str(reg_0);
        } else {
            ostr << var2;
        }
        ostr << "\t" << result;
        break;
    }
    /**********************************************************/
    case instr_label: {
        ostr << result << ":";
        break;
    }
    case instr_note: {
        ostr << result;
        break;
    }
    case instr_syscall:
        break;
    default:
        break;
    }
    return ostr.str();
}

bool TargetCode::isCalInstr() {
    static set<mipsInstr> calInstrSet = {
        // I_instr
        instr_addiu, instr_sll, instr_slti,
        // R_instr
        instr_addu, instr_subu, instr_mul, /*instr_div,*/
        instr_mflo, instr_mfhi,
        instr_move,
        instr_slt, instr_seq, instr_sne, instr_sgt, instr_sge, instr_sle,
    };
    return (calInstrSet.find(instr) != calInstrSet.end());
}

bool TargetCode::isLoadInstr() {
    return (instr == instr_lw || instr == instr_lb);
}

bool TargetCode::isStoreInstr() {
    return (instr == instr_sw || instr == instr_sb);
}