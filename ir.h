#pragma once

#include <string>
#include <vector>
#include <iostream>

/// Intermediate Representation for high-level statements.
/// Each IRInstruction is a target-independent operation that
/// is later lowered to ARM64 tokens by IRCodeGen.
struct IRInstruction {
    enum Op {
        ADD,            // dst = src1 + src2
        SUB,            // dst = src1 - src2
        MUL,            // dst = src1 * src2
        DIV,            // dst = src1 / src2
        MOD,            // dst = src1 % src2
        MOV,            // dst = src1
        LOAD,           // dst = *(src1 + imm)
        STORE,          // *(dst + imm) = src1
        CMP_BRANCH,     // if src1 <cond> src2 goto label
        BRANCH,         // goto label
        CALL,           // call src1
        RET,            // return (br x30)
        LABEL,          // label definition
        DATA8,          // .8byte value
    };

    Op op;
    std::string dst;        // destination register or label name (for LABEL)
    std::string src1;       // first source register
    std::string src2;       // second source register
    std::string label;      // target label (for branches)
    std::string cond;       // condition (==, !=, <, <=, >, >=)
    std::string imm;        // immediate value (for LOAD/STORE offset, DATA8 value)
};

inline std::string irOpToString(IRInstruction::Op op) {
    switch (op) {
        case IRInstruction::ADD:        return "ADD";
        case IRInstruction::SUB:        return "SUB";
        case IRInstruction::MUL:        return "MUL";
        case IRInstruction::DIV:        return "DIV";
        case IRInstruction::MOD:        return "MOD";
        case IRInstruction::MOV:        return "MOV";
        case IRInstruction::LOAD:       return "LOAD";
        case IRInstruction::STORE:      return "STORE";
        case IRInstruction::CMP_BRANCH: return "CMP_BRANCH";
        case IRInstruction::BRANCH:     return "BRANCH";
        case IRInstruction::CALL:       return "CALL";
        case IRInstruction::RET:        return "RET";
        case IRInstruction::LABEL:      return "LABEL";
        case IRInstruction::DATA8:      return "DATA8";
    }
    return "???";
}

/// Dump IR to a stream in a human-readable format.
inline void dumpIR(const std::vector<IRInstruction> &ir, std::ostream &out) {
    for (auto &i : ir) {
        switch (i.op) {
            case IRInstruction::LABEL:
                out << i.dst << ":\n";
                break;
            case IRInstruction::ADD:
            case IRInstruction::SUB:
            case IRInstruction::MUL:
            case IRInstruction::DIV:
            case IRInstruction::MOD:
                out << "  " << irOpToString(i.op) << " " << i.dst
                    << ", " << i.src1 << ", " << i.src2 << "\n";
                break;
            case IRInstruction::MOV:
                out << "  MOV " << i.dst << ", " << i.src1 << "\n";
                break;
            case IRInstruction::LOAD:
                out << "  LOAD " << i.dst << ", [" << i.src1 << " + " << i.imm << "]\n";
                break;
            case IRInstruction::STORE:
                out << "  STORE [" << i.dst << " + " << i.imm << "], " << i.src1 << "\n";
                break;
            case IRInstruction::CMP_BRANCH:
                out << "  CMP_BRANCH " << i.src1 << " " << i.cond
                    << " " << i.src2 << ", " << i.label << "\n";
                break;
            case IRInstruction::BRANCH:
                out << "  BRANCH " << i.label << "\n";
                break;
            case IRInstruction::CALL:
                out << "  CALL " << i.src1 << "\n";
                break;
            case IRInstruction::RET:
                out << "  RET\n";
                break;
            case IRInstruction::DATA8:
                out << "  DATA8 " << i.imm << "\n";
                break;
        }
    }
}
