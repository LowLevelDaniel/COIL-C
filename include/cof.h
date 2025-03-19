/**
 * @file cof.h
 * @brief COIL Object Format (COF) handling
 *
 * Defines functions for generating COIL binary format files according
 * to the format described in the COIL 1.0.0 specification (obj.md).
 */

#ifndef COF_H
#define COF_H

#include <stdio.h>
#include <stdint.h>
#include "ast.h"
#include "coil_constants.h"

/**
 * Initialize the string table for COF generation
 */
void string_table_init();

/**
 * Add a string to the string table and return its offset
 * @param str String to add
 * @return Offset in the string table
 */
uint32_t string_table_add(const char *str);

/**
 * Write the string table to the output file
 * @param output Output file
 */
void string_table_write(FILE *output);

/**
 * Free the string table resources
 */
void string_table_free();

/**
 * Binary format utility functions that explicitly handle endianness
 * to ensure consistent output regardless of host platform
 */

/**
 * Write an 8-bit unsigned integer to the output file
 * @param output Output file
 * @param value Value to write
 */
void write_u8(FILE *output, uint8_t value);

/**
 * Write a 16-bit unsigned integer to the output file in little-endian format
 * @param output Output file
 * @param value Value to write
 */
void write_u16(FILE *output, uint16_t value);

/**
 * Write a 32-bit unsigned integer to the output file in little-endian format
 * @param output Output file
 * @param value Value to write
 */
void write_u32(FILE *output, uint32_t value);

/**
 * Write a 32-bit signed integer to the output file in little-endian format
 * @param output Output file
 * @param value Value to write
 */
void write_i32(FILE *output, int32_t value);

/**
 * Write a 64-bit unsigned integer to the output file in little-endian format
 * @param output Output file
 * @param value Value to write
 */
void write_u64(FILE *output, uint64_t value);

/**
 * Write a 32-bit floating-point value to the output file
 * @param output Output file
 * @param value Value to write
 */
void write_f32(FILE *output, float value);

/**
 * Write a 64-bit floating-point value to the output file
 * @param output Output file
 * @param value Value to write
 */
void write_f64(FILE *output, double value);

/**
 * Write a string with its length prefix to the output file
 * @param output Output file
 * @param str String to write
 */
void write_string(FILE *output, const char *str);

/**
 * Emit a COIL instruction with its opcode, qualifier, and operand count
 * @param output Output file
 * @param opcode Instruction opcode
 * @param qualifier Instruction qualifier
 * @param operand_count Number of operands
 */
void emit_instruction(FILE *output, uint8_t opcode, uint8_t qualifier, uint8_t operand_count);

/**
 * Emit a register operand
 * @param output Output file
 * @param type Type of the register
 * @param width Width of the register in bytes
 * @param reg_num Register number
 */
void emit_register_operand(FILE *output, uint8_t type, uint8_t width, uint8_t reg_num);

/**
 * Emit an immediate 32-bit integer operand
 * @param output Output file
 * @param type Type of the immediate value
 * @param value 32-bit integer value
 */
void emit_immediate_operand_i32(FILE *output, uint8_t type, int32_t value);

/**
 * Emit a label operand
 * @param output Output file
 * @param label Label number
 */
void emit_label_operand(FILE *output, int label);

/**
 * Emit a symbol operand
 * @param output Output file
 * @param name Symbol name
 */
void emit_symbol_operand(FILE *output, const char *name);

/**
 * Emit a memory operand (address with offset)
 * @param output Output file
 * @param type Type of memory access
 * @param width Width of memory access in bytes
 * @param base_reg Base register for address
 * @param offset Offset from base register
 */
void emit_memory_operand(FILE *output, uint8_t type, uint8_t width, uint8_t base_reg, int32_t offset);

/**
 * Emit a symbol table entry
 * @param output Output file
 * @param name Symbol name
 * @param value Symbol value
 * @param type Symbol type
 * @param binding Symbol binding
 */
void emit_symbol_entry(FILE *output, const char *name, uint32_t value, uint8_t type, uint8_t binding);

/**
 * High-level COIL instruction emitters
 */

/**
 * Emit a NOP (no operation) instruction
 * @param output Output file
 */
void emit_nop(FILE *output);

/**
 * Emit a MOV (move register to register) instruction
 * @param output Output file
 * @param dest_reg Destination register
 * @param src_reg Source register
 */
void emit_mov(FILE *output, uint8_t dest_reg, uint8_t src_reg);

/**
 * Emit a MOVI (move immediate to register) instruction
 * @param output Output file
 * @param dest_reg Destination register
 * @param value Immediate value
 */
void emit_movi(FILE *output, uint8_t dest_reg, int32_t value);

/**
 * Emit an ADD (addition) instruction
 * @param output Output file
 * @param dest_reg Destination register
 * @param src1_reg First source register
 * @param src2_reg Second source register
 */
