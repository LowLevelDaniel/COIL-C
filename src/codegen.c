/**
 * @file codegen.c
 * @brief COIL code generation implementation
 */

#include "../include/codegen.h"
#include "../include/arena.h"
#include <string.h>
#include <stdlib.h>

/* Symbol table implementation */

#define SYMBOL_TABLE_INITIAL_SIZE 64

static unsigned int hash_string(const char* str) {
  unsigned int hash = 5381;
  int c;
  
  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }
  
  return hash;
}

SymbolTable* symbol_table_create(struct Arena* arena) {
  SymbolTable* table = arena_alloc(arena, sizeof(SymbolTable));
  table->bucket_count = SYMBOL_TABLE_INITIAL_SIZE;
  table->buckets = arena_calloc(arena, sizeof(SymbolEntry*) * table->bucket_count);
  table->size = 0;
  table->current_scope = 0;
  table->arena = arena;
  return table;
}

void symbol_table_enter_scope(SymbolTable* table) {
  table->current_scope++;
}

void symbol_table_exit_scope(SymbolTable* table) {
  // We don't actually remove symbols; we just check scopes during lookup
  if (table->current_scope > 0) {
    table->current_scope--;
  }
}

Symbol* symbol_table_add(SymbolTable* table, const char* name, Type* type, bool is_global, int var_id) {
  unsigned int hash = hash_string(name) % table->bucket_count;
  
  // Check if symbol already exists in current scope
  SymbolEntry* entry = table->buckets[hash];
  while (entry) {
    if (strcmp(entry->symbol.name, name) == 0 && entry->symbol.scope_level == table->current_scope) {
      return NULL; // Symbol already exists in current scope
    }
    entry = entry->next;
  }
  
  // Create new entry
  SymbolEntry* new_entry = arena_alloc(table->arena, sizeof(SymbolEntry));
  new_entry->symbol.name = arena_strdup(table->arena, name);
  new_entry->symbol.type = type;
  new_entry->symbol.scope_level = table->current_scope;
  new_entry->symbol.is_global = is_global;
  new_entry->symbol.var_id = var_id;
  
  // Add to bucket
  new_entry->next = table->buckets[hash];
  table->buckets[hash] = new_entry;
  table->size++;
  
  return &new_entry->symbol;
}

Symbol* symbol_table_lookup(SymbolTable* table, const char* name) {
  unsigned int hash = hash_string(name) % table->bucket_count;
  
  // Find the symbol in the most recent scope
  SymbolEntry* entry = table->buckets[hash];
  Symbol* best_match = NULL;
  
  while (entry) {
    if (strcmp(entry->symbol.name, name) == 0) {
      // If we haven't found a match yet, or this one is in a more recent scope
      if (!best_match || entry->symbol.scope_level > best_match->scope_level) {
        best_match = &entry->symbol;
      }
    }
    entry = entry->next;
  }
  
  return best_match;
}

/* COIL Code Generator implementation */

CodeGenerator* codegen_create(Program* program, FILE* output, struct Arena* arena) {
  CodeGenerator* gen = arena_alloc(arena, sizeof(CodeGenerator));
  gen->program = program;
  gen->symbols = symbol_table_create(arena);
  gen->arena = arena;
  gen->output = output;
  gen->var_counter = 0;
  gen->label_counter = 0;
  gen->current_scope = 0;
  gen->current_function_return_type = NULL;
  gen->string_literals = NULL;
  gen->string_count = 0;
  gen->has_error = false;
  gen->error_message = NULL;
  return gen;
}

uint8_t codegen_map_type(Type* type) {
  switch (type->kind) {
    case TYPE_VOID:
      return 0xF0; // COIL_TYPE_VOID
    
    case TYPE_CHAR:
      if (type->is_signed)
        return 0x00; // COIL_TYPE_INT with width 8
      else
        return 0x01; // COIL_TYPE_UINT with width 8
    
    case TYPE_SHORT:
      if (type->is_signed)
        return 0x00; // COIL_TYPE_INT with width 16
      else
        return 0x01; // COIL_TYPE_UINT with width 16
    
    case TYPE_INT:
      if (type->is_signed)
        return 0x00; // COIL_TYPE_INT with width 32
      else
        return 0x01; // COIL_TYPE_UINT with width 32
    
    case TYPE_LONG:
      if (type->is_signed)
        return 0x00; // COIL_TYPE_INT with width 64
      else
        return 0x01; // COIL_TYPE_UINT with width 64
    
    case TYPE_FLOAT:
      return 0x02; // COIL_TYPE_FLOAT with width 32
    
    case TYPE_DOUBLE:
      return 0x02; // COIL_TYPE_FLOAT with width 64
    
    case TYPE_POINTER:
      return 0xF4; // COIL_TYPE_PTR
    
    // For other types, we'll need more complex handling in a full compiler
    default:
      return 0x00; // Default to INT for now
  }
}

void codegen_emit_instruction(CodeGenerator* gen, uint8_t opcode, uint8_t qualifier, uint8_t operand_count) {
  fputc(opcode, gen->output);
  fputc(qualifier, gen->output);
  fputc(operand_count, gen->output);
}

void codegen_emit_operand(CodeGenerator* gen, uint8_t qualifier, uint8_t type, const void* data, size_t size) {
  fputc(qualifier, gen->output);
  fputc(type, gen->output);
  
  if (data && size > 0) {
    fwrite(data, 1, size, gen->output);
  }
}

