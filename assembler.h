#pragma once

#include "token.h"
#include "symbol_table.h"
#include "encoder.h"

#include <vector>
#include <string>
#include <map>
#include <cstdint>
#include <stdexcept>
#include <iostream>

/// Two-pass assembler that works on Token vectors.
class Assembler {
public:
    /// Assemble a stream of tokens. Emits binary to stdout, labels to stderr.
    void assemble(const std::vector<Token> &tokens) {
        auto lines = groupLines(tokens);
        pass1(lines);
        pass2(lines);
        dumpSymbols();
    }

private:
    SymbolTable symbols_;

    // ---- instruction pattern table ----
    // r = REG or sp,  z = REG or ZREG,  i = INT/HEXINT,
    // j = INT/HEXINT/label,  c = COMMA,  l = '[',  t = ']'
    static const std::map<std::string, std::string> &patterns() {
        static const std::map<std::string, std::string> p = {
            {"add",   "rcrcz"},   {"sub",   "rcrcz"},
            {"mul",   "rcrcz"},   {"smulh", "rcrcz"},
            {"umulh", "rcrcz"},   {"sdiv",  "rcrcz"},
            {"udiv",  "rcrcz"},   {"cmp",   "rcz"},
            {"br",    "r"},       {"blr",   "r"},
            {"ldur",  "rclrcit"}, {"stur",  "rclrcit"},
            {"ldr",   "rcj"},     {"b",     "j"},
        };
        return p;
    }

    static const std::map<std::string, int> &condCodes() {
        static const std::map<std::string, int> c = {
            {".eq",0},{".ne",1},{".hs",2},{".lo",3},
            {".hi",8},{".ls",9},{".ge",10},{".lt",11},
            {".gt",12},{".le",13}
        };
        return c;
    }

    // ---- group tokens into lines ----
    static std::vector<std::vector<Token>> groupLines(const std::vector<Token> &tokens) {
        std::vector<std::vector<Token>> lines;
        std::vector<Token> cur;
        for (auto &t : tokens) {
            if (t.type == NEWLINE) {
                if (!cur.empty()) { lines.push_back(std::move(cur)); cur.clear(); }
            } else {
                cur.push_back(t);
            }
        }
        if (!cur.empty()) lines.push_back(std::move(cur));
        return lines;
    }

    // ---- pass 1 : build symbol table ----
    void pass1(const std::vector<std::vector<Token>> &lines) {
        uint64_t pc = 0;
        for (auto &line : lines) {
            if (line.size() == 1 && line[0].type == LABEL) {
                std::string name = line[0].lexeme;
                if (name.back() == ':') name.pop_back();
                symbols_.define(name, pc);
            } else if (!line.empty() && line[0].type == DOTID && line[0].lexeme == ".8byte") {
                pc += 8;
            } else {
                pc += 4;
            }
        }
    }

    // ---- pass 2 : encode & emit ----
    void pass2(const std::vector<std::vector<Token>> &lines) {
        uint64_t pc = 0;
        for (auto &line : lines) {
            if (line.empty()) continue;

            // skip label-only lines
            if (line.size() == 1 && line[0].type == LABEL) continue;

            // .8byte directive
            if (line[0].type == DOTID && line[0].lexeme == ".8byte") {
                if (line.size() < 2) throw std::runtime_error("Missing operand for .8byte");
                uint64_t val = 0;
                if (line[1].type == ID)
                    val = symbols_.lookup(line[1].lexeme);
                else
                    val = std::stoull(line[1].lexeme, nullptr, 0);
                Encoder::emit64le(val);
                pc += 8;
                continue;
            }

            if (line[0].type != ID)
                throw std::runtime_error("Expected instruction, got: " + line[0].lexeme);

            std::string instr = line[0].lexeme;
            const auto &pat = patterns();

            auto it = pat.find(instr);
            if (it == pat.end() && instr != "b")
                throw std::runtime_error("Unknown instruction: " + instr);

            std::string pattern = (it != pat.end()) ? it->second : "";
            int args[3] = {0, 0, 0};
            int ai = 0;
            size_t ti = 1;

            // handle b.cond
            if (instr == "b" && line.size() > 1 && line[1].type == DOTID) {
                auto ci = condCodes().find(line[1].lexeme);
                if (ci == condCodes().end())
                    throw std::runtime_error("Invalid condition: " + line[1].lexeme);
                args[ai++] = ci->second;
                instr = "b.cond";
                pattern = pat.at("b");   // j
                ti = 2;
            }

            if (pattern.empty())
                throw std::runtime_error("No pattern for instruction: " + instr);

            for (char p : pattern) {
                if (ti >= line.size())
                    throw std::runtime_error("Too few operands for " + instr);
                Token t = line[ti++];
                switch (p) {
                    case 'r':
                        if (t.type == REG || (t.type == ID && t.lexeme == "sp"))
                            args[ai++] = Encoder::readReg(t.lexeme);
                        else throw std::runtime_error("Expected register or sp");
                        break;
                    case 'z':
                        if (t.type != REG && t.type != ZREG)
                            throw std::runtime_error("Expected register or xzr");
                        args[ai++] = Encoder::readReg(t.lexeme);
                        break;
                    case 'c':
                        if (t.type != COMMA) throw std::runtime_error("Expected comma");
                        break;
                    case 'l':
                        if (t.type != LBRACK) throw std::runtime_error("Expected '['");
                        break;
                    case 't':
                        if (t.type != RBRACK) throw std::runtime_error("Expected ']'");
                        break;
                    case 'i':
                        if (t.type == INT || t.type == HEXINT)
                            args[ai++] = Encoder::readImm(t.lexeme);
                        else throw std::runtime_error("Expected immediate");
                        break;
                    case 'j':
                        if (t.type == INT || t.type == HEXINT)
                            args[ai++] = Encoder::readImm(t.lexeme);
                        else if (t.type == ID)
                            args[ai++] = static_cast<int>(
                                static_cast<int64_t>(symbols_.lookup(t.lexeme)) -
                                static_cast<int64_t>(pc));
                        else throw std::runtime_error("Expected immediate or label");
                        break;
                }
            }

            if (ti < line.size())
                throw std::runtime_error("Extra tokens after " + instr);

            uint32_t word = Encoder::encode(instr, args[0], args[1], args[2]);
            Encoder::emit32le(word);
            pc += 4;
        }
    }

    void dumpSymbols() const {
        for (auto &name : symbols_.order())
            std::cerr << name << " " << symbols_.lookup(name) << "\n";
    }
};
