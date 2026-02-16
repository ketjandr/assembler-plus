#pragma once

#include "token.h"

#include <vector>
#include <string>
#include <sstream>
#include <istream>
#include <cctype>
#include <algorithm>
#include <stdexcept>
#include <set>

/// Parses a high-level pseudocode syntax and lowers it into the standard
/// Token stream consumed by the Assembler.
///
/// Supported syntax (one statement per line):
///
///   label <name>                         →  LABEL  <name>:
///   <xd> = <xn> + <xm>                  →  add xd, xn, xm
///   <xd> = <xn> - <xm>                  →  sub xd, xn, xm
///   <xd> = <xn> * <xm>                  →  mul xd, xn, xm
///   <xd> = <xn> / <xm>                  →  sdiv xd, xn, xm
///   <xd> = <xn> % <xm>                  →  msub (xd = xn - (xn/xm)*xm)
///   <xd> = *<xn>                         →  ldur xd, [xn, 0]
///   <xd> = *(<xn> + <imm>)              →  ldur xd, [xn, imm]
///   *<xn> = <xd>                         →  stur xd, [xn, 0]
///   *(<xn> + <imm>) = <xd>              →  stur xd, [xn, imm]
///   if <xn> <cmp> <xm> goto <label>     →  cmp xn, xm ; b.<cond> label
///   goto <label>                         →  b label
///   call <xn>                            →  blr xn
///   ret                                  →  br x30
///   .8byte <val>                         →  .8byte val
///
///  Lines starting with # are comments.
///
class HighLevelParser {
public:
    static std::vector<Token> parse(std::istream &in) {
        std::vector<Token> tokens;
        std::string line;
        while (std::getline(in, line)) {
            line = strip(line);
            if (line.empty() || line[0] == '#') {
                tokens.push_back({NEWLINE, ""});
                continue;
            }
            parseLine(line, tokens);
            tokens.push_back({NEWLINE, ""});
        }
        return tokens;
    }

private:
    // ---------- helpers ----------

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

    static Token regToken(const std::string &s) {
        if (s == "xzr") return {ZREG, s};
        if (s == "sp")  return {ID, s};
        if (s.size() >= 2 && s[0] == 'x' && std::isdigit(static_cast<unsigned char>(s[1])))
            return {REG, s};
        throw std::runtime_error("Expected register, got: " + s);
    }