int codegen_new_label(CodeGenerator* gen) {
  return gen->label_counter++;
}

void codegen_emit_label(CodeGenerator* gen, int label_id) {
  // Emit a symbol directive for the label
  uint8_t directive_opcode = 0xD3; // DIR_OPCODE_SYMBOL
  uint8_t qualifier = 0x01; // SYMBOL_LOCAL
  
  char label_name[32];
  sprintf(label_name, "L%d", label_id);
  uint16_t name_len = strlen(label_name) + 1;
  uint16_t length = name_len;
  
  fputc(directive_opcode, gen->output);
  fputc(qualifier, gen->output);
  fwrite(&length, sizeof(length), 1, gen->output);
  
  fputc(name_len - 1, gen->output); // Emit name length
  fwrite(label_name, 1, name_len - 1, gen->output); // Emit name without null terminator
}

int codegen_add_string_literal(CodeGenerator* gen, const char* str) {
  // Check if string already exists
  for (int i = 0; i < gen->string_count; i++) {
    if (strcmp(gen->string_literals[i], str) == 0) {
      return i;
    }
  }
  
  // Add new string
  if (gen->string_literals == NULL) {
    gen->string_literals = arena_alloc(gen->arena, sizeof(char*) * 16);
  } else if (gen->string_count % 16 == 0) {
    char** new_array = arena_alloc(gen->arena, sizeof(char*) * (gen->string_count + 16));
    memcpy(new_array, gen->string_literals, sizeof(char*) * gen->string_count);
    gen->string_literals = new_array;
  }
  
  gen->string_literals[gen->string_count] = arena_strdup(gen->arena, str);
  return gen->string_count++;
}

/* Expression code generation */

int codegen_binary_expression(CodeGenerator* gen, Expr* expr) {
  // Generate code for the left and right operands
  int left_var = codegen_expression(gen, expr->as.binary.left);
  int right_var = codegen_expression(gen, expr->as.binary.right);
  
  // Create a result variable
  int result_var = gen->var_counter++;
  
  // Map the operator to a COIL opcode
  CoilOpcode opcode;
  uint8_t qualifier = 0x00;
  
  switch (expr->as.binary.operator) {
    case TOKEN_PLUS:
      opcode = OP_ADD;
      break;
    case TOKEN_MINUS:
      opcode = OP_SUB;
      break;
    case TOKEN_STAR:
      opcode = OP_MUL;
      break;
    case TOKEN_SLASH:
      opcode = OP_DIV;
      break;
    case TOKEN_PERCENT:
      opcode = OP_MOD;
      break;
    case TOKEN_AMPERSAND:
      opcode = OP_AND;
      break;
    case TOKEN_PIPE:
      opcode = OP_OR;
      break;
    case TOKEN_CARET:
      opcode = OP_XOR;
      break;
    case TOKEN_LESS_LESS:
      opcode = OP_SHL;
      break;
    case TOKEN_GREATER_GREATER:
      opcode = OP_SHR;
      break;
    // For comparison operators, we need to use CMP and then a conditional
    case TOKEN_EQUAL_EQUAL:
    case TOKEN_EXCLAIM_EQUAL:
    case TOKEN_LESS:
    case TOKEN_LESS_EQUAL:
    case TOKEN_GREATER:
    case TOKEN_GREATER_EQUAL:
      // Compare the operands
      codegen_emit_instruction(gen, OP_CMP, 0x00, 2);
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &left_var, sizeof(left_var));
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &right_var, sizeof(right_var));
      
      // Create a temporary for the result
      int zero_var = gen->var_counter++;
      int one_var = gen->var_counter++;
      
      // Create the constants 0 and 1
      codegen_emit_instruction(gen, OP_MOVI, 0x00, 2);
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &zero_var, sizeof(zero_var));
      int zero = 0;
      codegen_emit_operand(gen, OPQUAL_IMM, 0x00, &zero, sizeof(zero));
      
      codegen_emit_instruction(gen, OP_MOVI, 0x00, 2);
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &one_var, sizeof(one_var));
      int one = 1;
      codegen_emit_operand(gen, OPQUAL_IMM, 0x00, &one, sizeof(one));
      
      // Generate unique labels
      int false_label = codegen_new_label(gen);
      int end_label = codegen_new_label(gen);
      
      // Branch to false_label if comparison fails
      uint8_t branch_qualifier;
      switch (expr->as.binary.operator) {
        case TOKEN_EQUAL_EQUAL:
          branch_qualifier = BR_NE; // Jump if NOT equal
          break;
        case TOKEN_EXCLAIM_EQUAL:
          branch_qualifier = BR_EQ; // Jump if equal
          break;
        case TOKEN_LESS:
          branch_qualifier = BR_GE; // Jump if NOT less than
          break;
        case TOKEN_LESS_EQUAL:
          branch_qualifier = BR_GT; // Jump if NOT less than or equal
          break;
        case TOKEN_GREATER:
          branch_qualifier = BR_LE; // Jump if NOT greater than
          break;
        case TOKEN_GREATER_EQUAL:
          branch_qualifier = BR_LT; // Jump if NOT greater than or equal
          break;
        default:
          branch_qualifier = BR_ALWAYS;
          break;
      }
      
      codegen_emit_instruction(gen, OP_BRC, branch_qualifier, 1);
      codegen_emit_operand(gen, OPQUAL_LBL, 0x00, &false_label, sizeof(false_label));
      
      // True case: result = 1
      codegen_emit_instruction(gen, OP_MOV, 0x00, 2);
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &result_var, sizeof(result_var));
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &one_var, sizeof(one_var));
      
      // Jump to end
      codegen_emit_instruction(gen, OP_BR, 0x00, 1);
      codegen_emit_operand(gen, OPQUAL_LBL, 0x00, &end_label, sizeof(end_label));
      
      // False label
      codegen_emit_label(gen, false_label);
      
      // False case: result = 0
      codegen_emit_instruction(gen, OP_MOV, 0x00, 2);
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &result_var, sizeof(result_var));
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &zero_var, sizeof(zero_var));
      
      // End label
      codegen_emit_label(gen, end_label);
      
      return result_var;
    default:
      // Unsupported operator, emit an error
      gen->has_error = true;
      gen->error_message = arena_strdup(gen->arena, "Unsupported binary operator");
      return -1;
  }
  
  // Emit the instruction
  codegen_emit_instruction(gen, opcode, qualifier, 3);
  
  // Emit operands
  codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &result_var, sizeof(result_var));
  codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &left_var, sizeof(left_var));
  codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &right_var, sizeof(right_var));
  
  return result_var;
}

