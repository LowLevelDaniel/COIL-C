/**
 * @file expr_gen.c
 * @brief Expression code generation for the COIL C Compiler
 * @details Implements expression-specific code generation functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "codegen_internal.h"  /* Include internal function declarations */
#include "cof.h"
#include "coil_constants.h"
#include "symbol.h"

/* Note: The generate_expression function is implemented in codegen.c */

/**
 * @brief Generate COIL code for a subscript expression (array access)
 *
 * @param generator Pointer to the generator
 * @param expr Subscript expression to generate code for
 * @param dest_reg Destination register for the result
 * @return Register containing the result, or -1 on error
 */
int generate_subscript_expr(CodeGenerator *generator, Expression *expr, int dest_reg) {
  FILE *output;
  int array_reg;
  int index_reg;
  
  /* Validate input parameters */
  if (generator == NULL || expr == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to generate_subscript_expr\n");
    return -1;
  }
  
  output = generator->output;
  if (output == NULL) {
    fprintf(stderr, "Error: NULL output file in generate_subscript_expr\n");
    return -1;
  }
  
  /* Generate code for the array */
  array_reg = generate_expression(generator, expr->value.subscript.array, dest_reg);
  if (array_reg < 0) {
    return -1;
  }
  
  /* Generate code for the index */
  index_reg = generate_expression(generator, expr->value.subscript.index, dest_reg + 1);
  if (index_reg < 0) {
    return -1;
  }
  
  /* Multiply index by element size (assuming 4 bytes for now) */
  emit_movi(output, dest_reg + 2, 4);
  emit_mul(output, dest_reg + 1, index_reg, dest_reg + 2);
  
  /* Add the base address and the offset */
  emit_add(output, dest_reg + 1, array_reg, dest_reg + 1);
  
  /* Load from the calculated address */
  /* This is very simplified - real array access would be more complex */
  emit_load(output, dest_reg, dest_reg + 1, 0);
  
  return dest_reg;
}

/**
 * @brief Generate COIL code for a cast expression
 *
 * @param generator Pointer to the generator
 * @param expr Cast expression to generate code for
 * @param dest_reg Destination register for the result
 * @return Register containing the result, or -1 on error
 */
int generate_cast_expr(CodeGenerator *generator, Expression *expr, int dest_reg) {
  FILE *output;
  int operand_reg;
  
  /* Validate input parameters */
  if (generator == NULL || expr == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to generate_cast_expr\n");
    return -1;
  }
  
  output = generator->output;
  if (output == NULL) {
    fprintf(stderr, "Error: NULL output file in generate_cast_expr\n");
    return -1;
  }
  
  /* Generate code for the expression being cast */
  operand_reg = generate_expression(generator, expr->value.cast.expression, dest_reg);
  if (operand_reg < 0) {
    return -1;
  }
  
  /* Determine the cast operation based on source and destination types */
  if (expr->value.cast.expression->data_type->base_type == TYPE_INT && 
      expr->value.cast.cast_type->base_type == TYPE_FLOAT) {
    /* Integer to float conversion */
    emit_instruction(output, OP_ITOF, 0x00, 0x02);
    emit_register_operand(output, COIL_TYPE_FLOAT, 0x04, dest_reg);
    emit_register_operand(output, COIL_TYPE_INT, 0x04, operand_reg);
  } 
  else if (expr->value.cast.expression->data_type->base_type == TYPE_FLOAT && 
           expr->value.cast.cast_type->base_type == TYPE_INT) {
    /* Float to integer conversion */
    emit_instruction(output, OP_FTOI, 0x00, 0x02);
    emit_register_operand(output, COIL_TYPE_INT, 0x04, dest_reg);
    emit_register_operand(output, COIL_TYPE_FLOAT, 0x04, operand_reg);
  } 
  else if (expr->value.cast.expression->data_type->base_type == TYPE_CHAR && 
           expr->value.cast.cast_type->base_type == TYPE_INT) {
    /* Char to integer - already the right size, just need to copy */
    if (dest_reg != operand_reg) {
      emit_mov(output, dest_reg, operand_reg);
    }
  } 
  else if (expr->value.cast.expression->data_type->base_type == TYPE_INT && 
           expr->value.cast.cast_type->base_type == TYPE_CHAR) {
    /* Integer to char - truncate the value */
    emit_instruction(output, OP_TRUNC, 0x00, 0x03);
    emit_register_operand(output, COIL_TYPE_INT, 0x01, dest_reg); /* 1 byte */
    emit_register_operand(output, COIL_TYPE_INT, 0x04, operand_reg); /* 4 bytes */
    emit_immediate_operand_i32(output, COIL_TYPE_INT, 0xFF); /* Mask for 8 bits */
  } 
  else {
    /* No specific conversion needed, just copy the value */
    if (dest_reg != operand_reg) {
      emit_mov(output, dest_reg, operand_reg);
    }
  }
  
  return dest_reg;
}

