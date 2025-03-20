/**
 * @file codegen.h
 * @brief COIL code generation for C compiler
 */

#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include <stdio.h>
#include <stdint.h>

// Forward declaration for Arena
struct Arena;

/**
 * @brief Symbol information for code generation
 */
typedef struct {
  char* name;
  Type* type;
  int scope_level;
  bool is_global;
  int var_id;  // COIL variable ID (for locals)
} Symbol;

/**
 * @brief Symbol table entry for code generation
 */
typedef struct SymbolEntry {
  Symbol symbol;
  struct SymbolEntry* next;
} SymbolEntry;

/**
 * @brief Symbol table for code generation
 */
typedef struct {
  SymbolEntry** buckets;
  int bucket_count;
  int size;
  int current_scope;
  struct Arena* arena;
} SymbolTable;

/**
 * @brief COIL instruction opcodes (from ISA.md)
 */
typedef enum {
  // Control Flow (0x01 - 0x0F)
  OP_SYMB = 0x01, // Define a symbol
  OP_BR   = 0x02, // Unconditional branch to location
  OP_BRC  = 0x03, // Conditional branch to location
  OP_CALL = 0x04, // Call subroutine
  OP_RET  = 0x05, // Return from subroutine

  // Arithmetic Operations (0x10 - 0x1F)
  OP_ADD  = 0x10, // Addition (for any numeric type)
  OP_SUB  = 0x11, // Subtraction (for any numeric type)
  OP_MUL  = 0x12, // Multiplication (for any numeric type)
  OP_DIV  = 0x13, // Division (for any numeric type)
  OP_MOD  = 0x14, // Modulus (for any numeric type)
  OP_NEG  = 0x15, // Negation (for any numeric type)
  OP_INC  = 0x17, // Increment (for any numeric type)
  OP_DEC  = 0x18, // Decrement (for any numeric type)

  // Bit Manipulation (0x20 - 0x2F)
  OP_AND  = 0x20, // Bitwise AND
  OP_OR   = 0x21, // Bitwise OR
  OP_XOR  = 0x22, // Bitwise XOR
  OP_NOT  = 0x23, // Bitwise NOT
  OP_SHL  = 0x24, // Shift left
  OP_SHR  = 0x25, // Shift right logical
  OP_SAR  = 0x26, // Shift right arithmetic

  // Comparison Operations (0x30 - 0x3F)
  OP_CMP  = 0x30, // Compare values and set flags

  // Data Movement (0x40 - 0x4F)
  OP_MOV   = 0x40, // Move data
  OP_LOAD  = 0x41, // Load from memory
  OP_STORE = 0x42, // Store to memory
  OP_MOVI  = 0x45, // Move immediate value

  // Variable Operations (0x60 - 0x6F)
  OP_VARCR = 0x60, // Create variable
  OP_VARDL = 0x61, // Delete variable
  OP_VARSC = 0x62, // Create variable scope
  OP_VAREND = 0x63, // End variable scope
  OP_VARGET = 0x64, // Get variable value
  OP_VARSET = 0x65, // Set variable value
  OP_VARREF = 0x66, // Get variable reference

  // Conversion Operations (0x70 - 0x7F)
  OP_FTOI  = 0x73, // Float to integer
  OP_ITOF  = 0x74, // Integer to float

  // Function Support (0xC0 - 0xCF)
  OP_ENTER  = 0xC0, // Function prologue
  OP_LEAVE  = 0xC1, // Function epilogue
  OP_PARAM  = 0xC2, // Function parameter
  OP_RESULT = 0xC3  // Function result
} CoilOpcode;

/**
 * @brief COIL operand qualifiers (from ISA.md)
 */
typedef enum {
  OPQUAL_IMM = 0x01, // Value is an immediate constant
  OPQUAL_VAR = 0x02, // Value refers to a variable
  OPQUAL_REG = 0x03, // Value refers to a virtual register
  OPQUAL_MEM = 0x04, // Value refers to a memory address
  OPQUAL_LBL = 0x05, // Value refers to a label
  OPQUAL_STR = 0x06, // Value is a string literal
  OPQUAL_SYM = 0x07  // Value refers to a symbol
} CoilOperandQualifier;

/**
 * @brief COIL branch condition qualifiers (from ISA.md)
 */
