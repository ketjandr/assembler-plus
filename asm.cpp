#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <cstdint>
#include <vector>

/** Prints an error to stderr with an "ERROR: " prefix, and newline suffix. Terminates the program with an error.
 *
 * @param message The error to print
 */
void formatError(const std::string & message)
{
    throw std::runtime_error(message);
}

enum TokenType {
    // Not a real token type we output: a unique value for initializing a TokenType when the
    // actual value is unknown
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

struct Token
{
    TokenType type;
    std::string lexeme;
};

#define TOKEN_TYPE_READER(t) if(s == #t) return t
TokenType stringToTokenType(const std::string & s)
{
    TOKEN_TYPE_READER(DOTID);
    TOKEN_TYPE_READER(LABEL);
    TOKEN_TYPE_READER(ID);
    TOKEN_TYPE_READER(HEXINT);
    TOKEN_TYPE_READER(REG);
    TOKEN_TYPE_READER(ZREG);
    TOKEN_TYPE_READER(INT);
    TOKEN_TYPE_READER(COMMA);
    TOKEN_TYPE_READER(LBRACK);
    TOKEN_TYPE_READER(RBRACK);
    TOKEN_TYPE_READER(NEWLINE);
    return NONE;
}
#undef TOKEN_TYPE_READER

#define TOKEN_TYPE_PRINTER(t) case t: return #t
std::string tokenTypeToString(const TokenType & t)
{
    switch (t) {
        TOKEN_TYPE_PRINTER(DOTID);
        TOKEN_TYPE_PRINTER(LABEL);
        TOKEN_TYPE_PRINTER(ID);
        TOKEN_TYPE_PRINTER(HEXINT);
        TOKEN_TYPE_PRINTER(REG);
        TOKEN_TYPE_PRINTER(ZREG);
        TOKEN_TYPE_PRINTER(INT);
        TOKEN_TYPE_PRINTER(COMMA);
        TOKEN_TYPE_PRINTER(LBRACK);
        TOKEN_TYPE_PRINTER(RBRACK);
        TOKEN_TYPE_PRINTER(NEWLINE);
        default:
            formatError("Unrecognized token type");
            return "";
    }
    return "NONE";
}
#undef TOKEN_TYPE_PRINTER

std::ostream & operator<<(std::ostream & out, const Token token)
{
    out << tokenTypeToString(token.type) << " " << token.lexeme;
    return out;
}

std::istream & operator>>(std::istream & in, Token& token)
{
    std::string tokenType;
    in >> tokenType;
    token.type = stringToTokenType(tokenType);
    if (token.type != NEWLINE) {
        in >> token.lexeme;
    } else {
        token.lexeme = "";
    }
    return in;
}

bool validRegister(int i) {
    return 0 <= i && i <= 31;
}

bool validSignedImm(int i, int bits) {
    int lo = -(1 << (bits - 1));
    int hi =  (1 << (bits - 1)) - 1;
    return lo <= i && i <= hi;
}

int readImm(const std::string & s)
{
    if(s.starts_with("0x"))
    {
        return std::stoi(s.substr(2), nullptr, 16);
    }
    return std::stoi(s);
}

uint32_t readReg(const std::string & s)
{
    if (s == "xzr" || s == "sp") return 31;

    if(!s.starts_with("x"))
    {
        throw std::runtime_error("Invalid register value '" + s + "'");
    }
    int ret = std::stoi(s.substr(1), nullptr, 10);
    if (ret > 30) {
        throw std::runtime_error("Register value '" + s + "' is too large");
    }
    if (ret < 0) {
        throw std::runtime_error("Register value '" + s + "' is negative");
    }
    return ret;
}

/** For a given instruction, returns the machine code for that instruction.
 *
 * @param[out] word The machine code for the instruction
 * @param instruction The name of the instruction
 * @param one The value of the first parameter
 * @param two The value of the second parameter
 * @param three The value of the third parameter
 */
bool compileLine(uint32_t &          word,
                 const std::string & instruction,
                 int            one,
                 int            two,
                 int            three)
{
    if (instruction == "add") {
        if (!validRegister(one) || !validRegister(two) || !validRegister(three)) {
            std::cerr << "ERROR: invalid register value\n";
            return false;
        }
        word = 0x8B206000;
        word |= one;
        word |= (two << 5);
        word |= (three << 16);
    } else if (instruction == "sub") {
        if (!validRegister(one) || !validRegister(two) || !validRegister(three)) {
            std::cerr << "ERROR: invalid register value\n";
            return false;
        }
        word = 0xCB206000;
        word |= one;
        word |= (two << 5);
        word |= (three << 16);
    } else if (instruction == "mul") {
        if (!validRegister(one) || !validRegister(two) || !validRegister(three)) {
            std::cerr << "ERROR: invalid register value\n";
            return false;
        }
        word = 0x9B007C00;
        word |= one;
        word |= (two << 5);
        word |= (three << 16);
    } else if (instruction == "smulh") {
        if (!validRegister(one) || !validRegister(two) || !validRegister(three)) {
            std::cerr << "ERROR: invalid register value\n";
            return false;
        }
        word = 0x9B407C00;
        word |= one;
        word |= (two << 5);
        word |= (three << 16);
    } else if (instruction == "umulh") {
        if (!validRegister(one) || !validRegister(two) || !validRegister(three)) {
            std::cerr << "ERROR: invalid register value\n";
            return false;
        }
        word = 0x9BC07C00;
        word |= one;
        word |= (two << 5);
        word |= (three << 16);
    } else if (instruction == "sdiv") {
        if (!validRegister(one) || !validRegister(two) || !validRegister(three)) {
            std::cerr << "ERROR: invalid register value\n";
            return false;
        }
        word = 0x9AC00C00;
        word |= one;
        word |= (two << 5);
        word |= (three << 16);
    } else if (instruction == "udiv") {
        if (!validRegister(one) || !validRegister(two) || !validRegister(three)) {
            std::cerr << "ERROR: invalid register value\n";
            return false;
        }
        word = 0x9AC00800;
        word |= one;
        word |= (two << 5);
        word |= (three << 16);
    } else if (instruction == "cmp") {
        if (!validRegister(one) || !validRegister(two)) {
            std::cerr << "ERROR: invalid register value\n";
            return false;
        }
        word = 0xEB20601F;
        word |= (one << 5);
        word |= (two << 16);
    } else if (instruction == "br") {
        if (!validRegister(one)) {
            std::cerr << "ERROR: invalid register value\n";
            return false;
        }
        word = 0xD61F0000;
        word |= (one << 5);
    } else if (instruction == "blr") {
        if (!validRegister(one)) {
            std::cerr << "ERROR: invalid register value\n";
            return false;
        }
        word = 0xD63F0000;
        word |= (one << 5);
    } else if (instruction == "ldur") {
        if (!validRegister(one) || !validRegister(two) || !validSignedImm(three, 9)) {
            std::cerr << "ERROR: invalid register or immediate value\n";
            return false;
        }
        word = 0xF8400000;
        word |= one;
        word |= (two << 5);
        // only take lowest 9 bits, in case imm < 0
        uint32_t imm9 = static_cast<uint32_t>(three) & 0x1FF;
        word |= (imm9 << 12);
    } else if (instruction == "stur") {
        if (!validRegister(one) || !validRegister(two) || !validSignedImm(three, 9)) {
            std::cerr << "ERROR: invalid register or immediate value\n";
            return false;
        }
        word = 0xF8000000;
        word |= one;
        word |= (two << 5);
        // only take lowest 9 bits, in case imm < 0
        uint32_t imm9 = static_cast<uint32_t>(three) & 0x1FF;
        word |= (imm9 << 12);
    } else if (instruction == "ldr") {
        if (two % 4) {
            std::cerr << "ERROR: Imm must be divisible by 4\n";
            return false;
        }
        if (!validRegister(one) || !validSignedImm(two / 4, 19)) {
            std::cerr << "ERROR: invalid register or immediate value\n";
            return false;
        }
        word = 0x58000000;
        word |= one;
        // only take lowest 19 bits, in case imm < 0
        uint32_t imm19 = static_cast<uint32_t>(two / 4) & 0x7FFFF;
        word |= (imm19 << 5);
    } else if (instruction == "b") {
        if (one % 4) {
            std::cerr << "ERROR: Imm must be divisible by 4\n";
            return false;
        }
        if (!validSignedImm(one / 4, 26)) {
            std::cerr << "ERROR: invalid immediate value\n";
            return false;
        }
        word = 0x14000000;
        // only take lowest 26 bits, in case imm < 0
        uint32_t imm26 = static_cast<uint32_t>(one / 4) & 0x3FFFFFF;
        word |= imm26;
    } else if (instruction == "b.cond") {
        if (two % 4 != 0) {
            std::cerr << "ERROR: b.cond offset must be divisible by 4\n";
            return false;
        }
        if (!validSignedImm(two / 4, 19)) {
            std::cerr << "ERROR: b.cond immediate out of range\n";
            return false;
        }
        if (one < 0 || one > 13) {
            std::cerr << "ERROR: invalid condition code\n";
            return false;
        }
        word = 0x54000000;

        // only take lowest 19 bits, in case imm < 0
        uint32_t imm19 = static_cast<uint32_t>(two / 4) & 0x7FFFF;
        word |= (imm19 << 5);
        
        // condition code
        word |= (one & 0x1F);
    } else {
        std::cerr << "ERROR: invalid instruction\n";
        return false;
    }
    return true;
}

static void emit32le(uint32_t w) {
  std::cout.put((char)(w & 0xFF));
  std::cout.put((char)((w >> 8) & 0xFF));
  std::cout.put((char)((w >> 16) & 0xFF));
  std::cout.put((char)((w >> 24) & 0xFF));
}
static void emit64le(uint64_t w) {
  for (int i = 0; i < 8; i++) std::cout.put((char)((w >> (8*i)) & 0xFF));
}

/** Takes a tokenization of an ARM64 assembly file as input, then outputs a list of parameters for compileLine,
 * replacing label uses with their respective addresses. Prints label addresses into standard out.
 *
 * If the file is not found, print an error and returns a non-0 value.
 *
 * @return 0 on success, non-0 on error
 */
int _main(int argc, char * argv[])
{
    if(argc > 2)
    {
        std::cerr << "Usage:" << std::endl
                  << "\ttokenasm [FILE]" << std::endl
                  << std::endl
                  << "If FILE is unspecified or if FILE is `-`, read the assembly from standard "
                  << "in. Otherwise, read the assembly from FILE." << std::endl;
        return 1;
    }

    std::ifstream fp;
    std::istream &in =
        (argc > 1 && std::string(argv[1]) != "-")
      ? [&]() -> std::istream& {
            fp.open(argv[1]);
            return fp;
        }()
      : std::cin;

    if(!fp && argc > 1)
    {
        formatError((std::stringstream() << "File '" << argv[1] << "' not found!").str());
        return 1;
    }
    Token currToken;
    std::vector<Token> tokens;
    while (!in.eof()) {
        in >> currToken;
        if (!in.fail()) tokens.push_back(currToken);
        
    }

    // group tokens into valid assembly lines
    std::vector<std::vector<Token>> lines;
    std::vector<Token> currLine;

    for (const Token token : tokens) {
        if (!(token.type == NEWLINE)) {
            currLine.push_back(token);
        } else {
            if (!currLine.empty()) {
                lines.push_back(currLine);
                currLine.clear();
            }
        }
    }

    if (!currLine.empty()) lines.push_back(currLine);

    // pass 1: build a symbol table
    std::map<std::string, uint64_t> symbolTable;
    std::vector<std::string> symbolOrder;
    uint64_t pc = 0;

    for (const auto &currLine : lines) {
        if (currLine.size() == 1 && currLine[0].type == LABEL) {
            std::string name = currLine[0].lexeme;
            name.pop_back(); // remove colon

            if (symbolTable.count(name)) formatError("Found a duplicate label");
            
            symbolTable[name] = pc;
            symbolOrder.push_back(name);
        } else if (currLine.size() >= 1 && currLine[0].type == DOTID && currLine[0].lexeme == ".8byte") {
            pc += 8;
        } else {
            pc += 4;
        }
    }

    // r = "REG" or sp, z = "REG" or "ZREG", i = "INT" or "HEXINT"
    // j = "INT" or "HEXTINT" or "LABEL"
    // c = "COMMA", l = "LBRACK", t = "RBRACK"
    const std::map<std::string, std::string> INSTRUCTION_PARAMETER_PATTERN = {
        {"add",   "rcrcz"},   // add xd, xn, xm
        {"sub",   "rcrcz"},   // sub xd, xn, xm
        {"mul",   "rcrcz"},   // mul xd, xn, xm
        {"smulh", "rcrcz"},   // smulh xd, xn, xm
        {"umulh", "rcrcz"},   // umulh xd, xn, xm
        {"sdiv",  "rcrcz"},   // sdiv xd, xn, xm
        {"udiv",  "rcrcz"},   // udiv xd, xn, xm
        {"cmp",   "rcz"},     // cmp xn, xm
        {"br",    "r"},       // br xn
        {"blr",   "r"},       // blr xn
        {"ldur",  "rclrcit"}, // ldur xd, [xn, i]
        {"stur",  "rclrcit"}, // stur xd, [xn, i]
        {"ldr",   "rcj"},     // ldr xd, i
        {"b",     "j"},       // b i (or b.cond i)
    };

    const std::map<std::string, int> B_COND_TO_INT = {
        {".eq", 0}, {".ne", 1}, {".hs", 2}, {".lo", 3},
        {".hi", 8}, {".ls", 9}, {".ge", 10}, {".lt", 11},
        {".gt", 12}, {".le", 13}
    };

    // pass 2: generate assembly code
    pc = 0;
    for (const auto &line : lines) {
        if (line.empty()) continue;

        // skip labels
        if (line.size() == 1 && line[0].type == LABEL) {
            continue; 
        }

        // handle .8byte case
        if (line[0].type == DOTID && line[0].lexeme == ".8byte") {
            if (line.size() < 2) formatError("Missing operand for .8byte");
            
            uint64_t value = 0;
            if (line[1].type == ID) { // refers to label
                if (symbolTable.count(line[1].lexeme)) {
                    value = symbolTable[line[1].lexeme];
                } else {
                    formatError("Undefined label: " + line[1].lexeme);
                }
            } else {
                value = std::stoull(line[1].lexeme, nullptr, 0); // handles hex and decimal
            }
            emit64le(value);
            pc += 8;
        } else if (line[0].type == ID) {
            std::string instr = line[0].lexeme;

            std::string pattern = INSTRUCTION_PARAMETER_PATTERN.at(instr);
            std::vector<int> args(3, 0); // args for compileLine
            int argIdx = 0;
            size_t tokenIdx = 1; // read the first token already, i.e. the ID

            if (line[0].lexeme == "b" && line.size() > 1 && line[1].type == DOTID) {
                if (B_COND_TO_INT.count(line[1].lexeme)) {
                    args[argIdx++] = B_COND_TO_INT.at(line[1].lexeme);
                } else {
                    formatError("Invalid condition: " + line[1].lexeme);
                }
                instr = "b.cond";
                tokenIdx = 2;
            }

            for (char p : pattern) {
                if (tokenIdx >= line.size()) formatError("Too few tokens for " + instr);
                
                Token t = line[tokenIdx++];
                
                switch (p) {
                    case 'r': 
                        if (t.type == REG || (t.type == ID && t.lexeme == "sp")) {
                            args[argIdx++] = readReg(t.lexeme);
                        } else {
                            formatError("Expected register or sp");
                        }
                        break;
                    case 'z':
                        if (t.type != REG && t.type != ZREG) formatError("Expected register or xzr");
                        args[argIdx++] = readReg(t.lexeme);
                        break;
                    case 'c':
                        if (t.type != COMMA) formatError("Expected comma");
                        break;
                    case 'l':
                        if (t.type != LBRACK) formatError("Expected '['");
                        break;
                    case 't':
                        if (t.type != RBRACK) formatError("Expected ']'");
                        break;
                    case 'i':
                        if (t.type == INT || t.type == HEXINT) {
                            args[argIdx++] = readImm(t.lexeme);
                        } else {
                            formatError("Expected immediate or label");
                        }
                        break;
                    case 'j':
                        if (t.type == INT || t.type == HEXINT) {
                            args[argIdx++] = readImm(t.lexeme);
                        } else if (t.type == ID) { // refers to label
                            if (symbolTable.count(t.lexeme)) {
                                args[argIdx++] = (int64_t)symbolTable[t.lexeme] - (int64_t)pc;
                            } else {
                                formatError("Undefined label: " + t.lexeme);
                            }
                        } else {
                            formatError("Expected immediate or label");
                        }
                        break;
                }
            }

            if (tokenIdx < line.size()) formatError("Extra tokens after instruction");

            uint32_t word = 0;
            if (compileLine(word, instr, args[0], args[1], args[2])) {
                emit32le(word);
            } else {
                formatError("Failed to assemble " + instr);
            }
            pc += 4;
        } else {
            formatError("Invalid Syntax!");
        }
    }

    for (const auto& name : symbolOrder) {
        std::cerr << name << " " << symbolTable[name] << "\n";
    }

    return 0;
}

int main(int argc, char* argv[]) {
    try {
        _main(argc, argv);
        return 0;
    } catch (std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
}