int codegen_unary_expression(CodeGenerator* gen, Expr* expr) {
  // Generate code for the operand
  int operand_var = codegen_expression(gen, expr->as.unary.operand);
  
  // Create a result variable
  int result_var = gen->var_counter++;
  
  // Map the operator to a COIL opcode
  CoilOpcode opcode;
  uint8_t qualifier = 0x00;
  
  switch (expr->as.unary.operator) {
    case TOKEN_MINUS:
      opcode = OP_NEG;
      break;
    case TOKEN_TILDE:
      opcode = OP_NOT;
      break;
    case TOKEN_EXCLAIM:
      // For logical NOT, we need to compare with zero
      // Create a zero constant
      int zero_var = gen->var_counter++;
      codegen_emit_instruction(gen, OP_MOVI, 0x00, 2);
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &zero_var, sizeof(zero_var));
      int zero = 0;
      codegen_emit_operand(gen, OPQUAL_IMM, 0x00, &zero, sizeof(zero));
      
      // Compare with zero
      codegen_emit_instruction(gen, OP_CMP, 0x00, 2);
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &operand_var, sizeof(operand_var));
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &zero_var, sizeof(zero_var));
      
      // Create a one constant
      int one_var = gen->var_counter++;
      codegen_emit_instruction(gen, OP_MOVI, 0x00, 2);
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &one_var, sizeof(one_var));
      int one = 1;
      codegen_emit_operand(gen, OPQUAL_IMM, 0x00, &one, sizeof(one));
      
      // Generate unique labels
      int false_label = codegen_new_label(gen);
      int end_label = codegen_new_label(gen);
      
      // Branch if not equal to zero
      codegen_emit_instruction(gen, OP_BRC, BR_NE, 1);
      codegen_emit_operand(gen, OPQUAL_LBL, 0x00, &false_label, sizeof(false_label));
      
      // True case (operand is zero): result = 1
      codegen_emit_instruction(gen, OP_MOV, 0x00, 2);
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &result_var, sizeof(result_var));
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &one_var, sizeof(one_var));
      
      // Jump to end
      codegen_emit_instruction(gen, OP_BR, 0x00, 1);
      codegen_emit_operand(gen, OPQUAL_LBL, 0x00, &end_label, sizeof(end_label));
      
      // False label
      codegen_emit_label(gen, false_label);
      
      // False case: result = 0
      codegen_emit_instruction(gen, OP_MOV, 0x00, 2);
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &result_var, sizeof(result_var));
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &zero_var, sizeof(zero_var));
      
      // End label
      codegen_emit_label(gen, end_label);
      
      return result_var;
    
    case TOKEN_PLUS_PLUS:
      if (expr->as.unary.is_postfix) {
        // Postfix ++: first save the original value
        codegen_emit_instruction(gen, OP_MOV, 0x00, 2);
        codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &result_var, sizeof(result_var));
        codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &operand_var, sizeof(operand_var));
        
        // Then increment the operand
        codegen_emit_instruction(gen, OP_INC, 0x00, 1);
        codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &operand_var, sizeof(operand_var));
        
        // For variables, we need to update the actual variable
        if (expr->as.unary.operand->type == EXPR_IDENTIFIER) {
          Symbol* symbol = symbol_table_lookup(gen->symbols, expr->as.unary.operand->as.identifier.name);
          if (symbol) {
            if (symbol->is_global) {
              // Global variable update
              codegen_emit_instruction(gen, OP_VARSET, 0x00, 2);
              codegen_emit_operand(gen, OPQUAL_STR, 0x00, symbol->name, strlen(symbol->name) + 1);
              codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &operand_var, sizeof(operand_var));
            } else {
              // Local variable update
              codegen_emit_instruction(gen, OP_VARSET, 0x00, 2);
              int var_id = symbol->var_id;
              codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &var_id, sizeof(var_id));
              codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &operand_var, sizeof(operand_var));
            }
          }
        }
      } else {
        // Prefix ++: increment the operand
        codegen_emit_instruction(gen, OP_INC, 0x00, 1);
        codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &operand_var, sizeof(operand_var));
        
        // Update the actual variable
        if (expr->as.unary.operand->type == EXPR_IDENTIFIER) {
          Symbol* symbol = symbol_table_lookup(gen->symbols, expr->as.unary.operand->as.identifier.name);
          if (symbol) {
            if (symbol->is_global) {
              // Global variable update
              codegen_emit_instruction(gen, OP_VARSET, 0x00, 2);
              codegen_emit_operand(gen, OPQUAL_STR, 0x00, symbol->name, strlen(symbol->name) + 1);
              codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &operand_var, sizeof(operand_var));
            } else {
              // Local variable update
              codegen_emit_instruction(gen, OP_VARSET, 0x00, 2);
              int var_id = symbol->var_id;
              codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &var_id, sizeof(var_id));
              codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &operand_var, sizeof(operand_var));
            }
          }
        }
        
        // Then return the new value
        codegen_emit_instruction(gen, OP_MOV, 0x00, 2);
        codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &result_var, sizeof(result_var));
        codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &operand_var, sizeof(operand_var));
      }
      return result_var;
    
    case TOKEN_MINUS_MINUS:
      // Similar to ++ but with DEC instead of INC
      if (expr->as.unary.is_postfix) {
        // Save original value
        codegen_emit_instruction(gen, OP_MOV, 0x00, 2);
        codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &result_var, sizeof(result_var));
        codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &operand_var, sizeof(operand_var));
        
        // Decrement
        codegen_emit_instruction(gen, OP_DEC, 0x00, 1);
        codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &operand_var, sizeof(operand_var));
        
        // Update variable
        if (expr->as.unary.operand->type == EXPR_IDENTIFIER) {
          // Similar to ++ case
          Symbol* symbol = symbol_table_lookup(gen->symbols, expr->as.unary.operand->as.identifier.name);
          if (symbol) {
            if (symbol->is_global) {
              codegen_emit_instruction(gen, OP_VARSET, 0x00, 2);
              codegen_emit_operand(gen, OPQUAL_STR, 0x00, symbol->name, strlen(symbol->name) + 1);
              codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &operand_var, sizeof(operand_var));
            } else {
              codegen_emit_instruction(gen, OP_VARSET, 0x00, 2);
              int var_id = symbol->var_id;
              codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &var_id, sizeof(var_id));
              codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &operand_var, sizeof(operand_var));
            }
          }
        }
      } else {
        // Prefix --
        codegen_emit_instruction(gen, OP_DEC, 0x00, 1);
        codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &operand_var, sizeof(operand_var));
        
        // Update variable
        if (expr->as.unary.operand->type == EXPR_IDENTIFIER) {
          // Similar to ++ case
          Symbol* symbol = symbol_table_lookup(gen->symbols, expr->as.unary.operand->as.identifier.name);
          if (symbol) {
            if (symbol->is_global) {
              codegen_emit_instruction(gen, OP_VARSET, 0x00, 2);
              codegen_emit_operand(gen, OPQUAL_STR, 0x00, symbol->name, strlen(symbol->name) + 1);
              codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &operand_var, sizeof(operand_var));
            } else {
              codegen_emit_instruction(gen, OP_VARSET, 0x00, 2);
              int var_id = symbol->var_id;
              codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &var_id, sizeof(var_id));
              codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &operand_var, sizeof(operand_var));
            }
          }
        }
        
        codegen_emit_instruction(gen, OP_MOV, 0x00, 2);
        codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &result_var, sizeof(result_var));
        codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &operand_var, sizeof(operand_var));
      }
      return result_var;
      
    case TOKEN_AMPERSAND:
      // Address-of operator
      if (expr->as.unary.operand->type == EXPR_IDENTIFIER) {
        Symbol* symbol = symbol_table_lookup(gen->symbols, expr->as.unary.operand->as.identifier.name);
        if (symbol) {
          // Get variable reference
          codegen_emit_instruction(gen, OP_VARREF, 0x00, 2);
          codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &result_var, sizeof(result_var));
          
          if (symbol->is_global) {
            codegen_emit_operand(gen, OPQUAL_STR, 0x00, symbol->name, strlen(symbol->name) + 1);
          } else {
            int var_id = symbol->var_id;
            codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &var_id, sizeof(var_id));
          }
          
          return result_var;
        }
      }
      // Unsupported use of address-of
      gen->has_error = true;
      gen->error_message = arena_strdup(gen->arena, "Unsupported use of address-of operator");
      return -1;
      
    case TOKEN_STAR:
      // Dereference operator
      // Load from memory address
      codegen_emit_instruction(gen, OP_LOAD, 0x00, 2);
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &result_var, sizeof(result_var));
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &operand_var, sizeof(operand_var));
      return result_var;
      
    default:
      // Unsupported operator
      gen->has_error = true;
      gen->error_message = arena_strdup(gen->arena, "Unsupported unary operator");
      return -1;
  }
  
  // Emit the instruction for operators that use a simple opcode (e.g., NEG, NOT)
  codegen_emit_instruction(gen, opcode, qualifier, 2);
  codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &result_var, sizeof(result_var));
  codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &operand_var, sizeof(operand_var));
  
  return result_var;
}

