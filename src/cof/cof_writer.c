/**
* COIL Object Format (COF) writing functions
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "cof.h"

/* Binary format utility functions */
void write_u8(FILE *output, uint8_t value) {
  fwrite(&value, sizeof(uint8_t), 1, output);
}

void write_u16(FILE *output, uint16_t value) {
  fwrite(&value, sizeof(uint16_t), 1, output);
}

void write_u32(FILE *output, uint32_t value) {
  fwrite(&value, sizeof(uint32_t), 1, output);
}

void write_i32(FILE *output, int32_t value) {
  fwrite(&value, sizeof(int32_t), 1, output);
}

void write_u64(FILE *output, uint64_t value) {
  fwrite(&value, sizeof(uint64_t), 1, output);
}

void write_f32(FILE *output, float value) {
  fwrite(&value, sizeof(float), 1, output);
}

void write_f64(FILE *output, double value) {
  fwrite(&value, sizeof(double), 1, output);
}

void write_string(FILE *output, const char *str) {
  uint8_t len = strlen(str);
  write_u8(output, len);
  fwrite(str, sizeof(char), len, output);
}

/* COIL instruction emission functions */
void emit_instruction(FILE *output, uint8_t opcode, uint8_t qualifier, uint8_t operand_count) {
  write_u8(output, opcode);
  write_u8(output, qualifier);
  write_u8(output, operand_count);
}

void emit_register_operand(FILE *output, uint8_t type, uint8_t width, uint8_t reg_num) {
  write_u8(output, OPQUAL_REG);
  write_u8(output, type);
  write_u8(output, width);
  write_u8(output, reg_num);
}

void emit_immediate_operand_i32(FILE *output, uint8_t type, int32_t value) {
  write_u8(output, OPQUAL_IMM);
  write_u8(output, type);
  write_u8(output, 0x04); // 32-bit width
  write_i32(output, value);
}

void emit_label_operand(FILE *output, int label) {
  write_u8(output, OPQUAL_LBL);
  write_u8(output, COIL_TYPE_VOID);
  write_u8(output, 0x00); // No additional type info
  write_i32(output, label);
}

void emit_symbol_operand(FILE *output, const char *name) {
  write_u8(output, OPQUAL_SYM);
  write_u8(output, COIL_TYPE_VOID);
  write_u8(output, 0x00); // No additional type info
  write_string(output, name);
}

void emit_memory_operand(FILE *output, uint8_t type, uint8_t width, uint8_t base_reg, int32_t offset) {
  write_u8(output, OPQUAL_MEM);
  write_u8(output, type);
  write_u8(output, width);
  write_u8(output, base_reg);
  write_i32(output, offset);
}

/* High-level COIL instruction emitters */

void emit_nop(FILE *output) {
  emit_instruction(output, OP_NOP, 0x00, 0x00);
}

void emit_mov(FILE *output, uint8_t dest_reg, uint8_t src_reg) {
  emit_instruction(output, OP_MOV, 0x00, 0x02);
  emit_register_operand(output, COIL_TYPE_INT, 0x04, dest_reg); // 32-bit int
  emit_register_operand(output, COIL_TYPE_INT, 0x04, src_reg);
}

void emit_movi(FILE *output, uint8_t dest_reg, int32_t value) {
  emit_instruction(output, OP_MOVI, 0x00, 0x02);
  emit_register_operand(output, COIL_TYPE_INT, 0x04, dest_reg);
  emit_immediate_operand_i32(output, COIL_TYPE_INT, value);
}

void emit_add(FILE *output, uint8_t dest_reg, uint8_t src1_reg, uint8_t src2_reg) {
  emit_instruction(output, OP_ADD, 0x00, 0x03);
  emit_register_operand(output, COIL_TYPE_INT, 0x04, dest_reg);
  emit_register_operand(output, COIL_TYPE_INT, 0x04, src1_reg);
  emit_register_operand(output, COIL_TYPE_INT, 0x04, src2_reg);
}

void emit_sub(FILE *output, uint8_t dest_reg, uint8_t src1_reg, uint8_t src2_reg) {
  emit_instruction(output, OP_SUB, 0x00, 0x03);
  emit_register_operand(output, COIL_TYPE_INT, 0x04, dest_reg);
  emit_register_operand(output, COIL_TYPE_INT, 0x04, src1_reg);
  emit_register_operand(output, COIL_TYPE_INT, 0x04, src2_reg);
}

void emit_mul(FILE *output, uint8_t dest_reg, uint8_t src1_reg, uint8_t src2_reg) {
  emit_instruction(output, OP_MUL, 0x00, 0x03);
  emit_register_operand(output, COIL_TYPE_INT, 0x04, dest_reg);
  emit_register_operand(output, COIL_TYPE_INT, 0x04, src1_reg);
  emit_register_operand(output, COIL_TYPE_INT, 0x04, src2_reg);
}

void emit_div(FILE *output, uint8_t dest_reg, uint8_t src1_reg, uint8_t src2_reg) {
  emit_instruction(output, OP_DIV, 0x00, 0x03);
  emit_register_operand(output, COIL_TYPE_INT, 0x04, dest_reg);
  emit_register_operand(output, COIL_TYPE_INT, 0x04, src1_reg);
  emit_register_operand(output, COIL_TYPE_INT, 0x04, src2_reg);
}

void emit_cmp(FILE *output, uint8_t src1_reg, uint8_t src2_reg) {
  emit_instruction(output, OP_CMP, 0x00, 0x02);
  emit_register_operand(output, COIL_TYPE_INT, 0x04, src1_reg);
  emit_register_operand(output, COIL_TYPE_INT, 0x04, src2_reg);
}

