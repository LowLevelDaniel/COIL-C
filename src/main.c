/**
 * @file main.c
 * @brief Main Compiler Driver for the COIL C Compiler
 * @details Handles the entire compilation pipeline from C source to COIL binary
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "codegen.h"
#include "coil_constants.h"

/**
 * @brief Configuration options for the compiler
 */
typedef struct {
  bool verbose;              /**< Verbose output flag */
  bool dump_ast;             /**< Dump AST flag */
  bool dump_symbols;         /**< Dump symbol table flag */
  bool optimize;             /**< Optimization flag */
  int optimization_level;    /**< Optimization level (0-3) */
  const char *output_file;   /**< Output file path */
} CompilerOptions;

/**
 * @brief Print usage information
 *
 * @param program_name Name of the program executable
 */
void print_usage(const char *program_name) {
  if (program_name == NULL) {
    fprintf(stderr, "Error: NULL program name passed to print_usage\n");
    return;
  }
  
  printf("Usage: %s [options] <input.c> [output.cof]\n", program_name);
  printf("\nOptions:\n");
  printf("  -v, --verbose        Print verbose information during compilation\n");
  printf("  -d, --dump-ast       Dump the Abstract Syntax Tree (AST)\n");
  printf("  -s, --dump-symbols   Dump the symbol table\n");
  printf("  -O<n>                Set optimization level (0-3)\n");
  printf("  -o <file>            Specify output file name\n");
  printf("  -h, --help           Show this help message\n");
}

/**
 * @brief Parse command line arguments
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @param options Pointer to store parsed options
 * @param input_file Pointer to store input file path
 * @return int 0 on success, non-zero on failure
 */
int parse_args(int argc, char *argv[], CompilerOptions *options, const char **input_file) {
  int i;
  char *output_name = NULL;
  char *result = NULL;
  const char *base_name = NULL;
  const char *slash = NULL;
  char *dot = NULL;
  
  /* Validate input parameters */
  if (options == NULL || input_file == NULL || argv == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to parse_args\n");
    return -1;
  }
  
  /* Set default options */
  options->verbose = false;
  options->dump_ast = false;
  options->dump_symbols = false;
  options->optimize = false;
  options->optimization_level = 0;
  options->output_file = NULL;
  *input_file = NULL;
  
  /* Parse arguments */
  for (i = 1; i < argc; i++) {
    if (argv[i] == NULL) {
      fprintf(stderr, "Error: NULL argument at index %d\n", i);
      return -1;
    }
    
    if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
      options->verbose = true;
    } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--dump-ast") == 0) {
      options->dump_ast = true;
    } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--dump-symbols") == 0) {
      options->dump_symbols = true;
    } else if (strncmp(argv[i], "-O", 2) == 0) {
      options->optimize = true;
      if (argv[i][2] >= '0' && argv[i][2] <= '3') {
        options->optimization_level = argv[i][2] - '0';
      } else {
        fprintf(stderr, "Warning: Invalid optimization level: %c (using 0)\n", argv[i][2]);
        options->optimization_level = 0;
      }
    } else if (strcmp(argv[i], "-o") == 0) {
      if (i + 1 < argc) {
        options->output_file = argv[++i];
      } else {
        fprintf(stderr, "Error: Missing output file name after -o\n");
        return -1;
      }
    } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      print_usage(argv[0]);
      exit(0);
    } else if (argv[i][0] == '-') {
      fprintf(stderr, "Error: Unknown option: %s\n", argv[i]);
      print_usage(argv[0]);
      return -1;
    } else {
      /* Not an option, must be the input file */
      if (*input_file == NULL) {
        *input_file = argv[i];
      } else if (options->output_file == NULL) {
        /* If output file is not set with -o, use the second positional argument */
        options->output_file = argv[i];
      } else {
        fprintf(stderr, "Error: Too many file arguments\n");
        print_usage(argv[0]);
        return -1;
      }
    }
  }
  
  /* Check if input file is provided */
  if (*input_file == NULL) {
    fprintf(stderr, "Error: No input file specified\n");
    print_usage(argv[0]);
    return -1;
  }
  
  /* If output file is not specified, derive it from input file */
  if (options->output_file == NULL) {
    /* Find the base name (without directories) */
    base_name = *input_file;
    slash = strrchr(*input_file, '/');
    if (slash != NULL) {
      base_name = slash + 1;
    }
    
    /* Remove .c extension if present */
    output_name = strdup(base_name);
    if (output_name == NULL) {
      fprintf(stderr, "Error: Memory allocation failed for output file name\n");
      return -1;
    }
    
    dot = strrchr(output_name, '.');
    if (dot != NULL && strcmp(dot, ".c") == 0) {
      *dot = '\0';
    }
    
    /* Add .cof extension */
    result = malloc(strlen(output_name) + 5); /* +5 for ".cof" and null terminator */
    if (result == NULL) {
      fprintf(stderr, "Error: Memory allocation failed for output file name\n");
      free(output_name);
      return -1;
    }
    
    sprintf(result, "%s.cof", output_name);
    options->output_file = result;
    
    free(output_name);
  }
  
  return 0;
}

/**
 * @brief Read the entire contents of a file
 *
 * @param filename Path to the file to read
 * @return char* File contents as a null-terminated string, or NULL on error
 */
