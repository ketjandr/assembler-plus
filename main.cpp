#include "token.h"
#include "lexer.h"
#include "encoder.h"
#include "symbol_table.h"
#include "assembler.h"
#include "ir.h"
#include "highlevel.h"
#include "ir_codegen.h"

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
              << "Options:\n"
              << "  --dump-ir     (--high only) Print IR to stderr instead of assembling\n\n"
              << "If FILE is omitted or is `-`, reads from stdin.\n";
}

int main(int argc, char *argv[]) {
    try {
        enum Mode { TOKENIZED, RAW, HIGH } mode = TOKENIZED;
        bool dumpIRFlag = false;
        const char *filename = nullptr;

        for (int i = 1; i < argc; ++i) {
            if (std::strcmp(argv[i], "--tokenized") == 0)      mode = TOKENIZED;
            else if (std::strcmp(argv[i], "--raw") == 0)       mode = RAW;
            else if (std::strcmp(argv[i], "--high") == 0)      mode = HIGH;
            else if (std::strcmp(argv[i], "--dump-ir") == 0)   dumpIRFlag = true;
            else if (std::strcmp(argv[i], "--help") == 0 ||
                     std::strcmp(argv[i], "-h") == 0) {
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

        // --- build token stream ---
        std::vector<Token> tokens;

        if (mode == HIGH) {
            // High-level pipeline:  source → IR → tokens
            auto ir = HighLevelParser::parse(in);

            if (dumpIRFlag) {
                dumpIR(ir, std::cerr);
                return 0;
            }

            tokens = IRCodeGen::lower(ir);
        } else {
            // Tokenized / raw pipelines go straight to tokens
            if (mode == TOKENIZED)  tokens = TokenizedLexer::lex(in);
            else                    tokens = RawAsmLexer::lex(in);
        }

        // --- assemble ---
        Assembler assembler;
        assembler.assemble(tokens);

        return 0;
    } catch (const std::exception &e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
}
