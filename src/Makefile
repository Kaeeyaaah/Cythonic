# Makefile for Cythonic C Lexer
# Supports Windows (MinGW/MSVC), Linux, and macOS

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
LEXER_TARGET = cythonic-lexer
PARSER_TARGET = cythonic-parser
LEXER_SRC = Lexer/Lexer.c
PARSER_SRC = Parser/Parser.c Lexer/Lexer.c

# Platform detection
ifeq ($(OS),Windows_NT)
    LEXER_TARGET := $(LEXER_TARGET).exe
    PARSER_TARGET := $(PARSER_TARGET).exe
    RM = del /Q
else
    RM = rm -f
endif

.PHONY: all clean run test parser run-parser

all: $(LEXER_TARGET) $(PARSER_TARGET)

$(LEXER_TARGET): $(LEXER_SRC)
	$(CC) $(CFLAGS) -DLEXER_MAIN -o $(LEXER_TARGET) $(LEXER_SRC)
	@echo Build complete: $(LEXER_TARGET)

$(PARSER_TARGET): $(PARSER_SRC)
	$(CC) $(CFLAGS) -o $(PARSER_TARGET) $(PARSER_SRC)
	@echo Build complete: $(PARSER_TARGET)

clean:
	$(RM) $(LEXER_TARGET) $(PARSER_TARGET)
	@echo Cleaned build artifacts

run: $(LEXER_TARGET)
	./$(LEXER_TARGET) ../../samples/sample.cytho

run-parser: $(PARSER_TARGET)
	./$(PARSER_TARGET) ../../samples/sample.cytho

test: $(LEXER_TARGET)
	@echo Running lexer on sample.cytho...
	./$(LEXER_TARGET) ../../samples/sample.cytho
	@echo
	@echo Symbol table generated: ../../samples/sample.cytho.symboltable.txt

help:
	@echo Cythonic C Build System
	@echo ==============================
	@echo.
	@echo Usage:
	@echo   make              - Build lexer and parser
	@echo   make run          - Run lexer on sample
	@echo   make run-parser   - Run parser on sample
	@echo   make test         - Test lexer
	@echo   make clean        - Remove artifacts
