CXX      := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -O2

HEADERS  := token.h lexer.h encoder.h symbol_table.h assembler.h highlevel.h
TARGET   := asm

.PHONY: all clean

all: $(TARGET)

$(TARGET): main.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ main.cpp

clean:
	rm -f $(TARGET)
