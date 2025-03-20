/**
 * @file main.c
 * @brief Main entry point for COIL C Compiler
 */

#include "../include/colc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage() {
  printf("Usage: colc [options] input_file\n");
  printf("Options:\n");
  printf("  -o <file>   Set output file (default: output.cof)\n");
  printf("  -O<level>   Set optimization level (0-3, default: 1)\n");
  printf("  -v          Enable verbose output\n");
  printf("  -ast        Print AST\n");
  printf("  -tokens     Print tokens\n");
  printf("  -g          Generate debug information\n");
  printf("  -h, --help  Show this help message\n");
  printf("  --version   Show version information\n");
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    print_usage();
    return 1;
  }
  
  CompilerOptions options = compiler_default_options();
  
  // Parse command line arguments
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
      options.output_file = argv[++i];
    } else if (strncmp(argv[i], "-O", 2) == 0) {
      options.optimization_level = atoi(argv[i] + 2);
      if (options.optimization_level < 0 || options.optimization_level > 3) {
        printf("Invalid optimization level: %s\n", argv[i] + 2);
        return 1;
      }
    } else if (strcmp(argv[i], "-v") == 0) {
      options.verbose = true;
    } else if (strcmp(argv[i], "-ast") == 0) {
      options.print_ast = true;
    } else if (strcmp(argv[i], "-tokens") == 0) {
      options.print_tokens = true;
    } else if (strcmp(argv[i], "-g") == 0) {
      options.emit_debug_info = true;
    } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      print_usage();
      return 0;
    } else if (strcmp(argv[i], "--version") == 0) {
      compiler_print_version();
      return 0;
    } else if (argv[i][0] == '-') {
      printf("Unknown option: %s\n", argv[i]);
      print_usage();
      return 1;
    } else {
      options.input_file = argv[i];
    }
  }
  
  // Check if input file is specified
  if (!options.input_file) {
    printf("No input file specified\n");
    print_usage();
    return 1;
  }
  
  // Compile file
  bool success = compiler_compile_file(options);
  
  if (!success) {
    const char* error = compiler_error();
    if (error) {
      printf("Compilation failed: %s\n", error);
    } else {
      printf("Compilation failed with unknown error\n");
    }
    return 1;
  }
  
  if (options.verbose) {
    printf("Successfully compiled '%s' to '%s'\n", options.input_file, options.output_file);
  }
  
  return 0;
}