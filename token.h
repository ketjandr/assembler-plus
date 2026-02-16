#pragma once

#include <iostream>
#include <string>
#include <stdexcept>

enum TokenType {
    NONE,
    DOTID,
    LABEL,
    ID,
    HEXINT,
    REG,
    ZREG,
    INT,
    COMMA,
    LBRACK,
    RBRACK,
    NEWLINE
};

struct Token {
    TokenType type;
    std::string lexeme;
};

// ---------- conversion helpers ----------

inline TokenType stringToTokenType(const std::string &s) {
#define TRY(t) if (s == #t) return t
    TRY(DOTID); TRY(LABEL); TRY(ID); TRY(HEXINT);
    TRY(REG); TRY(ZREG); TRY(INT); TRY(COMMA);
    TRY(LBRACK); TRY(RBRACK); TRY(NEWLINE);
#undef TRY
    return NONE;
}

inline std::string tokenTypeToString(TokenType t) {
    switch (t) {
#define CASE(t) case t: return #t
        CASE(DOTID); CASE(LABEL); CASE(ID); CASE(HEXINT);
        CASE(REG); CASE(ZREG); CASE(INT); CASE(COMMA);
        CASE(LBRACK); CASE(RBRACK); CASE(NEWLINE);
#undef CASE
        default: throw std::runtime_error("Unrecognized token type");
    }
}

// ---------- I/O ----------

inline std::ostream &operator<<(std::ostream &out, const Token &tok) {
    out << tokenTypeToString(tok.type) << " " << tok.lexeme;
    return out;
}

inline std::istream &operator>>(std::istream &in, Token &tok) {
    std::string tt;
    in >> tt;
    tok.type = stringToTokenType(tt);
    tok.lexeme = (tok.type != NEWLINE) ? (in >> tok.lexeme, tok.lexeme) : "";
    return in;
}
