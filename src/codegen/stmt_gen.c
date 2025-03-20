/**
 * @file stmt_gen.c
 * @brief Statement code generation for the COIL C Compiler
 * @details Implements statement-specific code generation functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "codegen_internal.h"  /* Include internal function declarations */
#include "cof.h"
#include "coil_constants.h"
#include "symbol.h"

/* Forward declarations of functions used in this file */
static int generate_declaration(CodeGenerator *generator, Statement *stmt);
static int generate_if_statement(CodeGenerator *generator, Statement *stmt);
static int generate_while_statement(CodeGenerator *generator, Statement *stmt);
static int generate_for_statement(CodeGenerator *generator, Statement *stmt);
static int generate_return_statement(CodeGenerator *generator, Statement *stmt);
static int generate_block(CodeGenerator *generator, Statement *stmt);

/**
 * @brief Generate COIL code for a declaration statement
 *
 * @param generator Pointer to the generator
 * @param stmt Declaration statement to generate code for
 * @return int 0 on success, non-zero on failure
 */
static int generate_declaration(CodeGenerator *generator, Statement *stmt) {
  FILE *output;
  int offset;
  int value_reg;
  
  /* Validate input parameters */
  if (generator == NULL || stmt == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to generate_declaration\n");
    return -1;
  }
  
  output = generator->output;
  if (output == NULL) {
    fprintf(stderr, "Error: NULL output file in generate_declaration\n");
    return -1;
  }
  
  /* Allocate space for the variable on the stack */
  offset = allocate_local_variable(generator, stmt->value.declaration.type);
  if (offset == 0) {
    /* Error in allocation */
    return -1;
  }
  
  /* Add the variable to the symbol table */
  symbol_table_add(generator->current_scope, stmt->value.declaration.name, 
                 stmt->value.declaration.type, offset);
  
  /* Initialize the variable if there's an initializer */
  if (stmt->value.declaration.initializer != NULL) {
    /* Generate code for the initializer */
    value_reg = generate_expression(generator, stmt->value.declaration.initializer, REG_RQ0);
    if (value_reg < 0) {
      return -1;
    }
    
    /* Store the value in the variable */
    emit_store(output, value_reg, REG_RBP, offset);
  }
  
  return 0;
}

/**
 * @brief Generate COIL code for an if statement
 *
 * @param generator Pointer to the generator
 * @param stmt If statement to generate code for
 * @return int 0 on success, non-zero on failure
 */
static int generate_if_statement(CodeGenerator *generator, Statement *stmt) {
  FILE *output;
  int else_label;
  int end_label;
  int result;
  
  /* Validate input parameters */
  if (generator == NULL || stmt == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to generate_if_statement\n");
    return -1;
  }
  
  output = generator->output;
  if (output == NULL) {
    fprintf(stderr, "Error: NULL output file in generate_if_statement\n");
    return -1;
  }
  
  /* Generate labels for the else and end branches */
  else_label = generator_next_label(generator);
  end_label = generator_next_label(generator);
  
  /* Generate code for the condition */
  result = generate_expression(generator, stmt->value.if_statement.condition, REG_RQ0);
  if (result < 0) {
    return -1;
  }
  
  /* Compare the condition with 0 */
  emit_movi(output, REG_RQ1, 0);
  emit_cmp(output, REG_RQ0, REG_RQ1);
  
  /* Jump to else_label if condition is false (equal to 0) */
  emit_jcc(output, BR_EQ, else_label);
  
  /* Generate code for the "then" branch */
  result = generate_statement(generator, stmt->value.if_statement.then_branch);
  if (result != 0) {
    return result;
  }
  
  /* Jump to end_label to skip the else branch */
  emit_jmp(output, end_label);
  
  /* Generate code for the "else" branch */
  emit_label(output, else_label);
  if (stmt->value.if_statement.else_branch != NULL) {
    result = generate_statement(generator, stmt->value.if_statement.else_branch);
    if (result != 0) {
      return result;
    }
  }
  
  /* End label */
  emit_label(output, end_label);
  
  return 0;
}

/**
 * @brief Generate COIL code for a while statement
 *
 * @param generator Pointer to the generator
 * @param stmt While statement to generate code for
 * @return int 0 on success, non-zero on failure
 */
static int generate_while_statement(CodeGenerator *generator, Statement *stmt) {
  FILE *output;
  int start_label;
  int end_label;
  int result;
  
  /* Validate input parameters */
  if (generator == NULL || stmt == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to generate_while_statement\n");
    return -1;
  }
  
  output = generator->output;
  if (output == NULL) {
    fprintf(stderr, "Error: NULL output file in generate_while_statement\n");
    return -1;
  }
  
  /* Generate labels for the loop start and end */
  start_label = generator_next_label(generator);
  end_label = generator_next_label(generator);
  
  /* Start label */
  emit_label(output, start_label);
  
  /* Generate code for the condition */
  result = generate_expression(generator, stmt->value.while_statement.condition, REG_RQ0);
  if (result < 0) {
    return -1;
  }
  
  /* Compare the condition with 0 */
  emit_movi(output, REG_RQ1, 0);
  emit_cmp(output, REG_RQ0, REG_RQ1);
  
  /* Jump to end_label if condition is false (equal to 0) */
  emit_jcc(output, BR_EQ, end_label);
  
  /* Generate code for the loop body */
  result = generate_statement(generator, stmt->value.while_statement.body);
  if (result != 0) {
    return result;
  }
  
  /* Jump back to the start label */
  emit_jmp(output, start_label);
  
  /* End label */
  emit_label(output, end_label);
  
  return 0;
}

