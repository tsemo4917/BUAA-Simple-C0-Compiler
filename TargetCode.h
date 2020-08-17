#pragma once
#ifndef TargetCode_H
#include <string>

using std::string;

enum mipsReg {
    reg_0, reg_at, reg_v0, reg_v1, reg_a0, reg_a1, reg_a2, reg_a3,
    reg_t0, reg_t1, reg_t2, reg_t3, reg_t4, reg_t5, reg_t6, reg_t7,
    reg_s0, reg_s1, reg_s2, reg_s3, reg_s4, reg_s5, reg_s6, reg_s7,
    reg_t8, reg_t9, reg_k0, reg_k1, reg_gp, reg_sp, reg_fp, reg_ra
};

string reg2str(mipsReg reg);

enum mipsInstr {
    // I_instr
    instr_addiu, instr_sll, instr_slti, instr_sra, 
    // R_instr
    instr_addu, instr_subu, instr_mul, instr_div,
    instr_mflo, instr_mfhi,
    instr_move,
    instr_slt, instr_seq, instr_sne, instr_sgt, instr_sge, instr_sle,
    // load/store
    instr_sw, instr_sb,
    instr_lw, instr_lb, 
    instr_li, instr_la, 
    // jump/branch
    instr_j, instr_jal, instr_jr,
    instr_beq, instr_bne, /*instr_beqz, instr_bnez, */
    instr_blt, instr_ble, 
    instr_bgt, instr_bge, 
    // set label
    instr_label,
    // add notes
    instr_note,
    instr_syscall,
};

class TargetCode {
public:
    mipsInstr instr;
    string result;
    string var1;
    string var2;

    TargetCode(mipsInstr instr, string result, string var1, string var2) {
        this->instr = instr;
        this->result = result;
        this->var1 = var1;
        this->var2 = var2;
    }

    bool isCalInstr();

    bool isLoadInstr();

    bool isStoreInstr();

    string toString();
};

#endif // !TargetCode_H