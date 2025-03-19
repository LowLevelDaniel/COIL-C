/**
* Expression code generation for the COIL C Compiler
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "cof.h"
#include "coil_constants.h"
#include "symbol.h"

int generate_expression(CodeGenerator *generator, Expression *expr, int dest_reg) {
  FILE *output = generator->output;
  
  switch (expr->type) {
      case EXPR_LITERAL_INT:
          emit_movi(output, dest_reg, expr->value.int_value);
          return dest_reg;
      
      case EXPR_LITERAL_FLOAT: {
          // For simplicity we're treating floats as integers in this example
          // A real implementation would use COIL_TYPE_FLOAT and proper float handling
          int int_val = (int)expr->value.float_value;
          emit_movi(output, dest_reg, int_val);
          return dest_reg;
      }
      
      case EXPR_BINARY: {
          int left_result = generate_expression(generator, expr->value.binary.left, dest_reg);
          int right_result = generate_expression(generator, expr->value.binary.right, dest_reg + 1);
          
          switch (expr->value.binary.operator) {
              case TOKEN_PLUS:
                  emit_add(output, dest_reg, left_result, right_result);
                  break;
              case TOKEN_MINUS:
                  emit_sub(output, dest_reg, left_result, right_result);
                  break;
              case TOKEN_MULTIPLY:
                  emit_mul(output, dest_reg, left_result, right_result);
                  break;
              case TOKEN_DIVIDE:
                  emit_div(output, dest_reg, left_result, right_result);
                  break;
                  
              case TOKEN_EQ:
              case TOKEN_NEQ:
              case TOKEN_LT:
              case TOKEN_LE:
              case TOKEN_GT:
              case TOKEN_GE: {
                  // Compare operands
                  emit_cmp(output, left_result, right_result);
                  
                  // Create labels for the true and end branches
                  int true_label = generator_next_label(generator);
                  int end_label = generator_next_label(generator);
                  
                  // Branch to true_label if condition is met
                  uint8_t condition;
                  switch (expr->value.binary.operator) {
                      case TOKEN_EQ:  condition = BR_EQ; break;
                      case TOKEN_NEQ: condition = BR_NE; break;
                      case TOKEN_LT:  condition = BR_LT; break;
                      case TOKEN_LE:  condition = BR_LE; break;
                      case TOKEN_GT:  condition = BR_GT; break;
                      case TOKEN_GE:  condition = BR_GE; break;
                      default: condition = BR_ALWAYS; break;
                  }
                  
                  emit_jcc(output, condition, true_label);
                  
                  // False branch - load 0
                  emit_movi(output, dest_reg, 0);
                  emit_jmp(output, end_label);
                  
                  // True branch - load 1
                  emit_label(output, true_label);
                  emit_movi(output, dest_reg, 1);
                  
                  // End label
                  emit_label(output, end_label);
                  break;
              }
              
              default:
                  fprintf(stderr, "Unsupported binary operator\n");
                  exit(1);
          }
          
          return dest_reg;
      }
      
      case EXPR_VARIABLE: {
          Symbol *symbol = symbol_table_lookup(generator->current_scope, expr->value.variable_name);
          if (symbol == NULL) {
              fprintf(stderr, "Undefined variable: %s\n", expr->value.variable_name);
              exit(1);
          }
          
          // For simplicity, we use stack-based variables
          // RBP (REG_RBP) is the base pointer
          emit_load(output, dest_reg, REG_RBP, symbol->offset);
          return dest_reg;
      }
      
      case EXPR_ASSIGN: {
          Symbol *symbol = symbol_table_lookup(generator->current_scope, expr->value.assign.variable_name);
          if (symbol == NULL) {
              fprintf(stderr, "Undefined variable: %s\n", expr->value.assign.variable_name);
              exit(1);
          }
          
          int value_reg = generate_expression(generator, expr->value.assign.value, dest_reg);
          
          // Store the value at the variable's stack location
          // RBP (REG_RBP) is the base pointer
          emit_store(output, value_reg, REG_RBP, symbol->offset);
          return value_reg;
      }
      
      case EXPR_CALL: {
          // Push arguments (simplified; real ABI would be more complex)
          for (int i = expr->value.call.argument_count - 1; i >= 0; i--) {
              int arg_reg = generate_expression(generator, expr->value.call.arguments[i], dest_reg + i);
              emit_push(output, arg_reg);
          }
          
          emit_call(output, expr->value.call.function_name);
          
          // Clean up the stack (caller cleanup, simplified)
          if (expr->value.call.argument_count > 0) {
              // Adjust stack pointer to remove arguments
              // This is simplified - real ABI would be more complex
              int stack_adjustment = expr->value.call.argument_count * 4;  // Assuming 4 bytes per argument
              emit_instruction(output, OP_ADJSP, 0x00, 0x01);
              emit_immediate_operand_i32(output, COIL_TYPE_INT, stack_adjustment);
          }
          
          // Return value is in REG_RQ0, move to dest_reg if needed
          if (dest_reg != REG_RQ0) {
              emit_mov(output, dest_reg, REG_RQ0);
          }
          
          return dest_reg;
      }
      
      case EXPR_UNARY: {
          int operand_reg = generate_expression(generator, expr->value.unary.operand, dest_reg);
          
          switch (expr->value.unary.operator) {
              case TOKEN_MINUS:
                  // Negate the value - we use 0 - value
                  emit_movi(output, dest_reg + 1, 0);
                  emit_sub(output, dest_reg, dest_reg + 1, operand_reg);
                  break;
                  
              case TOKEN_NOT:
                  // Logical NOT - we compare with 0 to get a boolean
                  emit_movi(output, dest_reg + 1, 0);
                  emit_cmp(output, operand_reg, dest_reg + 1);
                  
                  // Create labels for the true and end branches
                  int true_label = generator_next_label(generator);
                  int end_label = generator_next_label(generator);
                  
                  emit_jcc(output, BR_EQ, true_label);  // Jump if equal to 0
                  
                  // False branch - result is 0
                  emit_movi(output, dest_reg, 0);
                  emit_jmp(output, end_label);
                  
                  // True branch - result is 1
                  emit_label(output, true_label);
                  emit_movi(output, dest_reg, 1);
                  
                  // End label
                  emit_label(output, end_label);
                  break;
                  
              case TOKEN_BIT_NOT:
                  // Bitwise NOT - use XOR with -1 (all bits set)
                  emit_movi(output, dest_reg + 1, -1);
                  emit_instruction(output, OP_XOR, 0x00, 0x03);
                  emit_register_operand(output, COIL_TYPE_INT, 0x04, dest_reg);
                  emit_register_operand(output, COIL_TYPE_INT, 0x04, operand_reg);
                  emit_register_operand(output, COIL_TYPE_INT, 0x04, dest_reg + 1);
                  break;
                  
              default:
                  fprintf(stderr, "Unsupported unary operator\n");
                  exit(1);
          }
          
          return dest_reg;
      }
      
      case EXPR_SUBSCRIPT: {
          // Generate code for the array
          int array_reg = generate_expression(generator, expr->value.subscript.array, dest_reg);
          
          // Generate code for the index
          int index_reg = generate_expression(generator, expr->value.subscript.index, dest_reg + 1);
          
          // Multiply index by element size (assuming 4 bytes for now)
          emit_movi(output, dest_reg + 2, 4);
          emit_mul(output, dest_reg + 1, index_reg, dest_reg + 2);
          
          // Add the base address and the offset
          emit_add(output, dest_reg + 1, array_reg, dest_reg + 1);
          
          // Load from the calculated address
          // This is very simplified - real array access would be more complex
          emit_load(output, dest_reg, dest_reg + 1, 0);
          
          return dest_reg;
      }
      
      default:
          fprintf(stderr, "Unsupported expression type: %d\n", expr->type);
          exit(1);
  }
}