int codegen_literal_expression(CodeGenerator* gen, Expr* expr) {
  // Create a result variable
  int result_var = gen->var_counter++;
  
  switch (expr->type) {
    case EXPR_INTEGER_LITERAL: {
      // Emit instruction to move immediate value
      codegen_emit_instruction(gen, OP_MOVI, 0x00, 2);
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &result_var, sizeof(result_var));
      codegen_emit_operand(gen, OPQUAL_IMM, 0x00, &expr->as.int_literal.value, sizeof(expr->as.int_literal.value));
      break;
    }
      
    case EXPR_FLOAT_LITERAL: {
      // Emit instruction to move immediate float value
      codegen_emit_instruction(gen, OP_MOVI, 0x00, 2);
      codegen_emit_operand(gen, OPQUAL_VAR, 0x02, &result_var, sizeof(result_var)); // Type 0x02 for FLOAT
      codegen_emit_operand(gen, OPQUAL_IMM, 0x02, &expr->as.float_literal.value, sizeof(expr->as.float_literal.value));
      break;
    }
      
    case EXPR_CHAR_LITERAL: {
      // Emit instruction to move immediate char value
      codegen_emit_instruction(gen, OP_MOVI, 0x00, 2);
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &result_var, sizeof(result_var));
      char ch = expr->as.char_literal.value;
      codegen_emit_operand(gen, OPQUAL_IMM, 0x00, &ch, sizeof(ch));
      break;
    }
      
    case EXPR_STRING_LITERAL: {
      // Add string to string table
      int str_id = codegen_add_string_literal(gen, expr->as.string_literal.value);
      
      // Emit instruction to get string pointer
      codegen_emit_instruction(gen, OP_MOVI, 0x00, 2);
      codegen_emit_operand(gen, OPQUAL_VAR, 0xF4, &result_var, sizeof(result_var)); // Type PTR
      codegen_emit_operand(gen, OPQUAL_STR, 0x00, &str_id, sizeof(str_id));
      break;
    }
      
    default:
      // Unsupported literal type
      gen->has_error = true;
      gen->error_message = arena_strdup(gen->arena, "Unsupported literal type");
      return -1;
  }
  
  return result_var;
}

