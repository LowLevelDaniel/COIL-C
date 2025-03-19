CC = gcc
CFLAGS = -Wall -Werror -Wextra -g -Iinclude
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

# Test files
TEST_SRCS = $(wildcard test/*.c)
TEST_BINS = $(TEST_SRCS:test/%.c=$(TEST_DIR)/%.cof)

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
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -rf $(BUILD_DIR)

# Run tests
test: all $(TEST_BINS)
	@echo "All tests compiled successfully."

# Compile test files
$(TEST_DIR)/%.cof: test/%.c $(TARGET)
	$(TARGET) $< -o $@

# Run a specific test
run-test-%: $(TEST_DIR)/%.cof
	@echo "Test output for $*:"
	@hexdump -C $<

# Generate assembly for a test file
asm-%: test/%.c $(TARGET)
	$(TARGET) $< -o $(TEST_DIR)/$*.cof --dump-asm

# Special target for the example test
test-example: $(TARGET)
	$(TARGET) test/test.c -o $(TEST_DIR)/test.cof -v
	@echo "Test compilation completed successfully."
	@ls -la $(TEST_DIR)/test.cof

# Development tools
format:
	find src include -name "*.h" -o -name "*.c" | xargs clang-format -i

# Help target
help:
	@echo "Available targets:"
	@echo "  all        - Build the compiler (default)"
	@echo "  clean      - Remove build artifacts"
	@echo "  test       - Compile all test files"
	@echo "  run-test-X - Run specific test X (e.g., run-test-test)"
	@echo "  asm-X      - Generate assembly for test X"
	@echo "  format     - Format code using clang-format"
	@echo "  help       - Show this help message"