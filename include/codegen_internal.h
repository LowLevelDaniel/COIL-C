/**
 * @file codegen_internal.h
 * @brief Internal functions for code generation in the COIL C Compiler
 * @details This file contains declarations for functions used internally
 * by the code generation modules but not exposed in the public API.
 */

#ifndef CODEGEN_INTERNAL_H
#define CODEGEN_INTERNAL_H

#include "codegen.h"
#include "ast.h"

/**
 * @brief Generate COIL code for a subscript expression (array access)
 *
 * @param generator Pointer to the generator
 * @param expr Subscript expression to generate code for
 * @param dest_reg Destination register for the result
 * @return Register containing the result, or -1 on error
 */
int generate_subscript_expr(CodeGenerator *generator, Expression *expr, int dest_reg);

/**
 * @brief Generate COIL code for a cast expression
 *
 * @param generator Pointer to the generator
 * @param expr Cast expression to generate code for
 * @param dest_reg Destination register for the result
 * @return Register containing the result, or -1 on error
 */
int generate_cast_expr(CodeGenerator *generator, Expression *expr, int dest_reg);

/**
 * @brief Generate COIL code for a literal character expression
 *
 * @param generator Pointer to the generator
 * @param expr Character literal expression to generate code for
 * @param dest_reg Destination register for the result
 * @return Register containing the result, or -1 on error
 */
int generate_literal_char_expr(CodeGenerator *generator, Expression *expr, int dest_reg);

/**
 * @brief Generate COIL code for a unary expression
 *
 * @param generator Pointer to the generator
 * @param expr Unary expression to generate code for
 * @param dest_reg Destination register for the result
 * @return Register containing the result, or -1 on error
 */
int generate_unary_expr(CodeGenerator *generator, Expression *expr, int dest_reg);

#endif /* CODEGEN_INTERNAL_H */