int codegen_identifier_expression(CodeGenerator* gen, Expr* expr) {
  Symbol* symbol = symbol_table_lookup(gen->symbols, expr->as.identifier.name);
  if (!symbol) {
    gen->has_error = true;
    gen->error_message = arena_strdup(gen->arena, "Undefined variable");
    return -1;
  }
  
  // Create a result variable
  int result_var = gen->var_counter++;
  
  // Get variable value
  codegen_emit_instruction(gen, OP_VARGET, 0x00, 2);
  codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &result_var, sizeof(result_var));
  
  if (symbol->is_global) {
    // Global variable
    codegen_emit_operand(gen, OPQUAL_STR, 0x00, symbol->name, strlen(symbol->name) + 1);
  } else {
    // Local variable
    int var_id = symbol->var_id;
    codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &var_id, sizeof(var_id));
  }
  
  return result_var;
}

int codegen_assign_expression(CodeGenerator* gen, Expr* expr) {
  // Generate code for the right-hand side
  int value_var = codegen_expression(gen, expr->as.assign.value);
  
  // Check if left-hand side is an identifier
  if (expr->as.assign.target->type == EXPR_IDENTIFIER) {
    Symbol* symbol = symbol_table_lookup(gen->symbols, expr->as.assign.target->as.identifier.name);
    if (!symbol) {
      gen->has_error = true;
      gen->error_message = arena_strdup(gen->arena, "Undefined variable in assignment");
      return -1;
    }
    
    // Handle compound assignment operators
    if (expr->as.assign.operator != TOKEN_EQUAL) {
      // Get the current value of the variable
      int current_var = gen->var_counter++;
      codegen_emit_instruction(gen, OP_VARGET, 0x00, 2);
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &current_var, sizeof(current_var));
      
      if (symbol->is_global) {
        codegen_emit_operand(gen, OPQUAL_STR, 0x00, symbol->name, strlen(symbol->name) + 1);
      } else {
        int var_id = symbol->var_id;
        codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &var_id, sizeof(var_id));
      }
      
      // Perform the operation
      CoilOpcode opcode;
      switch (expr->as.assign.operator) {
        case TOKEN_PLUS_EQUAL:
          opcode = OP_ADD;
          break;
        case TOKEN_MINUS_EQUAL:
          opcode = OP_SUB;
          break;
        case TOKEN_STAR_EQUAL:
          opcode = OP_MUL;
          break;
        case TOKEN_SLASH_EQUAL:
          opcode = OP_DIV;
          break;
        case TOKEN_PERCENT_EQUAL:
          opcode = OP_MOD;
          break;
        case TOKEN_AMPERSAND_EQUAL:
          opcode = OP_AND;
          break;
        case TOKEN_PIPE_EQUAL:
          opcode = OP_OR;
          break;
        case TOKEN_CARET_EQUAL:
          opcode = OP_XOR;
          break;
        // More compound operators would go here
        default:
          gen->has_error = true;
          gen->error_message = arena_strdup(gen->arena, "Unsupported compound assignment operator");
          return -1;
      }
      
      int result_var = gen->var_counter++;
      codegen_emit_instruction(gen, opcode, 0x00, 3);
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &result_var, sizeof(result_var));
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &current_var, sizeof(current_var));
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &value_var, sizeof(value_var));
      
      // Update value_var to the result
      value_var = result_var;
    }
    
    // Set variable value
    codegen_emit_instruction(gen, OP_VARSET, 0x00, 2);
    
    if (symbol->is_global) {
      codegen_emit_operand(gen, OPQUAL_STR, 0x00, symbol->name, strlen(symbol->name) + 1);
    } else {
      int var_id = symbol->var_id;
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &var_id, sizeof(var_id));
    }
    
    codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &value_var, sizeof(value_var));
    
    return value_var;
  } else if (expr->as.assign.target->type == EXPR_UNARY && 
             expr->as.assign.target->as.unary.operator == TOKEN_STAR) {
    // Pointer assignment: *ptr = value
    int ptr_var = codegen_expression(gen, expr->as.assign.target->as.unary.operand);
    
    // Store to memory
    codegen_emit_instruction(gen, OP_STORE, 0x00, 2);
    codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &ptr_var, sizeof(ptr_var));
    codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &value_var, sizeof(value_var));
    
    return value_var;
  } else if (expr->as.assign.target->type == EXPR_INDEX) {
    // Array assignment: arr[index] = value
    int array_var = codegen_expression(gen, expr->as.assign.target->as.index.array);
    int index_var = codegen_expression(gen, expr->as.assign.target->as.index.index);
    
    // Calculate the address: arr + index * element_size
    int element_size = 4; // Default to 4 bytes (int)
    if (expr->as.assign.target->result_type && 
        expr->as.assign.target->result_type->kind == TYPE_POINTER &&
        expr->as.assign.target->result_type->as.pointer.element_type) {
      // Calculate size based on element type
      switch (expr->as.assign.target->result_type->as.pointer.element_type->kind) {
        case TYPE_CHAR:
          element_size = 1;
          break;
        case TYPE_SHORT:
          element_size = 2;
          break;
        case TYPE_INT:
          element_size = 4;
          break;
        case TYPE_LONG:
        case TYPE_DOUBLE:
          element_size = 8;
          break;
        default:
          element_size = 4;
          break;
      }
    }
    
    // Multiply index by element size
    int scaled_index_var = gen->var_counter++;
    if (element_size > 1) {
      int size_var = gen->var_counter++;
      codegen_emit_instruction(gen, OP_MOVI, 0x00, 2);
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &size_var, sizeof(size_var));
      codegen_emit_operand(gen, OPQUAL_IMM, 0x00, &element_size, sizeof(element_size));
      
      codegen_emit_instruction(gen, OP_MUL, 0x00, 3);
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &scaled_index_var, sizeof(scaled_index_var));
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &index_var, sizeof(index_var));
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &size_var, sizeof(size_var));
    } else {
      // No scaling needed for char arrays
      codegen_emit_instruction(gen, OP_MOV, 0x00, 2);
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &scaled_index_var, sizeof(scaled_index_var));
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &index_var, sizeof(index_var));
    }
    
    // Calculate address: array + scaled_index
    int addr_var = gen->var_counter++;
    codegen_emit_instruction(gen, OP_ADD, 0x00, 3);
    codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &addr_var, sizeof(addr_var));
    codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &array_var, sizeof(array_var));
    codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &scaled_index_var, sizeof(scaled_index_var));
    
    // Store value to calculated address
    codegen_emit_instruction(gen, OP_STORE, 0x00, 2);
    codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &addr_var, sizeof(addr_var));
    codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &value_var, sizeof(value_var));
    
    return value_var;
  } else {
    gen->has_error = true;
    gen->error_message = arena_strdup(gen->arena, "Unsupported assignment target");
    return -1;
  }
}

