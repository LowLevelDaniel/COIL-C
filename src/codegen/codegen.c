/**
 * @file codegen.c
 * @brief Enhanced code generation for the COIL C Compiler
 * @details Ensures compliance with COIL 1.0.0 specification
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "codegen_internal.h"  /* Include internal function declarations */
#include "cof.h"
#include "coil_constants.h"

/**
 * @brief Initialize a code generator
 *
 * @param generator Pointer to the generator to initialize
 * @param output Output file for code
 * @return int 0 on success, non-zero on failure
 */
int generator_init(CodeGenerator *generator, FILE *output) {
  /* Validate input parameters */
  if (generator == NULL || output == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to generator_init\n");
    return -1;
  }
  
  generator->output = output;
  generator->current_scope = symbol_table_create(NULL);
  if (generator->current_scope == NULL) {
    fprintf(stderr, "Error: Failed to create symbol table\n");
    return -1;
  }
  
  generator->next_label = 0;
  generator->stack_offset = 0;
  generator->optimization_level = 0;
  
  return 0;
}

/**
 * @brief Set the optimization level for the code generator
 *
 * @param generator Pointer to the generator
 * @param level Optimization level (0-3)
 * @return int 0 on success, non-zero on failure
 */
int generator_set_optimization(CodeGenerator *generator, int level) {
  /* Validate input parameters */
  if (generator == NULL) {
    fprintf(stderr, "Error: NULL generator passed to generator_set_optimization\n");
    return -1;
  }
  
  if (level < 0 || level > 3) {
    fprintf(stderr, "Error: Invalid optimization level: %d (must be 0-3)\n", level);
    return -1;
  }
  
  generator->optimization_level = level;
  return 0;
}

/**
 * @brief Get the next label number for local jumps/branches
 *
 * @param generator Pointer to the generator
 * @return Next unique label number
 */
int generator_next_label(CodeGenerator *generator) {
  /* Validate input parameter */
  if (generator == NULL) {
    fprintf(stderr, "Error: NULL generator passed to generator_next_label\n");
    return -1;
  }
  
  return generator->next_label++;
}

/**
 * @brief Allocate space on the stack for a variable
 * Returns the stack offset of the variable
 *
 * @param generator Pointer to the generator
 * @param type Type of the variable
 * @return Stack offset for the variable, or 0 on error
 */
int allocate_local_variable(CodeGenerator *generator, Type *type) {
  int align;
  int aligned_offset;
  
  /* Validate input parameters */
  if (generator == NULL || type == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to allocate_local_variable\n");
    return 0;
  }
  
  /* Calculate alignment - ensures variables are properly aligned */
  align = type->size >= 4 ? 4 : (type->size >= 2 ? 2 : 1);
  aligned_offset = (generator->stack_offset + align - 1) & ~(align - 1);
  
  /* Update stack offset */
  generator->stack_offset = aligned_offset + type->size;
  
  /* Return negative offset from base pointer */
  return -aligned_offset;
}

/**
 * @brief Emit code for function prologue
 * Sets up the stack frame and saves necessary registers
 *
 * @param generator Pointer to the generator
 * @param function Function to generate prologue for
 * @return int 0 on success, non-zero on failure
 */
int emit_function_prologue(CodeGenerator *generator, Function *function) {
  FILE *output;
  int i;
  int param_size;
  int aligned_size;
  int offset;
  
  /* Validate input parameters */
  if (generator == NULL || function == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to emit_function_prologue\n");
    return -1;
  }
  
  output = generator->output;
  if (output == NULL) {
    fprintf(stderr, "Error: NULL output file in emit_function_prologue\n");
    return -1;
  }
  
  /* Define the function symbol */
  emit_symbol_directive(output, SYM_BIND_GLOBAL, function->name, 0);
  
  /* Setup stack frame with ENTER instruction */
  /* This saves old base pointer and sets up new stack frame */
  emit_enter(output, 0); /* Frame size will be determined later */
  
  /* Allocate space for parameters */
  for (i = 0; i < function->parameter_count; i++) {
    /* Parameters are at positive offsets from the base pointer (already on stack) */
    param_size = function->parameter_types[i]->size;
    aligned_size = (param_size + 3) & ~3; /* Align to 4 bytes */
    offset = 8 + (i * aligned_size); /* 8 bytes for return address and saved base pointer */
    
    /* Add parameter to symbol table */
    symbol_table_add(generator->current_scope, function->parameter_names[i], 
                   function->parameter_types[i], offset);
  }
  
  return 0;
}

/**
 * @brief Emit code for function epilogue
 * Cleans up the stack frame and returns
 *
 * @param generator Pointer to the generator
 * @return int 0 on success, non-zero on failure
 */