/**
 * @brief Generate COIL code for a for statement
 *
 * @param generator Pointer to the generator
 * @param stmt For statement to generate code for
 * @return int 0 on success, non-zero on failure
 */
 static int generate_for_statement(CodeGenerator *generator, Statement *stmt) {
  FILE *output;
  int cond_label;
  int incr_label;
  int end_label;
  int result;
  
  /* Validate input parameters */
  if (generator == NULL || stmt == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to generate_for_statement\n");
    return -1;
  }
  
  output = generator->output;
  if (output == NULL) {
    fprintf(stderr, "Error: NULL output file in generate_for_statement\n");
    return -1;
  }
  
  /* Generate labels for the loop condition, increment, and end */
  cond_label = generator_next_label(generator);
  incr_label = generator_next_label(generator);
  end_label = generator_next_label(generator);
  
  /* Generate code for the initializer if present */
  if (stmt->value.for_statement.initializer != NULL) {
    result = generate_expression(generator, stmt->value.for_statement.initializer, REG_RQ0);
    if (result < 0) {
      return -1;
    }
  }
  
  /* Condition label */
  emit_label(output, cond_label);
  
  /* Generate code for the condition if present */
  if (stmt->value.for_statement.condition != NULL) {
    result = generate_expression(generator, stmt->value.for_statement.condition, REG_RQ0);
    if (result < 0) {
      return -1;
    }
    
    /* Compare the condition with 0 */
    emit_movi(output, REG_RQ1, 0);
    emit_cmp(output, REG_RQ0, REG_RQ1);
    
    /* Jump to end_label if condition is false (equal to 0) */
    emit_jcc(output, BR_EQ, end_label);
  }
  
  /* Generate code for the loop body */
  result = generate_statement(generator, stmt->value.for_statement.body);
  if (result != 0) {
    return result;
  }
  
  /* Increment label */
  emit_label(output, incr_label);
  
  /* Generate code for the increment if present */
  if (stmt->value.for_statement.increment != NULL) {
    result = generate_expression(generator, stmt->value.for_statement.increment, REG_RQ0);
    if (result < 0) {
      return -1;
    }
  }
  
  /* Jump back to the condition label */
  emit_jmp(output, cond_label);
  
  /* End label */
  emit_label(output, end_label);
  
  return 0;
}

/**
 * @brief Generate COIL code for a return statement
 *
 * @param generator Pointer to the generator
 * @param stmt Return statement to generate code for
 * @return int 0 on success, non-zero on failure
 */
static int generate_return_statement(CodeGenerator *generator, Statement *stmt) {
  int result;
  
  /* Validate input parameters */
  if (generator == NULL || stmt == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to generate_return_statement\n");
    return -1;
  }
  
  /* Generate code for the return value if present */
  if (stmt->value.expression != NULL) {
    result = generate_expression(generator, stmt->value.expression, REG_RQ0);
    if (result < 0) {
      return -1;
    }
  }
  
  /* Clean up and return */
  result = emit_function_epilogue(generator);
  if (result != 0) {
    return result;
  }
  
  return 0;
}

/**
 * @brief Generate COIL code for a block statement
 *
 * @param generator Pointer to the generator
 * @param stmt Block statement to generate code for
 * @return int 0 on success, non-zero on failure
 */
static int generate_block(CodeGenerator *generator, Statement *stmt) {
  SymbolTable *old_scope;
  int i;
  int result;
  
  /* Validate input parameters */
  if (generator == NULL || stmt == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to generate_block\n");
    return -1;
  }
  
  /* Create a new scope for the block */
  old_scope = generator->current_scope;
  generator->current_scope = symbol_table_create(old_scope);
  if (generator->current_scope == NULL) {
    fprintf(stderr, "Error: Failed to create symbol table for block\n");
    return -1;
  }
  
  /* Generate code for each statement in the block */
  for (i = 0; i < stmt->value.block.statement_count; i++) {
    result = generate_statement(generator, stmt->value.block.statements[i]);
    if (result != 0) {
      symbol_table_free(generator->current_scope);
      generator->current_scope = old_scope;
      return result;
    }
  }
  
  /* Restore the previous scope */
  symbol_table_free(generator->current_scope);
  generator->current_scope = old_scope;
  
  return 0;
}

/**
 * @brief Generate COIL code for a statement
 * 
 * This function is declared in codegen.h and implemented here.
 * It routes to the appropriate statement handler based on statement type.
 *
 * @param generator Pointer to the generator
 * @param stmt Statement to generate code for
 * @return int 0 on success, non-zero on failure
 */
int generate_statement(CodeGenerator *generator, Statement *stmt) {
  /* Validate input parameters */
  if (generator == NULL || stmt == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to generate_statement\n");
    return -1;
  }
  
  switch (stmt->type) {
    case STMT_EXPRESSION:
      return (generate_expression(generator, stmt->value.expression, REG_RQ0) < 0) ? -1 : 0;
      
    case STMT_DECLARATION:
      return generate_declaration(generator, stmt);
      
    case STMT_IF:
      return generate_if_statement(generator, stmt);
      
    case STMT_WHILE:
      return generate_while_statement(generator, stmt);
      
    case STMT_FOR:
      return generate_for_statement(generator, stmt);
      
    case STMT_RETURN:
      return generate_return_statement(generator, stmt);
      
    case STMT_BLOCK:
      return generate_block(generator, stmt);
      
    default:
      fprintf(stderr, "Error: Unsupported statement type: %d\n", stmt->type);
      return -1;
  }
}