int codegen_call_expression(CodeGenerator* gen, Expr* expr) {
  // Create variables for arguments
  int* arg_vars = arena_alloc(gen->arena, sizeof(int) * expr->as.call.arg_count);
  
  // Generate code for each argument
  for (int i = 0; i < expr->as.call.arg_count; i++) {
    arg_vars[i] = codegen_expression(gen, expr->as.call.arguments[i]);
  }
  
  // Generate code for the function pointer
  int func_var = codegen_expression(gen, expr->as.call.function);
  
  // Emit parameter setup
  for (int i = 0; i < expr->as.call.arg_count; i++) {
    codegen_emit_instruction(gen, OP_PARAM, i, 1);
    codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &arg_vars[i], sizeof(arg_vars[i]));
  }
  
  // Create a result variable
  int result_var = gen->var_counter++;
  
  // Emit function call
  codegen_emit_instruction(gen, OP_CALL, 0x00, 1);
  codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &func_var, sizeof(func_var));
  
  // Get function result
  codegen_emit_instruction(gen, OP_RESULT, 0x00, 1);
  codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &result_var, sizeof(result_var));
  
  return result_var;
}

int codegen_expression(CodeGenerator* gen, Expr* expr) {
  if (!expr) return -1;
  
  switch (expr->type) {
    case EXPR_BINARY:
      return codegen_binary_expression(gen, expr);
      
    case EXPR_UNARY:
      return codegen_unary_expression(gen, expr);
      
    case EXPR_INTEGER_LITERAL:
    case EXPR_FLOAT_LITERAL:
    case EXPR_STRING_LITERAL:
    case EXPR_CHAR_LITERAL:
      return codegen_literal_expression(gen, expr);
      
    case EXPR_IDENTIFIER:
      return codegen_identifier_expression(gen, expr);
      
    case EXPR_ASSIGN:
      return codegen_assign_expression(gen, expr);
      
    case EXPR_CALL:
      return codegen_call_expression(gen, expr);
      
    // More expression types would be handled here
      
    default:
      gen->has_error = true;
      gen->error_message = arena_strdup(gen->arena, "Unsupported expression type");
      return -1;
  }
}

