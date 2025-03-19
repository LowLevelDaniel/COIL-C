/**
* C to COIL Compiler (c2coil)
* 
* A basic C compiler that targets COIL v1.0.0 as its backend
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "codegen.h"
#include "coil_constants.h"

int main(int argc, char *argv[]) {
  if (argc != 3) {
      fprintf(stderr, "Usage: %s <input.c> <output.cof>\n", argv[0]);
      return 1;
  }
  
  // Read input file
  FILE *input_file = fopen(argv[1], "r");
  if (input_file == NULL) {
      fprintf(stderr, "Failed to open input file: %s\n", argv[1]);
      return 1;
  }
  
  // Get file size
  fseek(input_file, 0, SEEK_END);
  long file_size = ftell(input_file);
  fseek(input_file, 0, SEEK_SET);
  
  // Read file contents
  char *source = malloc(file_size + 1);
  size_t read_size = fread(source, 1, file_size, input_file);
  source[read_size] = '\0';
  fclose(input_file);
  
  // Initialize lexer
  Lexer lexer;
  lexer_init(&lexer, source);
  
  // Parse the program
  Program *program = parse_program(&lexer);
  
  // Generate COIL code
  generate_program(program, argv[2]);
  
  // Clean up
  free(source);
  // free_program(program); // Uncomment when free_program is implemented
  
  printf("Compilation successful: %s -> %s\n", argv[1], argv[2]);
  return 0;
}