typedef enum {
  BR_ALWAYS = 0x00, // Always branch (unconditional)
  BR_EQ     = 0x01, // Equal / Zero
  BR_NE     = 0x02, // Not equal / Not zero
  BR_LT     = 0x03, // Less than
  BR_LE     = 0x04, // Less than or equal
  BR_GT     = 0x05, // Greater than
  BR_GE     = 0x06  // Greater than or equal
} CoilBranchQualifier;

/**
 * @brief COIL code generator state
 */
typedef struct {
  Program* program;
  SymbolTable* symbols;
  struct Arena* arena;
  FILE* output;
  
  // Code generation state
  int var_counter;
  int label_counter;
  int current_scope;
  Type* current_function_return_type;
  
  // String table for literals
  char** string_literals;
  int string_count;
  
  // Error handling
  bool has_error;
  char* error_message;
} CodeGenerator;

/**
 * @brief Initialize a symbol table
 * @param arena Memory arena for allocations
 * @return New symbol table
 */
SymbolTable* symbol_table_create(struct Arena* arena);

/**
 * @brief Enter a new scope in the symbol table
 * @param table Symbol table
 */
void symbol_table_enter_scope(SymbolTable* table);

/**
 * @brief Exit the current scope in the symbol table
 * @param table Symbol table
 */
void symbol_table_exit_scope(SymbolTable* table);

/**
 * @brief Add a symbol to the symbol table
 * @param table Symbol table
 * @param name Symbol name
 * @param type Symbol type
 * @param is_global Whether the symbol is global
 * @param var_id COIL variable ID for the symbol (for locals)
 * @return Added symbol, or NULL if symbol already exists in current scope
 */
Symbol* symbol_table_add(SymbolTable* table, const char* name, Type* type, bool is_global, int var_id);

/**
 * @brief Look up a symbol in the symbol table
 * @param table Symbol table
 * @param name Symbol name
 * @return Found symbol, or NULL if not found
 */
Symbol* symbol_table_lookup(SymbolTable* table, const char* name);

/**
 * @brief Initialize a code generator
 * @param program The AST to generate code for
 * @param output Output file for COIL binary
 * @param arena Memory arena for allocations
 * @return New code generator
 */
CodeGenerator* codegen_create(Program* program, FILE* output, struct Arena* arena);

/**
 * @brief Generate COIL code for a program
 * @param gen Code generator
 * @return true if code generation succeeded
 */
bool codegen_generate(CodeGenerator* gen);

/**
 * @brief Map C type to COIL type
 * @param type C type
 * @return COIL type code
 */
uint8_t codegen_map_type(Type* type);

/**
 * @brief Emit a COIL instruction
 * @param gen Code generator
 * @param opcode Instruction opcode
 * @param qualifier Instruction qualifier
 * @param operand_count Number of operands
 */
void codegen_emit_instruction(CodeGenerator* gen, uint8_t opcode, uint8_t qualifier, uint8_t operand_count);

/**
 * @brief Emit a COIL operand
 * @param gen Code generator
 * @param qualifier Operand qualifier
 * @param type Operand type
 * @param data Operand data
 * @param size Size of data in bytes
 */
void codegen_emit_operand(CodeGenerator* gen, uint8_t qualifier, uint8_t type, const void* data, size_t size);

/**
 * @brief Generate COIL code for an expression
 * @param gen Code generator
 * @param expr Expression to generate code for
 * @return Variable ID containing the result
 */
int codegen_expression(CodeGenerator* gen, Expr* expr);

/**
 * @brief Generate COIL code for a statement
 * @param gen Code generator
 * @param stmt Statement to generate code for
 */
void codegen_statement(CodeGenerator* gen, Stmt* stmt);

/**
 * @brief Generate COIL code for a declaration
 * @param gen Code generator
 * @param decl Declaration to generate code for
 */
void codegen_declaration(CodeGenerator* gen, Decl* decl);

/**
 * @brief Generate a new unique label
 * @param gen Code generator
 * @return New label ID
 */
int codegen_new_label(CodeGenerator* gen);

/**
 * @brief Emit a label definition
 * @param gen Code generator
 * @param label_id Label ID
 */
void codegen_emit_label(CodeGenerator* gen, int label_id);

/**
 * @brief Create a unique string identifier for a literal
 * @param gen Code generator
 * @param str String literal
 * @return String ID
 */
int codegen_add_string_literal(CodeGenerator* gen, const char* str);

#endif /* CODEGEN_H */