/* Statement code generation */

void codegen_expression_statement(CodeGenerator* gen, Stmt* stmt) {
  codegen_expression(gen, stmt->as.expr.expr);
}

void codegen_block_statement(CodeGenerator* gen, Stmt* stmt) {
  // Enter a new scope
  symbol_table_enter_scope(gen->symbols);
  
  // Emit variable scope creation
  codegen_emit_instruction(gen, OP_VARSC, 0x00, 0);
  
  // Generate code for each statement in the block
  for (int i = 0; i < stmt->as.block.count; i++) {
    codegen_statement(gen, stmt->as.block.statements[i]);
  }
  
  // Emit variable scope end
  codegen_emit_instruction(gen, OP_VAREND, 0x00, 0);
  
  // Exit the scope
  symbol_table_exit_scope(gen->symbols);
}

void codegen_if_statement(CodeGenerator* gen, Stmt* stmt) {
  // Generate code for the condition
  int cond_var = codegen_expression(gen, stmt->as.if_stmt.condition);
  
  // Create labels for branches
  int false_label = codegen_new_label(gen);
  int end_label = codegen_new_label(gen);
  
  // Compare condition with zero
  int zero_var = gen->var_counter++;
  codegen_emit_instruction(gen, OP_MOVI, 0x00, 2);
  codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &zero_var, sizeof(zero_var));
  int zero = 0;
  codegen_emit_operand(gen, OPQUAL_IMM, 0x00, &zero, sizeof(zero));
  
  codegen_emit_instruction(gen, OP_CMP, 0x00, 2);
  codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &cond_var, sizeof(cond_var));
  codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &zero_var, sizeof(zero_var));
  
  // Branch if condition is false
  codegen_emit_instruction(gen, OP_BRC, BR_EQ, 1);
  codegen_emit_operand(gen, OPQUAL_LBL, 0x00, &false_label, sizeof(false_label));
  
  // Generate code for 'then' branch
  codegen_statement(gen, stmt->as.if_stmt.then_branch);
  
  // Jump to end if there's an 'else' branch
  if (stmt->as.if_stmt.else_branch) {
    codegen_emit_instruction(gen, OP_BR, 0x00, 1);
    codegen_emit_operand(gen, OPQUAL_LBL, 0x00, &end_label, sizeof(end_label));
  }
  
  // False label
  codegen_emit_label(gen, false_label);
  
  // Generate code for 'else' branch if it exists
  if (stmt->as.if_stmt.else_branch) {
    codegen_statement(gen, stmt->as.if_stmt.else_branch);
    
    // End label
    codegen_emit_label(gen, end_label);
  }
}

void codegen_while_statement(CodeGenerator* gen, Stmt* stmt) {
  // Create labels
  int start_label = codegen_new_label(gen);
  int end_label = codegen_new_label(gen);
  
  // Start label
  codegen_emit_label(gen, start_label);
  
  // Generate code for condition
  int cond_var = codegen_expression(gen, stmt->as.while_stmt.condition);
  
  // Compare condition with zero
  int zero_var = gen->var_counter++;
  codegen_emit_instruction(gen, OP_MOVI, 0x00, 2);
  codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &zero_var, sizeof(zero_var));
  int zero = 0;
  codegen_emit_operand(gen, OPQUAL_IMM, 0x00, &zero, sizeof(zero));
  
  codegen_emit_instruction(gen, OP_CMP, 0x00, 2);
  codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &cond_var, sizeof(cond_var));
  codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &zero_var, sizeof(zero_var));
  
  // Branch to end if condition is false
  codegen_emit_instruction(gen, OP_BRC, BR_EQ, 1);
  codegen_emit_operand(gen, OPQUAL_LBL, 0x00, &end_label, sizeof(end_label));
  
  // Generate code for loop body
  codegen_statement(gen, stmt->as.while_stmt.body);
  
  // Jump back to start
  codegen_emit_instruction(gen, OP_BR, 0x00, 1);
  codegen_emit_operand(gen, OPQUAL_LBL, 0x00, &start_label, sizeof(start_label));
  
  // End label
  codegen_emit_label(gen, end_label);
}

void codegen_return_statement(CodeGenerator* gen, Stmt* stmt) {
  if (stmt->as.return_stmt.value) {
    // Generate code for return value
    int value_var = codegen_expression(gen, stmt->as.return_stmt.value);
    
    // Set function result
    codegen_emit_instruction(gen, OP_RESULT, 0x00, 1);
    codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &value_var, sizeof(value_var));
  }
  
  // Return from function
  codegen_emit_instruction(gen, OP_RET, 0x00, 0);
}

void codegen_declaration_statement(CodeGenerator* gen, Stmt* stmt) {
  codegen_declaration(gen, stmt->as.decl_stmt.decl);
}