void emit_jmp(FILE *output, int label) {
  emit_instruction(output, OP_BR, 0x00, 0x01);
  emit_label_operand(output, label);
}

void emit_jcc(FILE *output, uint8_t condition, int label) {
  emit_instruction(output, OP_BRC, condition, 0x01);
  emit_label_operand(output, label);
}

void emit_call(FILE *output, const char *function_name) {
  emit_instruction(output, OP_CALL, 0x00, 0x01);
  emit_symbol_operand(output, function_name);
}

void emit_ret(FILE *output) {
  emit_instruction(output, OP_RET, 0x00, 0x00);
}

void emit_load(FILE *output, uint8_t dest_reg, uint8_t addr_reg, int offset) {
  emit_instruction(output, OP_LOAD, 0x00, 0x02);
  emit_register_operand(output, COIL_TYPE_INT, 0x04, dest_reg);
  emit_memory_operand(output, COIL_TYPE_INT, 0x04, addr_reg, offset);
}

void emit_store(FILE *output, uint8_t src_reg, uint8_t addr_reg, int offset) {
  emit_instruction(output, OP_STORE, 0x00, 0x02);
  emit_register_operand(output, COIL_TYPE_INT, 0x04, src_reg);
  emit_memory_operand(output, COIL_TYPE_INT, 0x04, addr_reg, offset);
}

void emit_push(FILE *output, uint8_t src_reg) {
  emit_instruction(output, OP_PUSH, 0x00, 0x01);
  emit_register_operand(output, COIL_TYPE_INT, 0x04, src_reg);
}

void emit_pop(FILE *output, uint8_t dest_reg) {
  emit_instruction(output, OP_POP, 0x00, 0x01);
  emit_register_operand(output, COIL_TYPE_INT, 0x04, dest_reg);
}

void emit_enter(FILE *output, int frame_size) {
  emit_instruction(output, OP_ENTER, 0x00, 0x01);
  emit_immediate_operand_i32(output, COIL_TYPE_INT, frame_size);
}

void emit_leave(FILE *output) {
  emit_instruction(output, OP_LEAVE, 0x00, 0x00);
}

void emit_label(FILE *output, int label) {
  emit_instruction(output, OP_SYMB, 0x00, 0x01);
  emit_label_operand(output, label);
}

/* Directive emission functions */
void emit_version_directive(FILE *output, uint8_t major, uint8_t minor, uint8_t patch) {
  write_u8(output, DIR_OPCODE_VERSION);
  write_u8(output, 0x00); // Reserved
  write_u16(output, 0x03); // Length (3 bytes)
  write_u8(output, major);
  write_u8(output, minor);
  write_u8(output, patch);
}

void emit_target_directive(FILE *output, uint16_t target_id) {
  write_u8(output, DIR_OPCODE_TARGET);
  write_u8(output, 0x00); // Reserved
  write_u16(output, 0x02); // Length (2 bytes)
  write_u16(output, target_id);
}

/* COF file structure functions */
void generate_cof_header(FILE *output, Program *program) {
  // Remember starting position
  // long start_pos = ftell(output);
  
  // Calculate offsets
  uint32_t header_offset = 0;
  uint32_t section_header_offset = COF_HEADER_SIZE;
  uint32_t code_section_offset = COF_HEADER_SIZE + COF_SECTION_HEADER_SIZE;
  
  // --- Main COF Header (32 bytes) ---
  fseek(output, header_offset, SEEK_SET);
  
  // Magic number: 'COIL'
  write_u32(output, COF_MAGIC);
  
  // Version: 1.0.0
  write_u8(output, 1);   // Major
  write_u8(output, 0);   // Minor
  write_u8(output, 0);   // Patch
  write_u8(output, COF_FLAG_EXECUTABLE); // Flags
  
  // Architecture target: x86-64
  write_u16(output, TARGET_X86_64);
  
  // Section count: 1 (just code for now)
  write_u16(output, 1);
  
  // Entrypoint: we'll update this after finding main()
  write_u32(output, 0);
  
  // String table: none for now
  write_u32(output, 0);  // Offset
  write_u32(output, 0);  // Size
  
  // Symbol table: none for now
  write_u32(output, 0);  // Offset
  write_u32(output, 0);  // Size
  
  // Padding to reach 32 bytes
  for (int i = 0; i < 8; i++) {
      write_u8(output, 0);
  }
  
  // --- Section Header (36 bytes) ---
  fseek(output, section_header_offset, SEEK_SET);
  
  // Section name: no name for now
  write_u32(output, 0);
  
  // Section type: CODE
  write_u32(output, COF_SECTION_CODE);
  
  // Section flags: EXEC | ALLOC
  write_u32(output, COF_SEC_FLAG_EXEC | COF_SEC_FLAG_ALLOC);
  
  // Section offset: starts right after section header
  write_u32(output, code_section_offset);
  
  // Section size: placeholder, we'll update this later
  write_u32(output, 0);
  
  // No link, info, or specific alignment requirements
  write_u32(output, 0);  // Link
  write_u32(output, 0);  // Info
  write_u32(output, 4);  // Alignment (4-byte)
  write_u32(output, 0);  // Entry size (not a table)
  
  // Move to the start of the code section
  fseek(output, code_section_offset, SEEK_SET);
  
  // Output the version directive
  emit_version_directive(output, 1, 0, 0);
  
  // Output the target directive
  emit_target_directive(output, TARGET_X86_64);
}

void update_cof_header(FILE *output, uint32_t entrypoint, uint32_t code_size) {
  // Update section size
  fseek(output, COF_HEADER_SIZE + 16, SEEK_SET); // Size field in section header
  write_u32(output, code_size);
  
  // Update entrypoint
  fseek(output, 16, SEEK_SET); // Entrypoint field in COF header
  write_u32(output, entrypoint);
}