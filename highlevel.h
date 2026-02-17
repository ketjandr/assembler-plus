#pragma once

#include "ir.h"

#include <vector>
#include <string>
#include <sstream>
#include <istream>
#include <cctype>
#include <algorithm>
#include <stdexcept>

/// Parses high-level pseudocode into a target-independent IR.
///
/// Supported syntax (one statement per line):
///
///   label <name>                         →  LABEL
///   <xd> = <xn> + <xm>                  →  ADD
///   <xd> = <xn> - <xm>                  →  SUB
///   <xd> = <xn> * <xm>                  →  MUL
///   <xd> = <xn> / <xm>                  →  DIV
///   <xd> = <xn> % <xm>                  →  MOD
///   <xd> = <xn>                          →  MOV
///   <xd> = *<xn>                         →  LOAD (offset 0)
///   <xd> = *(<xn> + <imm>)              →  LOAD
///   *<xn> = <xd>                         →  STORE (offset 0)
///   *(<xn> + <imm>) = <xd>              →  STORE
///   if <xn> <cmp> <xm> goto <label>     →  CMP_BRANCH
///   goto <label>                         →  BRANCH
///   call <xn>                            →  CALL
///   ret                                  →  RET
///   .8byte <val>                         →  DATA8
///
///  Lines starting with # are comments.
///
class HighLevelParser {
public:
    static std::vector<IRInstruction> parse(std::istream &in) {
        std::vector<IRInstruction> ir;
        std::string line;
        while (std::getline(in, line)) {
            line = strip(line);
            if (line.empty() || line[0] == '#') continue;
            parseLine(line, ir);
        }
        return ir;
    }

private:
    static std::string strip(const std::string &s) {
        size_t a = 0, b = s.size();
        while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
        while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
        return s.substr(a, b - a);
    }

    static std::vector<std::string> split(const std::string &s) {
        std::vector<std::string> out;
        std::istringstream ss(s);
        std::string w;
        while (ss >> w) out.push_back(w);
        return out;
    }

    static bool isInteger(const std::string &s) {
        if (s.empty()) return false;
        size_t st = (s[0] == '-' || s[0] == '+') ? 1 : 0;
        if (st >= s.size()) return false;
        return std::all_of(s.begin() + st, s.end(),
                           [](unsigned char c){ return std::isdigit(c); });
    }

    static bool isReg(const std::string &s) {
        if (s == "xzr" || s == "sp") return true;
        return s.size() >= 2 && s[0] == 'x' && std::isdigit(static_cast<unsigned char>(s[1]));
    }

    // ---------- line dispatcher ----------

