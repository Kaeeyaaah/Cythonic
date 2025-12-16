# Makefile for Cythonic Compiler
# Supports Windows (MinGW/MSVC), Linux, and macOS

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
TARGET = cythonic
SRC = Cythonic.c

# Platform detection
ifeq ($(OS),Windows_NT)
    TARGET := $(TARGET).exe
    RM = del /Q
else
    RM = rm -f
endif

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)
	@echo Build complete: $(TARGET)

clean:
	$(RM) $(TARGET)
	@echo Cleaned build artifacts

run: $(TARGET)
	./$(TARGET) ../samples/sample.cytho