void codegen_statement(CodeGenerator* gen, Stmt* stmt) {
  if (!stmt) return;
  
  switch (stmt->type) {
    case STMT_EXPR:
      codegen_expression_statement(gen, stmt);
      break;
      
    case STMT_BLOCK:
      codegen_block_statement(gen, stmt);
      break;
      
    case STMT_IF:
      codegen_if_statement(gen, stmt);
      break;
      
    case STMT_WHILE:
      codegen_while_statement(gen, stmt);
      break;
      
    case STMT_RETURN:
      codegen_return_statement(gen, stmt);
      break;
      
    case STMT_DECL:
      codegen_declaration_statement(gen, stmt);
      break;
      
    // More statement types would be handled here
      
    default:
      gen->has_error = true;
      gen->error_message = arena_strdup(gen->arena, "Unsupported statement type");
      break;
  }
}

/* Declaration code generation */

void codegen_variable_declaration(CodeGenerator* gen, Decl* decl) {
  // Get COIL type
  uint8_t coil_type = codegen_map_type(decl->declared_type);
  
  if (decl->is_static || decl->is_extern) {
    // Global variable - add to symbol table
    symbol_table_add(gen->symbols, decl->name, decl->declared_type, true, -1);
    
    // No code generation for extern
    if (decl->is_extern) return;
    
    // TODO: Emit appropriate directives for global variables
    
  } else {
    // Local variable
    int var_id = gen->var_counter++;
    
    // Add to symbol table
    symbol_table_add(gen->symbols, decl->name, decl->declared_type, false, var_id);
    
    // Emit variable creation
    codegen_emit_instruction(gen, OP_VARCR, 0x00, 2);
    codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &var_id, sizeof(var_id));
    codegen_emit_operand(gen, OPQUAL_IMM, coil_type, NULL, 0);
    
    // Initialize if there's an initializer
    if (decl->as.var.initializer) {
      int init_var = codegen_expression(gen, decl->as.var.initializer);
      
      codegen_emit_instruction(gen, OP_VARSET, 0x00, 2);
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &var_id, sizeof(var_id));
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &init_var, sizeof(init_var));
    }
  }
}

void codegen_function_declaration(CodeGenerator* gen, Decl* decl) {
  // Skip if just a prototype
  if (!decl->as.func.body) return;
  
  // Add to symbol table
  symbol_table_add(gen->symbols, decl->name, decl->declared_type, true, -1);
  
  // Emit function symbol directive
  // TODO: Emit appropriate directives for function symbol
  
  // Save current function return type
  Type* prev_return_type = gen->current_function_return_type;
  gen->current_function_return_type = decl->declared_type->as.function.return_type;
  
  // Enter function scope
  symbol_table_enter_scope(gen->symbols);
  
  // Emit function prologue
  codegen_emit_instruction(gen, OP_ENTER, 0x00, 0);
  
  // Create variable scope for parameters
  codegen_emit_instruction(gen, OP_VARSC, 0x00, 0);
  
  // Add parameters to symbol table
  if (decl->declared_type->as.function.param_count > 0 && decl->as.func.param_names) {
    for (int i = 0; i < decl->declared_type->as.function.param_count; i++) {
      // Create variable for parameter
      int var_id = gen->var_counter++;
      Type* param_type = decl->declared_type->as.function.param_types[i];
      uint8_t coil_type = codegen_map_type(param_type);
      
      // Add to symbol table
      symbol_table_add(gen->symbols, decl->as.func.param_names[i], param_type, false, var_id);
      
      // Create parameter variable
      codegen_emit_instruction(gen, OP_VARCR, 0x00, 2);
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &var_id, sizeof(var_id));
      codegen_emit_operand(gen, OPQUAL_IMM, coil_type, NULL, 0);
      
      // Set parameter value
      codegen_emit_instruction(gen, OP_PARAM, i, 1);
      codegen_emit_operand(gen, OPQUAL_VAR, 0x00, &var_id, sizeof(var_id));
    }
  }
  
  // Generate code for function body
  codegen_statement(gen, decl->as.func.body);
  
  // End parameter scope
  codegen_emit_instruction(gen, OP_VAREND, 0x00, 0);
  
  // Emit function epilogue
  codegen_emit_instruction(gen, OP_LEAVE, 0x00, 0);
  
  // Emit return if there isn't one at the end
  codegen_emit_instruction(gen, OP_RET, 0x00, 0);
  
  // Exit function scope
  symbol_table_exit_scope(gen->symbols);
  
  // Restore previous function return type
  gen->current_function_return_type = prev_return_type;
}

void codegen_declaration(CodeGenerator* gen, Decl* decl) {
  if (!decl) return;
  
  switch (decl->type) {
    case DECL_VAR:
      codegen_variable_declaration(gen, decl);
      break;
      
    case DECL_FUNC:
      codegen_function_declaration(gen, decl);
      break;
      
    // More declaration types would be handled here
      
    default:
      gen->has_error = true;
      gen->error_message = arena_strdup(gen->arena, "Unsupported declaration type");
      break;
  }
}

bool codegen_generate(CodeGenerator* gen) {
  // Emit COF header
  // TODO: Implement proper COF header generation
  
  // Global scope
  gen->current_scope = 0;
  
  // Generate code for each declaration
  for (int i = 0; i < gen->program->count; i++) {
    codegen_declaration(gen, gen->program->declarations[i]);
    
    if (gen->has_error) {
      return false;
    }
  }
  
  return true;
}