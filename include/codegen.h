/**
 * @file codegen.h
 * @brief Code generation for the COIL C Compiler
 *
 * Defines interfaces for generating COIL binary code from the AST.
 */

#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "ast.h"
#include "symbol.h"

/**
 * Code generator state
 */
typedef struct {
    FILE *output;              /**< Output file for code generation */
    SymbolTable *current_scope; /**< Current symbol table scope */
    int next_label;            /**< Next available label number */
    int stack_offset;          /**< Current stack offset for local variables */
    int optimization_level;    /**< Optimization level (0-3) */
} CodeGenerator;

/**
 * Initialize a code generator
 * @param generator Pointer to the generator to initialize
 * @param output Output file for code
 */
void generator_init(CodeGenerator *generator, FILE *output);

/**
 * Set the optimization level for the code generator
 * @param generator Pointer to the generator
 * @param level Optimization level (0-3)
 */
void generator_set_optimization(CodeGenerator *generator, int level);

/**
 * Generate COIL code for an expression
 * @param generator Pointer to the generator
 * @param expr Expression to generate code for
 * @param dest_reg Destination register for the result
 * @return Register containing the result
 */
int generate_expression(CodeGenerator *generator, Expression *expr, int dest_reg);

/**
 * Generate COIL code for a statement
 * @param generator Pointer to the generator
 * @param stmt Statement to generate code for
 */
void generate_statement(CodeGenerator *generator, Statement *stmt);

/**
 * Generate COIL code for a function
 * @param generator Pointer to the generator
 * @param function Function to generate code for
 */
void generate_function(CodeGenerator *generator, Function *function);

/**
 * Generate a COIL program and write to the specified file
 * @param program Program to generate code for
 * @param output_file Output file path
 * @param optimization_level Optimization level (0-3)
 */
void generate_program(Program *program, const char *output_file);

/**
 * Get the next label number for local jumps/branches
 * @param generator Pointer to the generator
 * @return Next unique label number
 */
int generator_next_label(CodeGenerator *generator);

/**
 * Allocate stack space for a local variable
 * @param generator Pointer to the generator
 * @param type Type of the variable
 * @return Stack offset for the variable
 */
int allocate_local_variable(CodeGenerator *generator, Type *type);

/**
 * Generate COIL code for function prologue
 * @param generator Pointer to the generator
 * @param function Function to generate prologue for
 */
void emit_function_prologue(CodeGenerator *generator, Function *function);

/**
 * Generate COIL code for function epilogue
 * @param generator Pointer to the generator
 */
void emit_function_epilogue(CodeGenerator *generator);

/**
 * Generate COIL code for a binary operation
 * @param generator Pointer to the generator
 * @param expr Binary expression to generate code for
 * @param dest_reg Destination register for the result
 * @return Register containing the result
 */
int generate_binary_operation(CodeGenerator *generator, Expression *expr, int dest_reg);

/**
 * Generate COIL code for a function call
 * @param generator Pointer to the generator
 * @param expr Function call expression to generate code for
 * @param dest_reg Destination register for the result
 * @return Register containing the result
 */
int generate_function_call(CodeGenerator *generator, Expression *expr, int dest_reg);

/**
 * Generate COIL code for a variable reference
 * @param generator Pointer to the generator
 * @param expr Variable reference expression to generate code for
 * @param dest_reg Destination register for the result
 * @return Register containing the result
 */
int generate_variable_reference(CodeGenerator *generator, Expression *expr, int dest_reg);

/**
 * Generate COIL code for a variable assignment
 * @param generator Pointer to the generator
 * @param expr Assignment expression to generate code for
 * @param dest_reg Destination register for the result
 * @return Register containing the result
 */
int generate_variable_assignment(CodeGenerator *generator, Expression *expr, int dest_reg);

#endif /* CODEGEN_H */