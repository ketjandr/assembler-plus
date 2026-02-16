#include "token.h"
#include "lexer.h"
#include "encoder.h"
#include "symbol_table.h"
#include "assembler.h"
#include "highlevel.h"

#include <fstream>
#include <iostream>
#include <string>
#include <cstring>
#include <vector>

static void printUsage() {
    std::cerr << "Usage:\n"
              << "  asm [OPTIONS] [FILE]\n\n"
              << "Modes (pick one, default is --tokenized):\n"
              << "  --tokenized   Input is pre-tokenized (TOKEN_TYPE lexeme) format\n"
              << "  --raw         Input is raw ARM64 assembly text\n"
              << "  --high        Input is high-level pseudocode syntax\n\n"
              << "If FILE is omitted or is `-`, reads from stdin.\n";
}

int main(int argc, char *argv[]) {
    try {
        enum Mode { TOKENIZED, RAW, HIGH } mode = TOKENIZED;
        const char *filename = nullptr;

        for (int i = 1; i < argc; ++i) {
            if (std::strcmp(argv[i], "--tokenized") == 0) mode = TOKENIZED;
            else if (std::strcmp(argv[i], "--raw") == 0)  mode = RAW;
            else if (std::strcmp(argv[i], "--high") == 0) mode = HIGH;
            else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
                printUsage();
                return 0;
            }
            else filename = argv[i];
        }

        // open input
        std::ifstream fp;
        if (filename && std::string(filename) != "-") {
            fp.open(filename);
            if (!fp) {
                std::cerr << "ERROR: Cannot open file: " << filename << "\n";
                return 1;
            }
        }
        std::istream &in = fp.is_open() ? fp : std::cin;

        // lex / parse into tokens
        std::vector<Token> tokens;
        switch (mode) {
            case TOKENIZED: tokens = TokenizedLexer::lex(in); break;
            case RAW:       tokens = RawAsmLexer::lex(in);    break;
            case HIGH:      tokens = HighLevelParser::parse(in); break;
        }

        // assemble
        Assembler assembler;
        assembler.assemble(tokens);

        return 0;
    } catch (const std::exception &e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
}