int emit_function_epilogue(CodeGenerator *generator) {
  FILE *output;
  
  /* Validate input parameter */
  if (generator == NULL) {
    fprintf(stderr, "Error: NULL generator passed to emit_function_epilogue\n");
    return -1;
  }
  
  output = generator->output;
  if (output == NULL) {
    fprintf(stderr, "Error: NULL output file in emit_function_epilogue\n");
    return -1;
  }
  
  /* Restore stack pointer and base pointer */
  emit_leave(output);
  
  /* Return from function */
  emit_ret(output);
  
  return 0;
}

/**
 * @brief Generate COIL code for a binary operation
 *
 * @param generator Pointer to the generator
 * @param expr Binary expression to generate code for
 * @param dest_reg Destination register for the result
 * @return Register containing the result, or -1 on error
 */
int generate_binary_operation(CodeGenerator *generator, Expression *expr, int dest_reg) {
  FILE *output;
  int left_reg;
  int right_reg;
  int true_label;
  int end_label;
  uint8_t condition;
  
  /* Validate input parameters */
  if (generator == NULL || expr == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to generate_binary_operation\n");
    return -1;
  }
  
  output = generator->output;
  if (output == NULL) {
    fprintf(stderr, "Error: NULL output file in generate_binary_operation\n");
    return -1;
  }
  
  /* Generate code for left and right operands */
  left_reg = generate_expression(generator, expr->value.binary.left, dest_reg);
  if (left_reg < 0) {
    return -1;
  }
  
  right_reg = generate_expression(generator, expr->value.binary.right, dest_reg + 1);
  if (right_reg < 0) {
    return -1;
  }
  
  /* Perform the operation based on the operator */
  switch (expr->value.binary.operator) {
    case TOKEN_PLUS:
      emit_add(output, dest_reg, left_reg, right_reg);
      break;
      
    case TOKEN_MINUS:
      emit_sub(output, dest_reg, left_reg, right_reg);
      break;
      
    case TOKEN_MULTIPLY:
      emit_mul(output, dest_reg, left_reg, right_reg);
      break;
      
    case TOKEN_DIVIDE:
      emit_div(output, dest_reg, left_reg, right_reg);
      break;
      
    case TOKEN_EQ:
    case TOKEN_NEQ:
    case TOKEN_LT:
    case TOKEN_LE:
    case TOKEN_GT:
    case TOKEN_GE:
      /* Compare the operands */
      emit_cmp(output, left_reg, right_reg);
      
      /* Create labels for the true and end branches */
      true_label = generator_next_label(generator);
      end_label = generator_next_label(generator);
      
      /* Determine the branch condition */
      switch (expr->value.binary.operator) {
        case TOKEN_EQ:  condition = BR_EQ; break;
        case TOKEN_NEQ: condition = BR_NE; break;
        case TOKEN_LT:  condition = BR_LT; break;
        case TOKEN_LE:  condition = BR_LE; break;
        case TOKEN_GT:  condition = BR_GT; break;
        case TOKEN_GE:  condition = BR_GE; break;
        default: condition = BR_ALWAYS; break;
      }
      
      /* Jump to true_label if condition is met */
      emit_jcc(output, condition, true_label);
      
      /* False case: load 0 */
      emit_movi(output, dest_reg, 0);
      emit_jmp(output, end_label);
      
      /* True case: load 1 */
      emit_label(output, true_label);
      emit_movi(output, dest_reg, 1);
      
      /* End label */
      emit_label(output, end_label);
      break;
      
    default:
      fprintf(stderr, "Error: Unsupported binary operator: %d\n", expr->value.binary.operator);
      return -1;
  }
  
  return dest_reg;
}

/**
 * @brief Generate COIL code for a function call
 *
 * @param generator Pointer to the generator
 * @param expr Function call expression to generate code for
 * @param dest_reg Destination register for the result
 * @return Register containing the result, or -1 on error
 */
int generate_function_call(CodeGenerator *generator, Expression *expr, int dest_reg) {
  FILE *output;
  int i;
  int arg_reg;
  int arg_size;
  Type *arg_type;
  int aligned_size;
  
  /* Validate input parameters */
  if (generator == NULL || expr == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to generate_function_call\n");
    return -1;
  }
  
  output = generator->output;
  if (output == NULL) {
    fprintf(stderr, "Error: NULL output file in generate_function_call\n");
    return -1;
  }
  
  /* Evaluate arguments in reverse order and push them onto the stack */
  for (i = expr->value.call.argument_count - 1; i >= 0; i--) {
    arg_reg = generate_expression(generator, expr->value.call.arguments[i], dest_reg);
    if (arg_reg < 0) {
      return -1;
    }
    
    emit_push(output, arg_reg);
  }
  
  /* Call the function */
  emit_call(output, expr->value.call.function_name);
  
  /* Clean up the stack (caller cleanup) */
  /* Calculate total size of arguments */
  arg_size = 0;
  for (i = 0; i < expr->value.call.argument_count; i++) {
    arg_type = expr->value.call.arguments[i]->data_type;
    aligned_size = (arg_type->size + 3) & ~3; /* Align to 4 bytes */
    arg_size += aligned_size;
  }
  
  /* Adjust stack pointer if needed */
  if (arg_size > 0) {
    emit_instruction(output, OP_ADJSP, 0x00, 0x01);
    emit_immediate_operand_i32(output, COIL_TYPE_INT, arg_size);
  }
  
  /* Move result to destination register if needed */
  if (dest_reg != REG_RQ0) {
    emit_mov(output, dest_reg, REG_RQ0);
  }
  
  return dest_reg;
}

