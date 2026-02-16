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

/// Reads the pre-tokenized format produced by the CS241 scanner
/// (each line is  "TOKEN_TYPE lexeme"  or  "NEWLINE").
class TokenizedLexer {
public:
    static std::vector<Token> lex(std::istream &in) {
        std::vector<Token> tokens;
        Token t;
        while (!in.eof()) {
            in >> t;
            if (!in.fail()) tokens.push_back(t);
        }
        return tokens;
    }
};

/// Reads raw ARM64 assembly text and produces Token vectors.
class RawAsmLexer {
public:
    static std::vector<Token> lex(std::istream &in) {
        std::vector<Token> tokens;
        std::string line;
        while (std::getline(in, line)) {
            tokenizeLine(line, tokens);
            tokens.push_back({NEWLINE, ""});
        }
        return tokens;
    }

private:
    static void tokenizeLine(const std::string &raw, std::vector<Token> &out) {
        std::string line = stripComment(raw);
        size_t i = 0;
        while (i < line.size()) {
            if (std::isspace(static_cast<unsigned char>(line[i]))) { ++i; continue; }

            if (line[i] == ',') { out.push_back({COMMA, ","}); ++i; continue; }
            if (line[i] == '[') { out.push_back({LBRACK, "["}); ++i; continue; }
            if (line[i] == ']') { out.push_back({RBRACK, "]"}); ++i; continue; }

            // collect a "word"
            size_t start = i;
            while (i < line.size() && !std::isspace(static_cast<unsigned char>(line[i]))
                   && line[i] != ',' && line[i] != '[' && line[i] != ']')
                ++i;
            std::string word = line.substr(start, i - start);
            classifyAndPush(word, out);
        }
    }

    static std::string stripComment(const std::string &line) {
        // comments start with ; or //
        auto pos = line.find(';');
        if (pos == std::string::npos) pos = line.find("//");
        return (pos == std::string::npos) ? line : line.substr(0, pos);
    }

    /// Classify a single word into one or more tokens.
    /// For "b.eq" style conditional branches, emits two tokens: ID "b" + DOTID ".eq".
    static void classifyAndPush(const std::string &word, std::vector<Token> &out) {
        if (word.empty()) return;

        // b.cond forms: b.eq, b.ne, b.lt, etc.
        static const std::set<std::string> condSuffixes = {
            ".eq",".ne",".hs",".lo",".hi",".ls",".ge",".lt",".gt",".le"
        };
        if (word.size() >= 4 && word[0] == 'b' && word[1] == '.') {
            std::string suffix = word.substr(1);
            if (condSuffixes.count(suffix)) {
                out.push_back({ID, "b"});
                out.push_back({DOTID, suffix});
                return;
            }
        }

        out.push_back(classify(word));
    }

    static Token classify(const std::string &word) {
        if (word.empty()) return {NONE, word};

        // label definition  e.g.  "loop:"
        if (word.back() == ':') return {LABEL, word};

        // directive  e.g.  ".8byte"
        if (word[0] == '.') return {DOTID, word};

        // hex literal
        if (word.size() > 2 && word[0] == '0' && (word[1] == 'x' || word[1] == 'X'))
            return {HEXINT, word};

        // integer literal (possibly negative)
        if (isInteger(word)) return {INT, word};

        // register xzr
        if (word == "xzr") return {ZREG, word};

        // register x0-x30
        if (word.size() >= 2 && word[0] == 'x' && std::isdigit(static_cast<unsigned char>(word[1]))) {
            return {REG, word};
        }

        // sp is treated as an ID (handled by assembler as register 31)
        // everything else is an ID (instruction name, label reference, sp, etc.)
        return {ID, word};
    }

    static bool isInteger(const std::string &s) {
        if (s.empty()) return false;
        size_t start = (s[0] == '-' || s[0] == '+') ? 1 : 0;
        if (start >= s.size()) return false;
        return std::all_of(s.begin() + start, s.end(),
                           [](unsigned char c){ return std::isdigit(c); });
    }
};
