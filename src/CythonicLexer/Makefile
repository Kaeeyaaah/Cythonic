# Makefile for Cythonic C Lexer
# Supports Windows (MinGW/MSVC), Linux, and macOS

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
TARGET = cythonic-lexer
SOURCE = Lexer.c

# Platform detection
ifeq ($(OS),Windows_NT)
    TARGET := $(TARGET).exe
    RM = del /Q
else
    RM = rm -f
endif

.PHONY: all clean run test

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)
	@echo Build complete: $(TARGET)

clean:
	$(RM) $(TARGET)
	@echo Cleaned build artifacts

run: $(TARGET)
	./$(TARGET) ../../samples/sample.cytho

test: $(TARGET)
	@echo Running lexer on sample.cytho...
	./$(TARGET) ../../samples/sample.cytho
	@echo
	@echo Symbol table generated: ../../samples/sample.cytho.symboltable.txt

help:
	@echo Cythonic C Lexer Build System
	@echo ==============================
	@echo.
	@echo Usage:
	@echo   make          - Build the lexer
	@echo   make run      - Build and run on samples/sample.cytho
	@echo   make test     - Build and test lexer
	@echo   make clean    - Remove build artifacts
	@echo   make help     - Show this help message