void emit_add(FILE *output, uint8_t dest_reg, uint8_t src1_reg, uint8_t src2_reg);

/**
 * Emit a SUB (subtraction) instruction
 * @param output Output file
 * @param dest_reg Destination register
 * @param src1_reg First source register
 * @param src2_reg Second source register
 */
void emit_sub(FILE *output, uint8_t dest_reg, uint8_t src1_reg, uint8_t src2_reg);

/**
 * Emit a MUL (multiplication) instruction
 * @param output Output file
 * @param dest_reg Destination register
 * @param src1_reg First source register
 * @param src2_reg Second source register
 */
void emit_mul(FILE *output, uint8_t dest_reg, uint8_t src1_reg, uint8_t src2_reg);

/**
 * Emit a DIV (division) instruction
 * @param output Output file
 * @param dest_reg Destination register
 * @param src1_reg First source register
 * @param src2_reg Second source register
 */
void emit_div(FILE *output, uint8_t dest_reg, uint8_t src1_reg, uint8_t src2_reg);

/**
 * Emit a CMP (compare) instruction
 * @param output Output file
 * @param src1_reg First source register
 * @param src2_reg Second source register
 */
void emit_cmp(FILE *output, uint8_t src1_reg, uint8_t src2_reg);

/**
 * Emit a JMP (unconditional jump) instruction
 * @param output Output file
 * @param label Target label
 */
void emit_jmp(FILE *output, int label);

/**
 * Emit a JCC (conditional jump) instruction
 * @param output Output file
 * @param condition Jump condition
 * @param label Target label
 */
void emit_jcc(FILE *output, uint8_t condition, int label);

/**
 * Emit a CALL (function call) instruction
 * @param output Output file
 * @param function_name Function name
 */
void emit_call(FILE *output, const char *function_name);

/**
 * Emit a RET (return from function) instruction
 * @param output Output file
 */
void emit_ret(FILE *output);

/**
 * Emit a LOAD (load from memory) instruction
 * @param output Output file
 * @param dest_reg Destination register
 * @param addr_reg Address register
 * @param offset Memory offset
 */
void emit_load(FILE *output, uint8_t dest_reg, uint8_t addr_reg, int offset);

/**
 * Emit a STORE (store to memory) instruction
 * @param output Output file
 * @param src_reg Source register
 * @param addr_reg Address register
 * @param offset Memory offset
 */
void emit_store(FILE *output, uint8_t src_reg, uint8_t addr_reg, int offset);

/**
 * Emit a PUSH (push to stack) instruction
 * @param output Output file
 * @param src_reg Source register
 */
void emit_push(FILE *output, uint8_t src_reg);

/**
 * Emit a POP (pop from stack) instruction
 * @param output Output file
 * @param dest_reg Destination register
 */
void emit_pop(FILE *output, uint8_t dest_reg);

/**
 * Emit an ENTER (function prologue) instruction
 * @param output Output file
 * @param frame_size Stack frame size
 */
void emit_enter(FILE *output, int frame_size);

/**
 * Emit a LEAVE (function epilogue) instruction
 * @param output Output file
 */
void emit_leave(FILE *output);

/**
 * Emit a label (symbolic location in code)
 * @param output Output file
 * @param label Label number
 */
void emit_label(FILE *output, int label);

/**
 * COIL directive emission functions
 */

/**
 * Emit a version directive
 * @param output Output file
 * @param major Major version
 * @param minor Minor version
 * @param patch Patch version
 */
void emit_version_directive(FILE *output, uint8_t major, uint8_t minor, uint8_t patch);

/**
 * Emit a target directive
 * @param output Output file
 * @param target_id Target architecture ID
 */
void emit_target_directive(FILE *output, uint16_t target_id);

/**
 * Emit a section directive
 * @param output Output file
 * @param section_type Section type
 * @param name Section name
 * @param flags Section flags
 */
void emit_section_directive(FILE *output, uint8_t section_type, const char *name, uint8_t flags);

/**
 * Emit a symbol directive
 * @param output Output file
 * @param qualifier Symbol qualifier
 * @param name Symbol name
 * @param value Symbol value
 */
void emit_symbol_directive(FILE *output, uint8_t qualifier, const char *name, uint64_t value);

/**
 * Write the symbol table to the output file
 * @param output Output file
 */
void write_symbol_table(FILE *output);

/**
 * Generate a COF file header
 * @param output Output file
 * @param program Program being compiled
 */
void generate_cof_header(FILE *output);

/**
 * Update the COF header with final sizes and offsets
 * @param output Output file
 * @param entrypoint Entrypoint offset
 * @param code_size Code section size
 */
void update_cof_header(FILE *output, uint32_t entrypoint);

#endif /* COF_H */