#pragma once

#include "ir.h"
#include "token.h"

#include <vector>
#include <string>
#include <stdexcept>
#include <cctype>
#include <map>

/// Lowers target-independent IR into ARM64 Token streams.
/// This is the "instruction selection" phase of the compiler pipeline.
class IRCodeGen {
public:
    static std::vector<Token> lower(const std::vector<IRInstruction> &ir) {
        std::vector<Token> tokens;
        for (auto &inst : ir) {
            lowerOne(inst, tokens);
            tokens.push_back({NEWLINE, ""});
        }
        return tokens;
    }

private:
    // ---------- helpers ----------

    static Token regToken(const std::string &s) {
        if (s == "xzr") return {ZREG, s};
        if (s == "sp")  return {ID, s};
        if (s.size() >= 2 && s[0] == 'x' && std::isdigit(static_cast<unsigned char>(s[1])))
            return {REG, s};
        throw std::runtime_error("IRCodeGen: expected register, got: " + s);
    }

    static Token immOrLabel(const std::string &s) {
        if (s.empty()) throw std::runtime_error("IRCodeGen: empty immediate/label");
        if (s[0] == '0' && s.size() > 2 && (s[1] == 'x' || s[1] == 'X'))
            return {HEXINT, s};
        // integer?
        bool isInt = true;
        size_t st = (s[0] == '-' || s[0] == '+') ? 1 : 0;
        if (st >= s.size()) isInt = false;
        else for (size_t i = st; i < s.size(); ++i)
            if (!std::isdigit(static_cast<unsigned char>(s[i]))) { isInt = false; break; }
        if (isInt) return {INT, s};
        return {ID, s};  // label reference
    }

    /// Map high-level comparison operators to ARM64 b.cond suffixes.
    static std::string condSuffix(const std::string &cond) {
        static const std::map<std::string, std::string> m = {
            {"==", ".eq"}, {"!=", ".ne"},
            {"<",  ".lt"}, {"<=", ".le"},
            {">",  ".gt"}, {">=", ".ge"},
        };
        auto it = m.find(cond);
        if (it == m.end())
            throw std::runtime_error("IRCodeGen: unknown condition: " + cond);
        return it->second;
    }

    static void emit3Reg(const std::string &instr, const std::string &a,
                         const std::string &b, const std::string &c,
                         std::vector<Token> &out) {
        out.push_back({ID, instr});
        out.push_back(regToken(a));
        out.push_back({COMMA, ","});
        out.push_back(regToken(b));
        out.push_back({COMMA, ","});
        out.push_back(regToken(c));
    }

    // ---------- lowering dispatch ----------

    static void lowerOne(const IRInstruction &inst, std::vector<Token> &out) {
        switch (inst.op) {
            case IRInstruction::LABEL:
                out.push_back({LABEL, inst.dst + ":"});
                break;

            case IRInstruction::ADD:
                emit3Reg("add", inst.dst, inst.src1, inst.src2, out);
                break;

            case IRInstruction::SUB:
                emit3Reg("sub", inst.dst, inst.src1, inst.src2, out);
                break;

            case IRInstruction::MUL:
                emit3Reg("mul", inst.dst, inst.src1, inst.src2, out);
                break;

            case IRInstruction::DIV:
                emit3Reg("sdiv", inst.dst, inst.src1, inst.src2, out);
                break;

            case IRInstruction::MOD:
                // dst = src1 % src2
                //   sdiv dst, src1, src2
                //   mul  dst, dst, src2
                //   sub  dst, src1, dst
                emit3Reg("sdiv", inst.dst, inst.src1, inst.src2, out);
                out.push_back({NEWLINE, ""});
                emit3Reg("mul", inst.dst, inst.dst, inst.src2, out);
                out.push_back({NEWLINE, ""});
                emit3Reg("sub", inst.dst, inst.src1, inst.dst, out);
                break;

            case IRInstruction::MOV:
                // add dst, src1, xzr
                emit3Reg("add", inst.dst, inst.src1, "xzr", out);
                break;

            case IRInstruction::LOAD:
                // ldur dst, [src1, imm]
                out.push_back({ID, "ldur"});
                out.push_back(regToken(inst.dst));
                out.push_back({COMMA, ","});
                out.push_back({LBRACK, "["});
                out.push_back(regToken(inst.src1));
                out.push_back({COMMA, ","});
                out.push_back({INT, inst.imm});
                out.push_back({RBRACK, "]"});
                break;

            case IRInstruction::STORE:
                // stur src1, [dst, imm]
                out.push_back({ID, "stur"});
                out.push_back(regToken(inst.src1));
                out.push_back({COMMA, ","});
                out.push_back({LBRACK, "["});
                out.push_back(regToken(inst.dst));
                out.push_back({COMMA, ","});
                out.push_back({INT, inst.imm});
                out.push_back({RBRACK, "]"});
                break;

            case IRInstruction::CMP_BRANCH:
                // cmp src1, src2
                out.push_back({ID, "cmp"});
                out.push_back(regToken(inst.src1));
                out.push_back({COMMA, ","});
                out.push_back(regToken(inst.src2));
                out.push_back({NEWLINE, ""});
                // b .cond label
                out.push_back({ID, "b"});
                out.push_back({DOTID, condSuffix(inst.cond)});
                out.push_back(immOrLabel(inst.label));
                break;

            case IRInstruction::BRANCH:
                out.push_back({ID, "b"});
                out.push_back(immOrLabel(inst.label));
                break;

            case IRInstruction::CALL:
                out.push_back({ID, "blr"});
                out.push_back(regToken(inst.src1));
                break;

            case IRInstruction::RET:
                out.push_back({ID, "br"});
                out.push_back({REG, "x30"});
                break;

            case IRInstruction::DATA8:
                out.push_back({DOTID, ".8byte"});
                out.push_back(immOrLabel(inst.imm));
                break;
        }
    }
};