    static void parseLine(const std::string &line, std::vector<IRInstruction> &ir) {
        auto words = split(line);
        if (words.empty()) return;

        // label <name>
        if (words[0] == "label") {
            if (words.size() < 2) throw std::runtime_error("label requires a name");
            ir.push_back({IRInstruction::LABEL, words[1], {}, {}, {}, {}, {}});
            return;
        }

        // goto <label>
        if (words[0] == "goto") {
            if (words.size() < 2) throw std::runtime_error("goto requires a label");
            ir.push_back({IRInstruction::BRANCH, {}, {}, {}, words[1], {}, {}});
            return;
        }

        // call <reg>
        if (words[0] == "call") {
            if (words.size() < 2) throw std::runtime_error("call requires a register");
            ir.push_back({IRInstruction::CALL, {}, words[1], {}, {}, {}, {}});
            return;
        }

        // ret
        if (words[0] == "ret") {
            ir.push_back({IRInstruction::RET, {}, {}, {}, {}, {}, {}});
            return;
        }

        // .8byte <val>
        if (words[0] == ".8byte") {
            if (words.size() < 2) throw std::runtime_error(".8byte requires a value");
            ir.push_back({IRInstruction::DATA8, {}, {}, {}, {}, {}, words[1]});
            return;
        }

        // if <xn> <op> <xm> goto <label>
        if (words[0] == "if") {
            if (words.size() < 6 || words[4] != "goto")
                throw std::runtime_error("Syntax: if <reg> <op> <reg> goto <label>");
            ir.push_back({IRInstruction::CMP_BRANCH, {}, words[1], words[3], words[5], words[2], {}});
            return;
        }

        // --- assignment forms ---
        auto eqPos = std::find(words.begin(), words.end(), "=");
        if (eqPos == words.end())
            throw std::runtime_error("Unrecognized statement: " + line);
        size_t eqIdx = eqPos - words.begin();

        // *xn = xd  or  *(xn + imm) = xd   → STORE
        if (words[0][0] == '*') {
            parseStore(words, eqIdx, ir);
            return;
        }

        if (eqIdx != 1)
            throw std::runtime_error("Expected 'register = ...': " + line);

        std::string dest = words[0];
        auto rhs = std::vector<std::string>(words.begin() + 2, words.end());

        // xd = *xn  or  xd = *(xn + imm)  → LOAD
        if (!rhs.empty() && rhs[0][0] == '*') {
            parseLoad(dest, rhs, ir);
            return;
        }

        // xd = xn op xm
        if (rhs.size() == 3 && isReg(rhs[0]) && isReg(rhs[2])) {
            IRInstruction::Op op;
            if      (rhs[1] == "+") op = IRInstruction::ADD;
            else if (rhs[1] == "-") op = IRInstruction::SUB;
            else if (rhs[1] == "*") op = IRInstruction::MUL;
            else if (rhs[1] == "/") op = IRInstruction::DIV;
            else if (rhs[1] == "%") op = IRInstruction::MOD;
            else throw std::runtime_error("Unknown operator: " + rhs[1]);
            ir.push_back({op, dest, rhs[0], rhs[2], {}, {}, {}});
            return;
        }

        // xd = xn  (move)
        if (rhs.size() == 1 && isReg(rhs[0])) {
            ir.push_back({IRInstruction::MOV, dest, rhs[0], {}, {}, {}, {}});
            return;
        }

        throw std::runtime_error("Unrecognized assignment: " + line);
    }

    // ---------- load ----------

    static void parseLoad(const std::string &dest,
                          const std::vector<std::string> &rhs,
                          std::vector<IRInstruction> &ir) {
        std::string first = rhs[0].substr(1); // strip '*'
        std::string base, offset = "0";

        if (first.empty() || first[0] == '(') {
            std::string combined = first;
            for (size_t i = 1; i < rhs.size(); ++i) combined += " " + rhs[i];
            if (combined.front() == '(') combined = combined.substr(1);
            if (combined.back() == ')') combined.pop_back();

            auto parts = split(combined);
            if (parts.size() == 1) {
                base = parts[0]; offset = "0";
            } else if (parts.size() == 3 && parts[1] == "+") {
                base = parts[0]; offset = parts[2];
            } else {
                throw std::runtime_error("Bad load syntax");
            }
        } else {
            base = first;
        }

        ir.push_back({IRInstruction::LOAD, dest, base, {}, {}, {}, offset});
    }

    // ---------- store ----------

    static void parseStore(const std::vector<std::string> &words, size_t eqIdx,
                           std::vector<IRInstruction> &ir) {
        std::string lhs;
        for (size_t i = 0; i < eqIdx; ++i) {
            if (i) lhs += " ";
            lhs += words[i];
        }
        if (eqIdx + 1 >= words.size())
            throw std::runtime_error("Missing value in store");
        std::string val = words[eqIdx + 1];

        lhs = lhs.substr(1); // strip '*'
        std::string base, offset = "0";
        if (lhs.empty() || lhs[0] == '(') {
            if (lhs.front() == '(') lhs = lhs.substr(1);
            if (lhs.back() == ')') lhs.pop_back();
            auto parts = split(lhs);
            if (parts.size() == 1) { base = parts[0]; offset = "0"; }
            else if (parts.size() == 3 && parts[1] == "+") { base = parts[0]; offset = parts[2]; }
            else throw std::runtime_error("Bad store syntax");
        } else {
            base = lhs;
        }

        ir.push_back({IRInstruction::STORE, base, val, {}, {}, {}, offset});
    }
};