/**
 * @brief Generate COIL code for a literal character expression
 *
 * @param generator Pointer to the generator
 * @param expr Character literal expression to generate code for
 * @param dest_reg Destination register for the result
 * @return Register containing the result, or -1 on error
 */
int generate_literal_char_expr(CodeGenerator *generator, Expression *expr, int dest_reg) {
  FILE *output;
  
  /* Validate input parameters */
  if (generator == NULL || expr == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to generate_literal_char_expr\n");
    return -1;
  }
  
  output = generator->output;
  if (output == NULL) {
    fprintf(stderr, "Error: NULL output file in generate_literal_char_expr\n");
    return -1;
  }
  
  /* Load the character value as an immediate value */
  emit_movi(output, dest_reg, (int)expr->value.char_value);
  
  return dest_reg;
}

/**
 * @brief Generate COIL code for a unary expression
 *
 * @param generator Pointer to the generator
 * @param expr Unary expression to generate code for
 * @param dest_reg Destination register for the result
 * @return Register containing the result, or -1 on error
 */
int generate_unary_expr(CodeGenerator *generator, Expression *expr, int dest_reg) {
  FILE *output;
  int operand_reg;
  int true_label;
  int end_label;
  
  /* Validate input parameters */
  if (generator == NULL || expr == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to generate_unary_expr\n");
    return -1;
  }
  
  output = generator->output;
  if (output == NULL) {
    fprintf(stderr, "Error: NULL output file in generate_unary_expr\n");
    return -1;
  }
  
  /* Generate code for the operand */
  operand_reg = generate_expression(generator, expr->value.unary.operand, dest_reg);
  if (operand_reg < 0) {
    return -1;
  }
  
  /* Perform the unary operation */
  switch (expr->value.unary.operator) {
    case TOKEN_MINUS:
      /* Negate the value (0 - value) */
      emit_movi(output, REG_RQ1, 0);
      emit_sub(output, dest_reg, REG_RQ1, operand_reg);
      break;
      
    case TOKEN_NOT:
      /* Logical NOT (compare with 0) */
      emit_movi(output, REG_RQ1, 0);
      emit_cmp(output, operand_reg, REG_RQ1);
      
      /* Create labels for true (value is 0) and end */
      true_label = generator_next_label(generator);
      end_label = generator_next_label(generator);
      
      emit_jcc(output, BR_EQ, true_label);
      
      /* Value is not 0, so result is 0 */
      emit_movi(output, dest_reg, 0);
      emit_jmp(output, end_label);
      
      /* Value is 0, so result is 1 */
      emit_label(output, true_label);
      emit_movi(output, dest_reg, 1);
      
      emit_label(output, end_label);
      break;
      
    case TOKEN_BIT_NOT:
      /* Bitwise NOT (XOR with -1) */
      emit_movi(output, REG_RQ1, -1);
      emit_instruction(output, OP_XOR, 0x00, 0x03);
      emit_register_operand(output, COIL_TYPE_INT, 0x04, dest_reg);
      emit_register_operand(output, COIL_TYPE_INT, 0x04, operand_reg);
      emit_register_operand(output, COIL_TYPE_INT, 0x04, REG_RQ1);
      break;
      
    default:
      fprintf(stderr, "Error: Unsupported unary operator: %d\n", expr->value.unary.operator);
      return -1;
  }
  
  return dest_reg;
}