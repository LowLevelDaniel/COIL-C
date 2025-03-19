/**
 * Enhanced Main Compiler Driver for the COIL C Compiler
 * Handles the entire compilation pipeline from C source to COIL binary
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

// Configuration options
typedef struct {
    bool verbose;
    bool dump_ast;
    bool dump_symbols;
    bool optimize;
    int optimization_level;
    const char *output_file;
} CompilerOptions;

/**
 * Print usage information
 */
void print_usage(const char *program_name) {
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
 * Parse command line arguments
 */
void parse_args(int argc, char *argv[], CompilerOptions *options, const char **input_file) {
    // Set default options
    options->verbose = false;
    options->dump_ast = false;
    options->dump_symbols = false;
    options->optimize = false;
    options->optimization_level = 0;
    options->output_file = NULL;
    *input_file = NULL;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            options->verbose = true;
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--dump-ast") == 0) {
            options->dump_ast = true;
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--dump-symbols") == 0) {
            options->dump_symbols = true;
        } else if (strncmp(argv[i], "-O", 2) == 0) {
            options->optimize = true;
            options->optimization_level = argv[i][2] - '0';
            if (options->optimization_level < 0 || options->optimization_level > 3) {
                fprintf(stderr, "Invalid optimization level: %c\n", argv[i][2]);
                options->optimization_level = 0;
            }
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                options->output_file = argv[++i];
            } else {
                fprintf(stderr, "Error: Missing output file name after -o\n");
                exit(1);
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            exit(0);
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Error: Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            exit(1);
        } else {
            // Not an option, must be the input file
            if (*input_file == NULL) {
                *input_file = argv[i];
            } else if (options->output_file == NULL) {
                // If output file is not set with -o, use the second positional argument
                options->output_file = argv[i];
            } else {
                fprintf(stderr, "Error: Too many file arguments\n");
                print_usage(argv[0]);
                exit(1);
            }
        }
    }
    
    // Check if input file is provided
    if (*input_file == NULL) {
        fprintf(stderr, "Error: No input file specified\n");
        print_usage(argv[0]);
        exit(1);
    }
    
    // If output file is not specified, derive it from input file
    if (options->output_file == NULL) {
        // Find the base name (without directories)
        const char *base_name = *input_file;
        const char *slash = strrchr(*input_file, '/');
        if (slash != NULL) {
            base_name = slash + 1;
        }
        
        // Remove .c extension if present
        char *output_name = strdup(base_name);
        char *dot = strrchr(output_name, '.');
        if (dot != NULL && strcmp(dot, ".c") == 0) {
            *dot = '\0';
        }
        
        // Add .cof extension
        char *result = malloc(strlen(output_name) + 5); // +5 for ".cof" and null terminator
        if (result == NULL) {
            fprintf(stderr, "Memory allocation failed for output file name\n");
            exit(1);
        }
        
        sprintf(result, "%s.cof", output_name);
        options->output_file = result;
        
        free(output_name);
    }
}

/**
 * Read the entire contents of a file
 */
char *read_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Failed to open input file: %s\n", filename);
        exit(1);
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate memory for the file contents
    char *source = malloc(file_size + 1);
    if (source == NULL) {
        fprintf(stderr, "Memory allocation failed for file contents\n");
        fclose(file);
        exit(1);
    }
    
    // Read the file
    size_t bytes_read = fread(source, 1, file_size, file);
    source[bytes_read] = '\0';
    
    fclose(file);
    return source;
}

/**
 * Simple AST dumper for debugging
 */
void dump_ast(Program *program) {
    printf("Program:\n");
    printf("  Functions: %d\n", program->function_count);
    
    for (int i = 0; i < program->function_count; i++) {
        Function *function = program->functions[i];
        printf("  Function: %s\n", function->name);
        printf("    Parameters: %d\n", function->parameter_count);
        
        for (int j = 0; j < function->parameter_count; j++) {
            char *type_str = type_to_string(function->parameter_types[j]);
            printf("      %s %s\n", type_str, function->parameter_names[j]);
            free(type_str);
        }
        
        char *return_type_str = type_to_string(function->return_type);
        printf("    Return type: %s\n", return_type_str);
        free(return_type_str);
        
        printf("    Body: [complex statement structure]\n");
    }
}

/**
 * Main compiler driver
 */
int main(int argc, char *argv[]) {
    // Parse command line arguments
    CompilerOptions options;
    const char *input_file;
    parse_args(argc, argv, &options, &input_file);
    
    // Start measuring compilation time
    clock_t start_time = clock();
    
    if (options.verbose) {
        printf("Compiling %s -> %s\n", input_file, options.output_file);
    }
    
    // Read the input file
    char *source = read_file(input_file);
    
    // Initialize lexer
    Lexer lexer;
    lexer_init(&lexer, source);
    
    if (options.verbose) {
        printf("Parsing...\n");
    }
    
    // Parse the program
    Program *program = parse_program(&lexer);
    
    // Dump AST if requested
    if (options.dump_ast) {
        printf("\n--- Abstract Syntax Tree ---\n");
        dump_ast(program);
    }
    
    if (options.verbose) {
        printf("Generating COIL code...\n");
    }
    
    // Generate COIL code
    generate_program(program, options.output_file);
    
    // Free resources
    free(source);
    free_program(program);
    
    // Calculate and report compilation time
    clock_t end_time = clock();
    double compilation_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    if (options.verbose) {
        printf("Compilation successful (%.3f seconds)\n", compilation_time);
    } else {
        printf("Compilation successful: %s -> %s\n", input_file, options.output_file);
    }
    
    return 0;
}