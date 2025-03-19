/**
* Code generation for the COIL C Compiler
*/

#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "ast.h"
#include "symbol.h"

typedef struct {
  FILE *output;
  SymbolTable *current_scope;
  int next_label;
  int stack_offset;
} CodeGenerator;

/**
* Initialize a code generator
*/
void generator_init(CodeGenerator *generator, FILE *output);

/**
* Generate COIL code for an expression
* Returns the register containing the result
*/
int generate_expression(CodeGenerator *generator, Expression *expr, int dest_reg);

/**
* Generate COIL code for a statement
*/
void generate_statement(CodeGenerator *generator, Statement *stmt);

/**
* Generate COIL code for a function
*/
void generate_function(CodeGenerator *generator, Function *function);

/**
* Generate a COIL program and write to the specified file
*/
void generate_program(Program *program, const char *output_file);

/**
* Get the next label number for local jumps/branches
*/
int generator_next_label(CodeGenerator *generator);

#endif /* CODEGEN_H */