/**
 * @brief Generate COIL code for a variable reference
 *
 * @param generator Pointer to the generator
 * @param expr Variable reference expression to generate code for
 * @param dest_reg Destination register for the result
 * @return Register containing the result, or -1 on error
 */
int generate_variable_reference(CodeGenerator *generator, Expression *expr, int dest_reg) {
  FILE *output;
  Symbol *symbol;
  
  /* Validate input parameters */
  if (generator == NULL || expr == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to generate_variable_reference\n");
    return -1;
  }
  
  output = generator->output;
  if (output == NULL) {
    fprintf(stderr, "Error: NULL output file in generate_variable_reference\n");
    return -1;
  }
  
  /* Look up the variable in the symbol table */
  symbol = symbol_table_lookup(generator->current_scope, expr->value.variable_name);
  if (symbol == NULL) {
    fprintf(stderr, "Error: Undefined variable '%s'\n", expr->value.variable_name);
    return -1;
  }
  
  /* Load the variable's value */
  emit_load(output, dest_reg, REG_RBP, symbol->offset);
  
  return dest_reg;
}

/**
 * @brief Generate COIL code for a variable assignment
 *
 * @param generator Pointer to the generator
 * @param expr Assignment expression to generate code for
 * @param dest_reg Destination register for the result
 * @return Register containing the result, or -1 on error
 */
int generate_variable_assignment(CodeGenerator *generator, Expression *expr, int dest_reg) {
  FILE *output;
  Symbol *symbol;
  int value_reg;
  
  /* Validate input parameters */
  if (generator == NULL || expr == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to generate_variable_assignment\n");
    return -1;
  }
  
  output = generator->output;
  if (output == NULL) {
    fprintf(stderr, "Error: NULL output file in generate_variable_assignment\n");
    return -1;
  }
  
  /* Look up the variable in the symbol table */
  symbol = symbol_table_lookup(generator->current_scope, expr->value.assign.variable_name);
  if (symbol == NULL) {
    fprintf(stderr, "Error: Undefined variable '%s'\n", expr->value.assign.variable_name);
    return -1;
  }
  
  /* Generate code for the right-hand side expression */
  value_reg = generate_expression(generator, expr->value.assign.value, dest_reg);
  if (value_reg < 0) {
    return -1;
  }
  
  /* Store the value in the variable */
  emit_store(output, value_reg, REG_RBP, symbol->offset);
  
  return value_reg;
}

/**
 * @brief Generate COIL code for an expression
 *
 * @param generator Pointer to the generator
 * @param expr Expression to generate code for
 * @param dest_reg Destination register for the result
 * @return Register containing the result, or -1 on error
 */
int generate_expression(CodeGenerator *generator, Expression *expr, int dest_reg) {
  FILE *output;
  
  /* Validate input parameters */
  if (generator == NULL || expr == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to generate_expression\n");
    return -1;
  }
  
  output = generator->output;
  if (output == NULL) {
    fprintf(stderr, "Error: NULL output file in generate_expression\n");
    return -1;
  }
  
  switch (expr->type) {
    case EXPR_LITERAL_INT:
      emit_movi(output, dest_reg, expr->value.int_value);
      return dest_reg;
      
    case EXPR_LITERAL_FLOAT: {
      /* For simplicity, we're treating floats as integers in this version */
      /* A more complete implementation would use COIL's floating-point support */
      int int_val = (int)expr->value.float_value;
      emit_movi(output, dest_reg, int_val);
      return dest_reg;
    }
      
    case EXPR_LITERAL_CHAR:
      return generate_literal_char_expr(generator, expr, dest_reg);
      
    case EXPR_BINARY:
      return generate_binary_operation(generator, expr, dest_reg);
      
    case EXPR_VARIABLE:
      return generate_variable_reference(generator, expr, dest_reg);
      
    case EXPR_ASSIGN:
      return generate_variable_assignment(generator, expr, dest_reg);
      
    case EXPR_CALL:
      return generate_function_call(generator, expr, dest_reg);
      
    case EXPR_UNARY:
      return generate_unary_expr(generator, expr, dest_reg);
      
    case EXPR_SUBSCRIPT:
      return generate_subscript_expr(generator, expr, dest_reg);
      
    case EXPR_CAST:
      return generate_cast_expr(generator, expr, dest_reg);
      
    default:
      fprintf(stderr, "Error: Unsupported expression type: %d\n", expr->type);
      return -1;
  }
}

