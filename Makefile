CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic -I./include
DEBUG_FLAGS = -g -DDEBUG
RELEASE_FLAGS = -O2 -DNDEBUG
LDFLAGS =

# Source files and object files
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES))

# Targets
TARGET = $(BIN_DIR)/colc

# Default target
all: dirs release

# Debug build
debug: CFLAGS += $(DEBUG_FLAGS)
debug: dirs $(TARGET)

# Release build
release: CFLAGS += $(RELEASE_FLAGS)
release: dirs $(TARGET)

# Create directories
dirs:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

# Link
$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Compile
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Dependencies
$(OBJ_DIR)/arena.o: $(SRC_DIR)/arena.c include/arena.h
$(OBJ_DIR)/lexer.o: $(SRC_DIR)/lexer.c include/lexer.h include/ast.h include/arena.h
$(OBJ_DIR)/parser.o: $(SRC_DIR)/parser.c include/parser.h include/lexer.h include/ast.h include/arena.h
$(OBJ_DIR)/ast.o: $(SRC_DIR)/ast.c include/ast.h include/arena.h
$(OBJ_DIR)/codegen.o: $(SRC_DIR)/codegen.c include/codegen.h include/ast.h include/arena.h
$(OBJ_DIR)/colc.o: $(SRC_DIR)/colc.c include/colc.h include/lexer.h include/parser.h include/ast.h include/codegen.h include/arena.h
$(OBJ_DIR)/main.o: $(SRC_DIR)/main.c include/colc.h

# Clean
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Install
install: release
	install -d $(DESTDIR)/usr/local/bin
	install -m 755 $(TARGET) $(DESTDIR)/usr/local/bin

# Test
test: debug
	@echo "Running tests..."
	@mkdir -p test/output
	./$(TARGET) -v test/sample.c -o test/output/sample.cof
	@echo "Tests completed."

.PHONY: all debug release dirs clean install test