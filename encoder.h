#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <stdexcept>

/// Validates and encodes a single ARM64 instruction into a 32-bit word.
class Encoder {
public:
    /// Encode an instruction. Returns the machine-code word.
    static uint32_t encode(const std::string &instr, int a, int b, int c) {
        uint32_t w = 0;
        if      (instr == "add")    w = encodeRRR(0x8B206000, a, b, c);
        else if (instr == "sub")    w = encodeRRR(0xCB206000, a, b, c);
        else if (instr == "mul")    w = encodeRRR(0x9B007C00, a, b, c);
        else if (instr == "smulh")  w = encodeRRR(0x9B407C00, a, b, c);
        else if (instr == "umulh")  w = encodeRRR(0x9BC07C00, a, b, c);
        else if (instr == "sdiv")   w = encodeRRR(0x9AC00C00, a, b, c);
        else if (instr == "udiv")   w = encodeRRR(0x9AC00800, a, b, c);
        else if (instr == "cmp")    w = encodeCmp(a, b);
        else if (instr == "br")     w = encodeBranchReg(0xD61F0000, a);
        else if (instr == "blr")    w = encodeBranchReg(0xD63F0000, a);
        else if (instr == "ldur")   w = encodeMem(0xF8400000, a, b, c);
        else if (instr == "stur")   w = encodeMem(0xF8000000, a, b, c);
        else if (instr == "ldr")    w = encodeLdr(a, b);
        else if (instr == "b")      w = encodeBranch(a);
        else if (instr == "b.cond") w = encodeBCond(a, b);
        else throw std::runtime_error("Unknown instruction: " + instr);
        return w;
    }

    // ---- helpers ----

    static bool validRegister(int r) { return 0 <= r && r <= 31; }

    static bool validSignedImm(int v, int bits) {
        int lo = -(1 << (bits - 1));
        int hi =  (1 << (bits - 1)) - 1;
        return lo <= v && v <= hi;
    }

    static int readImm(const std::string &s) {
        if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
            return std::stoi(s.substr(2), nullptr, 16);
        return std::stoi(s);
    }

    static uint32_t readReg(const std::string &s) {
        if (s == "xzr" || s == "sp") return 31;
        if (s[0] != 'x')
            throw std::runtime_error("Invalid register: " + s);
        int v = std::stoi(s.substr(1));
        if (v < 0 || v > 30)
            throw std::runtime_error("Register out of range: " + s);
        return static_cast<uint32_t>(v);
    }

    // ---- binary output ----

    static void emit32le(uint32_t w) {
        std::cout.put(static_cast<char>(w & 0xFF));
        std::cout.put(static_cast<char>((w >>  8) & 0xFF));
        std::cout.put(static_cast<char>((w >> 16) & 0xFF));
        std::cout.put(static_cast<char>((w >> 24) & 0xFF));
    }

    static void emit64le(uint64_t w) {
        for (int i = 0; i < 8; ++i)
            std::cout.put(static_cast<char>((w >> (8 * i)) & 0xFF));
    }

private:
    static void requireReg(int r) {
        if (!validRegister(r))
            throw std::runtime_error("Invalid register value");
    }

    static uint32_t encodeRRR(uint32_t base, int rd, int rn, int rm) {
        requireReg(rd); requireReg(rn); requireReg(rm);
        return base | rd | (rn << 5) | (rm << 16);
    }

    static uint32_t encodeCmp(int rn, int rm) {
        requireReg(rn); requireReg(rm);
        return 0xEB20601F | (rn << 5) | (rm << 16);
    }

    static uint32_t encodeBranchReg(uint32_t base, int rn) {
        requireReg(rn);
        return base | (rn << 5);
    }

    static uint32_t encodeMem(uint32_t base, int rt, int rn, int imm) {
        requireReg(rt); requireReg(rn);
        if (!validSignedImm(imm, 9))
            throw std::runtime_error("Immediate out of range for ldur/stur");
        uint32_t imm9 = static_cast<uint32_t>(imm) & 0x1FF;
        return base | rt | (rn << 5) | (imm9 << 12);
    }

    static uint32_t encodeLdr(int rd, int offset) {
        if (offset % 4)
            throw std::runtime_error("ldr offset must be divisible by 4");
        requireReg(rd);
        if (!validSignedImm(offset / 4, 19))
            throw std::runtime_error("ldr offset out of range");
        uint32_t imm19 = static_cast<uint32_t>(offset / 4) & 0x7FFFF;
        return 0x58000000 | rd | (imm19 << 5);
    }

    static uint32_t encodeBranch(int offset) {
        if (offset % 4)
            throw std::runtime_error("b offset must be divisible by 4");
        if (!validSignedImm(offset / 4, 26))
            throw std::runtime_error("b offset out of range");
        uint32_t imm26 = static_cast<uint32_t>(offset / 4) & 0x3FFFFFF;
        return 0x14000000 | imm26;
    }

    static uint32_t encodeBCond(int cond, int offset) {
        if (offset % 4)
            throw std::runtime_error("b.cond offset must be divisible by 4");
        if (!validSignedImm(offset / 4, 19))
            throw std::runtime_error("b.cond offset out of range");
        if (cond < 0 || cond > 13)
            throw std::runtime_error("Invalid condition code");
        uint32_t imm19 = static_cast<uint32_t>(offset / 4) & 0x7FFFF;
        return 0x54000000 | (imm19 << 5) | (cond & 0x1F);
    }
};
