/**
* Statement code generation for the COIL C Compiler
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "cof.h"
#include "coil_constants.h"
#include "symbol.h"

// Forward declaration
extern int generate_expression(CodeGenerator *generator, Expression *expr, int dest_reg);

void generate_statement(CodeGenerator *generator, Statement *stmt) {
  FILE *output = generator->output;
  
  switch (stmt->type) {
      case STMT_EXPRESSION:
          generate_expression(generator, stmt->value.expression, REG_RQ0);
          break;
      
      case STMT_RETURN:
          if (stmt->value.expression != NULL) {
              // Generate the return value expression, result in RQ0 (return value register)
              generate_expression(generator, stmt->value.expression, REG_RQ0);
          }
          
          // Cleanup and return
          emit_leave(output);
          emit_ret(output);
          break;
      
      case STMT_IF: {
          int else_label = generator_next_label(generator);
          int end_label = generator_next_label(generator);
          
          // Generate condition
          generate_expression(generator, stmt->value.if_statement.condition, REG_RQ0);
          
          // Compare with 0 to check truthiness
          emit_movi(output, REG_RQ1, 0);
          emit_cmp(output, REG_RQ0, REG_RQ1);
          
          // Jump to else branch if condition is false (equal to 0)
          emit_jcc(output, BR_EQ, else_label);
          
          // Generate then branch
          generate_statement(generator, stmt->value.if_statement.then_branch);
          
          // Jump to end, bypassing else branch
          emit_jmp(output, end_label);
          
          // Generate else branch
          emit_label(output, else_label);
          if (stmt->value.if_statement.else_branch != NULL) {
              generate_statement(generator, stmt->value.if_statement.else_branch);
          }
          
          // End label
          emit_label(output, end_label);
          break;
      }
      
      case STMT_WHILE: {
          int start_label = generator_next_label(generator);
          int end_label = generator_next_label(generator);
          
          // Start label
          emit_label(output, start_label);
          
          // Generate condition
          generate_expression(generator, stmt->value.while_statement.condition, REG_RQ0);
          
          // Compare with 0 to check truthiness
          emit_movi(output, REG_RQ1, 0);
          emit_cmp(output, REG_RQ0, REG_RQ1);
          
          // Jump to end if condition is false (equal to 0)
          emit_jcc(output, BR_EQ, end_label);
          
          // Generate loop body
          generate_statement(generator, stmt->value.while_statement.body);
          
          // Jump back to start to check condition again
          emit_jmp(output, start_label);
          
          // End label
          emit_label(output, end_label);
          break;
      }
      
      case STMT_FOR: {
          int start_label = generator_next_label(generator);
          int end_label = generator_next_label(generator);
          int increment_label = generator_next_label(generator);
          
          // Generate initializer
          if (stmt->value.for_statement.initializer != NULL) {
              generate_expression(generator, stmt->value.for_statement.initializer, REG_RQ0);
          }
          
          // Start label
          emit_label(output, start_label);
          
          // Generate condition
          if (stmt->value.for_statement.condition != NULL) {
              generate_expression(generator, stmt->value.for_statement.condition, REG_RQ0);
              
              // Compare with 0 to check truthiness
              emit_movi(output, REG_RQ1, 0);
              emit_cmp(output, REG_RQ0, REG_RQ1);
              
              // Jump to end if condition is false (equal to 0)
              emit_jcc(output, BR_EQ, end_label);
          }
          
          // Generate loop body
          generate_statement(generator, stmt->value.for_statement.body);
          
          // Increment label
          emit_label(output, increment_label);
          
          // Generate increment
          if (stmt->value.for_statement.increment != NULL) {
              generate_expression(generator, stmt->value.for_statement.increment, REG_RQ0);
          }
          
          // Jump back to start to check condition again
          emit_jmp(output, start_label);
          
          // End label
          emit_label(output, end_label);
          break;
      }
      
      case STMT_BLOCK: {
          // Create a new scope for the block
          SymbolTable *old_scope = generator->current_scope;
          generator->current_scope = symbol_table_create(old_scope);
          
          // Generate code for each statement in the block
          for (int i = 0; i < stmt->value.block.statement_count; i++) {
              generate_statement(generator, stmt->value.block.statements[i]);
          }
          
          // Restore the old scope
          generator->current_scope = old_scope;
          break;
      }
      
      case STMT_DECLARATION: {
          // Allocate space on the stack for the variable
          // We decrement the stack offset to allocate space
          generator->stack_offset -= stmt->value.declaration.type->size;
          int offset = generator->stack_offset;
          
          // Add the variable to the symbol table
          symbol_table_add(generator->current_scope, stmt->value.declaration.name, 
                            stmt->value.declaration.type, offset);
          
          // Initialize the variable if needed
          if (stmt->value.declaration.initializer != NULL) {
              int reg = generate_expression(generator, stmt->value.declaration.initializer, REG_RQ0);
              
              // Store the initialized value
              emit_store(output, reg, REG_RBP, offset);
          }
          
          break;
      }
      
      default:
          fprintf(stderr, "Unsupported statement type: %d\n", stmt->type);
          exit(1);
  }
}