    static Token immOrLabel(const std::string &s) {
        if (s.empty()) throw std::runtime_error("Empty immediate/label");
        if (s[0] == '0' && s.size() > 2 && (s[1] == 'x' || s[1] == 'X'))
            return {HEXINT, s};
        if (isInteger(s)) return {INT, s};
        return {ID, s};  // label reference
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

    // Map comparison operators to b.cond suffixes
    static std::string condSuffix(const std::string &op) {
        if (op == "==") return ".eq";
        if (op == "!=") return ".ne";
        if (op == "<")  return ".lt";
        if (op == "<=") return ".le";
        if (op == ">")  return ".gt";
        if (op == ">=") return ".ge";
        throw std::runtime_error("Unknown comparison operator: " + op);
    }

    // ---------- line dispatcher ----------

    static void parseLine(const std::string &line, std::vector<Token> &out) {
        auto words = split(line);
        if (words.empty()) return;

        // label <name>
        if (words[0] == "label") {
            if (words.size() < 2) throw std::runtime_error("label requires a name");
            out.push_back({LABEL, words[1] + ":"});
            return;
        }

        // goto <label>
        if (words[0] == "goto") {
            if (words.size() < 2) throw std::runtime_error("goto requires a label");
            out.push_back({ID, "b"});
            out.push_back(immOrLabel(words[1]));
            return;
        }

        // call <reg>
        if (words[0] == "call") {
            if (words.size() < 2) throw std::runtime_error("call requires a register");
            out.push_back({ID, "blr"});
            out.push_back(regToken(words[1]));
            return;
        }

        // ret
        if (words[0] == "ret") {
            out.push_back({ID, "br"});
            out.push_back({REG, "x30"});
            return;
        }

        // .8byte <val>
        if (words[0] == ".8byte") {
            if (words.size() < 2) throw std::runtime_error(".8byte requires a value");
            out.push_back({DOTID, ".8byte"});
            out.push_back(immOrLabel(words[1]));
            return;
        }

        // if <xn> <op> <xm> goto <label>
        if (words[0] == "if") {
            // if xn op xm goto label
            if (words.size() < 6 || words[4] != "goto")
                throw std::runtime_error("Syntax: if <reg> <op> <reg> goto <label>");
            std::string rn = words[1], op = words[2], rm = words[3], lbl = words[5];
            // emit:  cmp rn, rm  NEWLINE  b .cond label
            out.push_back({ID, "cmp"});
            out.push_back(regToken(rn));
            out.push_back({COMMA, ","});
            out.push_back(regToken(rm));
            out.push_back({NEWLINE, ""});
            out.push_back({ID, "b"});
            out.push_back({DOTID, condSuffix(op)});
            out.push_back(immOrLabel(lbl));
            return;
        }

        // --- assignment forms ---
        // Look for '='
        auto eqPos = std::find(words.begin(), words.end(), "=");
        if (eqPos == words.end())
            throw std::runtime_error("Unrecognized statement: " + line);

        size_t eqIdx = eqPos - words.begin();

        // *xn = xd  or  *(xn + imm) = xd   → stur
        if (words[0][0] == '*') {
            parseStore(words, eqIdx, out);
            return;
        }

        // xd = ...
        if (eqIdx != 1)
            throw std::runtime_error("Expected 'register = ...': " + line);

        std::string dest = words[0];
        auto rhs = std::vector<std::string>(words.begin() + 2, words.end());

        // xd = *xn  or  xd = *(xn + imm)  → ldur
        if (!rhs.empty() && rhs[0][0] == '*') {
            parseLoad(dest, rhs, out);
            return;
        }

        // xd = xn op xm
        if (rhs.size() == 3 && isReg(rhs[0]) && isReg(rhs[2])) {
            parseArith(dest, rhs[0], rhs[1], rhs[2], out);
            return;
        }

        // xd = xn  (move, encoded as add xd, xn, xzr)
        if (rhs.size() == 1 && isReg(rhs[0])) {
            out.push_back({ID, "add"});
            out.push_back(regToken(dest));
            out.push_back({COMMA, ","});
            out.push_back(regToken(rhs[0]));
            out.push_back({COMMA, ","});
            out.push_back({ZREG, "xzr"});
            return;
        }

        throw std::runtime_error("Unrecognized assignment: " + line);
    }

    // ---------- arithmetic ----------

    static void parseArith(const std::string &dst, const std::string &lhs,
                           const std::string &op, const std::string &rhs,
                           std::vector<Token> &out) {
        std::string instr;
        if      (op == "+") instr = "add";
        else if (op == "-") instr = "sub";
        else if (op == "*") instr = "mul";
        else if (op == "/") instr = "sdiv";
        else if (op == "%") {
            // modulo: xd = lhs - (lhs / rhs) * rhs
            // We need a temp register.  We'll use xd itself as the temp.
            //   sdiv xd, lhs, rhs
            //   mul  xd, xd, rhs      (using msub would be ideal, but not in ISA subset)
            //   sub  xd, lhs, xd
            emitInstr("sdiv", dst, lhs, rhs, out);
            out.push_back({NEWLINE, ""});
            emitInstr("mul", dst, dst, rhs, out);
            out.push_back({NEWLINE, ""});
            emitInstr("sub", dst, lhs, dst, out);
            return;
        }
        else throw std::runtime_error("Unknown operator: " + op);

        emitInstr(instr, dst, lhs, rhs, out);
    }

    static void emitInstr(const std::string &instr,
                          const std::string &a, const std::string &b,
                          const std::string &c, std::vector<Token> &out) {
        out.push_back({ID, instr});
        out.push_back(regToken(a));
        out.push_back({COMMA, ","});
        out.push_back(regToken(b));
        out.push_back({COMMA, ","});
        out.push_back(regToken(c));
    }

    // ---------- load ----------

    static void parseLoad(const std::string &dest,
                          const std::vector<std::string> &rhs,
                          std::vector<Token> &out) {
        // rhs[0] starts with '*'
        // Form 1: *xn          → ldur dest, [xn, 0]
        // Form 2: *(xn         (followed by + imm) )  → ldur dest, [xn, imm]
        std::string first = rhs[0].substr(1); // strip '*'

        std::string base;
        std::string offset = "0";

        if (first.empty() || first[0] == '(') {
            // *(xn + imm)  — words might be:  *(xn  +  imm)
            // or the parens might be attached differently
            std::string combined;
            combined = first;
            for (size_t i = 1; i < rhs.size(); ++i) combined += " " + rhs[i];
            // strip parens
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

        out.push_back({ID, "ldur"});
        out.push_back(regToken(dest));
        out.push_back({COMMA, ","});
        out.push_back({LBRACK, "["});
        out.push_back(regToken(base));
        out.push_back({COMMA, ","});
        out.push_back({INT, offset});
        out.push_back({RBRACK, "]"});
    }

    // ---------- store ----------

    static void parseStore(const std::vector<std::string> &words, size_t eqIdx,
                           std::vector<Token> &out) {
        // *xn = xd   or   *(xn + imm) = xd
        // Collect everything before '='
        std::string lhs;
        for (size_t i = 0; i < eqIdx; ++i) {
            if (i) lhs += " ";
            lhs += words[i];
        }
        // Collect everything after '='
        if (eqIdx + 1 >= words.size())
            throw std::runtime_error("Missing value in store");
        std::string val = words[eqIdx + 1];

        // strip leading '*'
        lhs = lhs.substr(1);

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

        out.push_back({ID, "stur"});
        out.push_back(regToken(val));
        out.push_back({COMMA, ","});
        out.push_back({LBRACK, "["});
        out.push_back(regToken(base));
        out.push_back({COMMA, ","});
        out.push_back({INT, offset});
        out.push_back({RBRACK, "]"});
    }
};
