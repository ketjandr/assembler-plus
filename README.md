# ARM64 Assembler+

Assembler+ is a two-pass ARM64 assembler that supports three input formats:

1. **Pre-tokenized** (`--tokenized`, default) — the CS241 scanner output format (`TOKEN_TYPE lexeme`)
2. **Raw assembly** (`--raw`) — standard ARM64 assembly text
3. **High-level pseudocode** (`--high`) — a readable, C-like syntax

All three modes produce identical machine code for equivalent programs.

## Building

```bash
make        # produces ./asm
make clean  # removes the binary
```

Requires a C++20-compatible compiler (e.g. `g++` or `clang++`).

## Usage

```
asm [OPTIONS] [FILE]
```

| Flag | Description |
|------|-------------|
| `--tokenized` | Input is pre-tokenized format (default) |
| `--raw` | Input is raw ARM64 assembly |
| `--high` | Input is high-level pseudocode |
| `--help`, `-h` | Show usage |

If `FILE` is omitted or is `-`, reads from stdin. Binary output goes to stdout; labels are printed to stderr.

```bash
./asm --raw program.s > program.bin
./asm --high program.hl > program.bin
cat tokens.txt | ./asm > program.bin
```

## Supported Instructions

| Instruction | Syntax | Description |
|-------------|--------|-------------|
| `add` | `add xd, xn, xm` | xd = xn + xm |
| `sub` | `sub xd, xn, xm` | xd = xn − xm |
| `mul` | `mul xd, xn, xm` | xd = xn × xm (low 64 bits) |
| `smulh` | `smulh xd, xn, xm` | xd = (xn × xm) >> 64 (signed) |
| `umulh` | `umulh xd, xn, xm` | xd = (xn × xm) >> 64 (unsigned) |
| `sdiv` | `sdiv xd, xn, xm` | xd = xn ÷ xm (signed) |
| `udiv` | `udiv xd, xn, xm` | xd = xn ÷ xm (unsigned) |
| `cmp` | `cmp xn, xm` | Set flags for xn − xm |
| `b` | `b label` | Unconditional branch |
| `b.cond` | `b.eq label` | Conditional branch (eq, ne, lt, le, gt, ge, hs, lo, hi, ls) |
| `br` | `br xn` | Branch to register |
| `blr` | `blr xn` | Branch-with-link to register |
| `ldr` | `ldr xd, offset` | PC-relative load |
| `ldur` | `ldur xd, [xn, imm]` | Load from base + offset |
| `stur` | `stur xd, [xn, imm]` | Store to base + offset |
| `.8byte` | `.8byte value` | Emit a 64-bit constant |

## High-Level Syntax

The `--high` mode accepts a pseudocode language that is lowered to ARM64 instructions before assembly.

| Syntax | Lowers to |
|--------|-----------|
| `label <name>` | `<name>:` |
| `x1 = x2 + x3` | `add x1, x2, x3` |
| `x1 = x2 - x3` | `sub x1, x2, x3` |
| `x1 = x2 * x3` | `mul x1, x2, x3` |
| `x1 = x2 / x3` | `sdiv x1, x2, x3` |
| `x1 = x2 % x3` | `sdiv` → `mul` → `sub` sequence |
| `x1 = x2` | `add x1, x2, xzr` (move) |
| `x1 = *x2` | `ldur x1, [x2, 0]` |
| `x1 = *(x2 + 8)` | `ldur x1, [x2, 8]` |
| `*x1 = x2` | `stur x2, [x1, 0]` |
| `*(x1 + 8) = x2` | `stur x2, [x1, 8]` |
| `if x1 == x2 goto lbl` | `cmp x1, x2` + `b.eq lbl` |
| `if x1 != x2 goto lbl` | `cmp x1, x2` + `b.ne lbl` |
| `if x1 < x2 goto lbl` | `cmp x1, x2` + `b.lt lbl` |
| `if x1 <= x2 goto lbl` | `cmp x1, x2` + `b.le lbl` |
| `if x1 > x2 goto lbl` | `cmp x1, x2` + `b.gt lbl` |
| `if x1 >= x2 goto lbl` | `cmp x1, x2` + `b.ge lbl` |
| `goto lbl` | `b lbl` |
| `call x1` | `blr x1` |
| `ret` | `br x30` |
| `.8byte val` | `.8byte val` |
| `# comment` | ignored |

### Example

```
# sum x1 += x3 while x1 != x2
label loop
if x1 == x2 goto done
x1 = x1 + x3
goto loop
label done
ret
```

This is equivalent to the raw assembly:

```asm
loop:
    cmp x1, x2
    b.eq done
    add x1, x1, x3
    b loop
done:
    br x30
```

## Project Structure

```
├── main.cpp           # Entry point — mode selection & I/O
├── token.h            # Token struct, TokenType enum, I/O operators
├── lexer.h            # TokenizedLexer (CS241 format), RawAsmLexer (raw text)
├── highlevel.h        # HighLevelParser — pseudocode → Token lowering
├── symbol_table.h     # SymbolTable — label definition & lookup
├── encoder.h          # Encoder — instruction validation & machine code encoding
├── assembler.h        # Assembler — two-pass orchestration
├── Makefile
└── README.md
```

| Module | Responsibility |
|--------|---------------|
| **Token** | Data types shared across all stages |
| **Lexer** | Convert input text → `Token` stream (two strategies) |
| **HighLevelParser** | Convert pseudocode → `Token` stream |
| **SymbolTable** | Track label → address mappings |
| **Encoder** | Validate operands and emit 32-bit machine code per instruction |
| **Assembler** | Group tokens into lines, run pass 1 (symbols) and pass 2 (encode + emit) |