char *read_file(const char *filename) {
  FILE *file;
  long file_size;
  char *source;
  size_t bytes_read;
  
  /* Validate input parameter */
  if (filename == NULL) {
    fprintf(stderr, "Error: NULL filename passed to read_file\n");
    return NULL;
  }
  
  file = fopen(filename, "r");
  if (file == NULL) {
    fprintf(stderr, "Error: Failed to open input file: %s\n", filename);
    return NULL;
  }
  
  /* Get file size */
  if (fseek(file, 0, SEEK_END) != 0) {
    fprintf(stderr, "Error: Failed to seek to end of file: %s\n", filename);
    fclose(file);
    return NULL;
  }
  
  file_size = ftell(file);
  if (file_size < 0) {
    fprintf(stderr, "Error: Failed to get file size: %s\n", filename);
    fclose(file);
    return NULL;
  }
  
  if (fseek(file, 0, SEEK_SET) != 0) {
    fprintf(stderr, "Error: Failed to seek to start of file: %s\n", filename);
    fclose(file);
    return NULL;
  }
  
  /* Allocate memory for the file contents */
  source = malloc(file_size + 1);
  if (source == NULL) {
    fprintf(stderr, "Error: Memory allocation failed for file contents: %s\n", filename);
    fclose(file);
    return NULL;
  }
  
  /* Read the file */
  bytes_read = fread(source, 1, file_size, file);
  source[bytes_read] = '\0';
  
  fclose(file);
  return source;
}

/**
 * @brief Simple AST dumper for debugging
 *
 * @param program Program AST to dump
 */
void dump_ast(Program *program) {
  int i, j;
  char *type_str;
  char *return_type_str;
  
  /* Validate input parameter */
  if (program == NULL) {
    fprintf(stderr, "Error: NULL program passed to dump_ast\n");
    return;
  }
  
  printf("Program:\n");
  printf("  Functions: %d\n", program->function_count);
  
  for (i = 0; i < program->function_count; i++) {
    Function *function = program->functions[i];
    if (function == NULL) {
      fprintf(stderr, "Error: NULL function at index %d\n", i);
      continue;
    }
    
    printf("  Function: %s\n", function->name ? function->name : "(null)");
    printf("    Parameters: %d\n", function->parameter_count);
    
    for (j = 0; j < function->parameter_count; j++) {
      if (function->parameter_types[j] == NULL || function->parameter_names[j] == NULL) {
        fprintf(stderr, "Error: NULL parameter type or name at index %d\n", j);
        continue;
      }
      
      type_str = type_to_string(function->parameter_types[j]);
      if (type_str == NULL) {
        fprintf(stderr, "Error: Failed to get type string for parameter %d\n", j);
        continue;
      }
      
      printf("      %s %s\n", type_str, function->parameter_names[j]);
      free(type_str);
    }
    
    if (function->return_type == NULL) {
      fprintf(stderr, "Error: NULL return type for function %s\n", function->name);
      continue;
    }
    
    return_type_str = type_to_string(function->return_type);
    if (return_type_str == NULL) {
      fprintf(stderr, "Error: Failed to get return type string for function %s\n", function->name);
      continue;
    }
    
    printf("    Return type: %s\n", return_type_str);
    free(return_type_str);
    
    printf("    Body: [complex statement structure]\n");
  }
}

/**
 * @brief Main compiler driver function
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @return int Exit code: 0 on success, non-zero on failure
 */
int main(int argc, char *argv[]) {
  /* Compiler variables */
  CompilerOptions options;
  const char *input_file = NULL;
  char *source = NULL;
  Lexer lexer;
  Program *program = NULL;
  int result;
  
  /* Timing variables */
  clock_t start_time, end_time;
  double compilation_time;
  
  /* Parse command line arguments */
  result = parse_args(argc, argv, &options, &input_file);
  if (result != 0) {
    return result;
  }
  
  /* Start measuring compilation time */
  start_time = clock();
  
  if (options.verbose) {
    printf("Compiling %s -> %s\n", input_file, options.output_file);
  }
  
  /* Read the input file */
  source = read_file(input_file);
  if (source == NULL) {
    return -1;
  }
  
  /* Initialize lexer */
  lexer_init(&lexer, source);
  
  if (options.verbose) {
    printf("Parsing...\n");
  }
  
  /* Parse the program */
  program = parse_program(&lexer);
  if (program == NULL) {
    fprintf(stderr, "Error: Parsing failed\n");
    free(source);
    return -1;
  }
  
  /* Dump AST if requested */
  if (options.dump_ast) {
    printf("\n--- Abstract Syntax Tree ---\n");
    dump_ast(program);
  }
  
  if (options.verbose) {
    printf("Generating COIL code...\n");
  }
  
  /* Generate COIL code */
  result = generate_program(program, options.output_file);
  if (result != 0) {
    fprintf(stderr, "Error: Code generation failed\n");
    free(source);
    free_program(program);
    return result;
  }
  
  /* Free resources */
  free(source);
  free_program(program);
  
  /* Calculate and report compilation time */
  end_time = clock();
  compilation_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
  
  if (options.verbose) {
    printf("Compilation successful (%.3f seconds)\n", compilation_time);
  } else {
    printf("Compilation successful: %s -> %s\n", input_file, options.output_file);
  }
  
  return 0;
}