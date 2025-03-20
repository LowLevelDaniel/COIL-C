/**
 * @file colc.c
 * @brief COIL C Compiler main implementation
 */

#include "../include/colc.h"
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/ast.h"
#include "../include/semantic.h"
#include "../include/codegen.h"
#include "../include/arena.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COLC_VERSION "0.1.0"
#define ARENA_INITIAL_SIZE (1024 * 1024) // 1MB

// Global error message
static char error_message[256] = {0};

CompilerOptions compiler_default_options() {
  CompilerOptions options;
  options.input_file = NULL;
  options.output_file = "output.cof";
  options.print_ast = false;
  options.print_tokens = false;
  options.optimize = true;
  options.optimization_level = 1;
  options.emit_debug_info = false;
  options.verbose = false;
  return options;
}

static char* read_file(const char* path) {
  FILE* file = fopen(path, "rb");
  if (!file) {
    snprintf(error_message, sizeof(error_message), "Could not open file '%s'", path);
    return NULL;
  }
  
  // Get file size
  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  fseek(file, 0, SEEK_SET);
  
  // Allocate memory for file content
  char* buffer = (char*)malloc(size + 1);
  if (!buffer) {
    fclose(file);
    snprintf(error_message, sizeof(error_message), "Memory allocation failed for file '%s'", path);
    return NULL;
  }
  
  // Read file
  size_t bytes_read = fread(buffer, 1, size, file);
  fclose(file);
  
  if (bytes_read < size) {
    free(buffer);
    snprintf(error_message, sizeof(error_message), "Failed to read file '%s'", path);
    return NULL;
  }
  
  // Null-terminate
  buffer[size] = '\0';
  
  return buffer;
}

// Print AST for debugging
static void print_ast(Program* program) {
  printf("AST dump:\n");
  printf("Program with %d declarations\n", program->count);
  
  for (int i = 0; i < program->count; i++) {
    Decl* decl = program->declarations[i];
    
    printf("Declaration %d: ", i);
    
    if (decl->type == DECL_VAR) {
      printf("Variable '%s'\n", decl->name);
    } else if (decl->type == DECL_FUNC) {
      printf("Function '%s' with %d parameters\n", decl->name, 
             decl->declared_type->as.function.param_count);
    } else {
      printf("Other declaration\n");
    }
  }
}

bool compiler_compile_string(const char* source, const char* source_name, 
                             const char* output_file, CompilerOptions options) {
  if (!source || !source_name || !output_file) {
    snprintf(error_message, sizeof(error_message), "Invalid arguments to compiler_compile_string");
    return false;
  }
  
  Arena* arena = arena_create(ARENA_INITIAL_SIZE);
  if (!arena) {
    snprintf(error_message, sizeof(error_message), "Memory allocation failed");
    return false;
  }
  
  if (options.verbose) {
    printf("Compiling '%s' to '%s'\n", source_name, output_file);
  }
  
  // Initialize lexer
  Lexer* lexer = lexer_create(source, source_name, arena);
  if (!lexer) {
    snprintf(error_message, sizeof(error_message), "Failed to initialize lexer");
    arena_destroy(arena);
    return false;
  }
  
  // Print tokens if requested
  if (options.print_tokens) {
    printf("Tokens:\n");
    Token token;
    do {
      token = lexer_next_token(lexer);
      printf("Token: %d\n", token.type);
    } while (token.type != TOKEN_EOF);
    
    // Reset lexer
    lexer = lexer_create(source, source_name, arena);
  }
  
  // Initialize parser
  Parser* parser = parser_create(lexer, arena);
  if (!parser) {
    snprintf(error_message, sizeof(error_message), "Failed to initialize parser");
    arena_destroy(arena);
    return false;
  }
  
  // Parse program
  Program* program = parser_parse_program(parser);
  if (!program) {
    const char* err = parser_error(parser);
    if (err) {
      snprintf(error_message, sizeof(error_message), "Parse error: %s", err);
    } else {
      snprintf(error_message, sizeof(error_message), "Parse error: Unknown error");
    }
    arena_destroy(arena);
    return false;
  }
  
  // Check for parser errors
  const char* parse_err = parser_error(parser);
  if (parse_err) {
    snprintf(error_message, sizeof(error_message), "Parse error: %s", parse_err);
    arena_destroy(arena);
    return false;
  }
  
  // Print AST if requested
  if (options.print_ast) {
    print_ast(program);
  }
  
  // Open output file
  FILE* output = fopen(output_file, "wb");
  if (!output) {
    snprintf(error_message, sizeof(error_message), "Could not open output file '%s'", output_file);
    arena_destroy(arena);
    return false;
  }
  
  // Generate code
  CodeGenerator* codegen = codegen_create(program, output, arena);
  if (!codegen) {
    snprintf(error_message, sizeof(error_message), "Failed to initialize code generator");
    fclose(output);
    arena_destroy(arena);
    return false;
  }
  
  bool codegen_success = codegen_generate(codegen);
  
  // Close output file
  fclose(output);
  
  if (!codegen_success) {
    snprintf(error_message, sizeof(error_message), "Code generation failed: %s", 
             codegen->has_error ? codegen->error_message : "Unknown error");
    arena_destroy(arena);
    return false;
  }
  
  // Free memory
  arena_destroy(arena);
  
  if (options.verbose) {
    printf("Compilation successful\n");
  }
  
  return true;
}

bool compiler_compile_file(CompilerOptions options) {
  if (!options.input_file) {
    snprintf(error_message, sizeof(error_message), "No input file specified");
    return false;
  }
  
  char* source = read_file(options.input_file);
  if (!source) {
    return false; // Error message set by read_file
  }
  
  bool result = compiler_compile_string(source, options.input_file, 
                                      options.output_file, options);
  
  free(source);
  return result;
}

const char* compiler_error() {
  return error_message[0] ? error_message : NULL;
}

void compiler_print_version() {
  printf("COIL C Compiler (colc) version %s\n", COLC_VERSION);
  printf("Using COIL version 1.0.0\n");
  printf("Copyright (c) 2023 The COIL Team\n");
}