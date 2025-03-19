CC = gcc
CFLAGS = -Wall -Werror -g -Iinclude
LDFLAGS =

BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin
TEST_DIR = $(BUILD_DIR)/test

# Source files
SRCS = $(wildcard src/*.c) \
       $(wildcard src/lexer/*.c) \
       $(wildcard src/parser/*.c) \
       $(wildcard src/symbol/*.c) \
       $(wildcard src/codegen/*.c) \
       $(wildcard src/cof/*.c) \
       $(wildcard src/util/*.c)

# Object files
OBJS = $(SRCS:src/%.c=$(OBJ_DIR)/%.o)

# Target executable
TARGET = $(BIN_DIR)/c2coil

# Phony targets
.PHONY: all clean test dirs

# Default target
all: dirs $(TARGET)

# Create directories
dirs:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(OBJ_DIR)/lexer
	@mkdir -p $(OBJ_DIR)/parser
	@mkdir -p $(OBJ_DIR)/symbol
	@mkdir -p $(OBJ_DIR)/codegen
	@mkdir -p $(OBJ_DIR)/cof
	@mkdir -p $(OBJ_DIR)/util
	@mkdir -p $(TEST_DIR)

# Build the target executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Compile source files
$(OBJ_DIR)/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -rf $(BUILD_DIR)

# Run tests
test: all
	@mkdir -p $(TEST_DIR)
	$(TARGET) test/test.c $(TEST_DIR)/test.cof
	@echo "Test compilation completed successfully."