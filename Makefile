# ────────────────────────────────
# Makefile for CLI project
# ────────────────────────────────

# Compiler and flags
CC      := gcc
CFLAGS  := -Wall -g -Wno-unused-function -I build -I src -I src/commands/scaffold -I src/utils
LDFLAGS := -lfl -lreadline

# Build paths
BUILD_DIR := build
TARGET    := $(BUILD_DIR)/cli

# Source files
SRCS := src/main.c \
        src/commands/commands.c \
        src/commands/scaffold/scaffold.c \
        src/commands/template/template.c \
        build/lex.yy.c \
        build/cli.tab.c

# Parser / Lexer
LEX_SRC := src/parser/cli.l
YACC_SRC := src/parser/cli.y

# Generated files
LEX_OUT := build/lex.yy.c
YACC_OUT := build/cli.tab.c
YACC_HDR := build/cli.tab.h

# ────────────────────────────────
# Rules
# ────────────────────────────────

all: $(TARGET)

# Generate parser
$(YACC_OUT) $(YACC_HDR): $(YACC_SRC)
	@echo "Generating parser..."
	@bison -d -o $(YACC_OUT) $(YACC_SRC)

# Generate lexer
$(LEX_OUT): $(LEX_SRC) $(YACC_HDR)
	@echo "Generating lexer..."
	@flex -o $(LEX_OUT) $(LEX_SRC)

# Compile everything
$(TARGET): $(SRCS) src/utils/defs.h
	@echo "Compiling CLI..."
	@$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)
	@echo "Done!"

# Run CLI
run: $(TARGET)
	@echo "Starting CLI..."
	@./$(TARGET)

# Clean build
clean:
	@echo "Cleaning..."
	@rm -f $(BUILD_DIR)/*