/**
 * @brief Find the function offset in the code section
 *
 * @param function Function to find
 * @param program Program containing the function
 * @return Function offset in the code section
 */
uint32_t find_function_offset(Function *function, Program *program) {
  /* TODO: track the actual offsets of functions */
  (void)function;
  (void)program;
  return COF_HEADER_SIZE + COF_SECTION_HEADER_SIZE + 16; /* Skip directives */
}

/**
 * @brief Generate COIL code for a function
 *
 * @param generator Pointer to the generator
 * @param function Function to generate code for
 * @return int 0 on success, non-zero on failure
 */
int generate_function(CodeGenerator *generator, Function *function) {
  SymbolTable *old_scope;
  int result;
  
  /* Validate input parameters */
  if (generator == NULL || function == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to generate_function\n");
    return -1;
  }
  
  /* Reset stack offset for new function */
  generator->stack_offset = 0;
  
  /* Create a new scope for the function */
  old_scope = generator->current_scope;
  generator->current_scope = symbol_table_create(old_scope);
  if (generator->current_scope == NULL) {
    fprintf(stderr, "Error: Failed to create symbol table for function\n");
    return -1;
  }
  
  /* Generate function prologue */
  result = emit_function_prologue(generator, function);
  if (result != 0) {
    symbol_table_free(generator->current_scope);
    generator->current_scope = old_scope;
    return result;
  }
  
  /* Generate code for function body */
  result = generate_statement(generator, function->body);
  if (result != 0) {
    symbol_table_free(generator->current_scope);
    generator->current_scope = old_scope;
    return result;
  }
  
  /* Add implicit return if the function doesn't already have one */
  if (function->body->type == STMT_BLOCK && 
      (function->body->value.block.statement_count == 0 || 
       function->body->value.block.statements[function->body->value.block.statement_count - 1]->type != STMT_RETURN)) {
    result = emit_function_epilogue(generator);
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
 * @brief Generate COIL code for the entire program
 *
 * @param program Program to generate code for
 * @param output_file Output file path
 * @return int 0 on success, non-zero on failure
 */
 int generate_program(Program *program, const char *output_file) {
  FILE *output;
  CodeGenerator generator;
  uint32_t entrypoint = 0;
  int i;
  int result;
  
  /* Validate input parameters */
  if (program == NULL || output_file == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to generate_program\n");
    return -1;
  }
  
  output = fopen(output_file, "wb");
  if (output == NULL) {
    fprintf(stderr, "Error: Failed to open output file '%s'\n", output_file);
    return -1;
  }
  
  /* Write initial COF header and section headers */
  generate_cof_header(output);
  
  /* Generate code for each function */
  result = generator_init(&generator, output);
  if (result != 0) {
    fprintf(stderr, "Error: Failed to initialize code generator\n");
    fclose(output);
    return result;
  }
  
  /* First pass: generate function symbols */
  for (i = 0; i < program->function_count; i++) {
    Function *function = program->functions[i];
    
    if (function == NULL) {
      fprintf(stderr, "Error: NULL function at index %d\n", i);
      continue;
    }
    
    /* Define the function symbol */
    emit_symbol_directive(output, SYM_BIND_GLOBAL, function->name, 0);
    
    /* Check if this is the main function */
    if (strcmp(function->name, "main") == 0) {
      /* In a real compiler, we'd track the actual offset */
      entrypoint = find_function_offset(function, program);
    }
  }
  
  /* Second pass: generate function code */
  for (i = 0; i < program->function_count; i++) {
    result = generate_function(&generator, program->functions[i]);
    if (result != 0) {
      fprintf(stderr, "Error: Failed to generate code for function '%s'\n", 
             program->functions[i]->name);
      fclose(output);
      return result;
    }
  }
  
  /* Update the COF header with the final sizes and offsets */
  update_cof_header(output, entrypoint);
  
  fclose(output);
  
  printf("Generated COIL binary: %s\n", output_file);
  
  return 0;
}

/**
 * @brief Free resources used by the code generator
 *
 * @param generator Pointer to the generator to free
 * @return int 0 on success, non-zero on failure
 */
int generator_free(CodeGenerator *generator) {
  /* Validate input parameter */
  if (generator == NULL) {
    fprintf(stderr, "Error: NULL generator passed to generator_free\n");
    return -1;
  }
  
  if (generator->current_scope != NULL) {
    symbol_table_free(generator->current_scope);
    generator->current_scope = NULL;
  }
  
  /* No need to free output file as it's handled by the caller */
  generator->output = NULL;
  
  return 0;
}