/**
 * COIL Object Format (COF) handling
 * Based on the format described in obj.md
 */

#ifndef COF_H
#define COF_H

#include <stdio.h>
#include <stdint.h>
#include "ast.h"
#include "coil_constants.h"

/* COF file structure constants */
#define COF_MAGIC       0x434F494C  /* 'COIL' in ASCII */
#define COF_HEADER_SIZE 32
#define COF_SECTION_HEADER_SIZE 36

/* COF header flags */
#define COF_FLAG_EXECUTABLE   0x01  /* Contains an entrypoint */
#define COF_FLAG_LINKABLE     0x02  /* Contains external references */
#define COF_FLAG_POSITION_IND 0x04  /* Position-independent code */
#define COF_FLAG_CONTAINS_DBG 0x08  /* Contains debug information */

/* Section flags */
#define COF_SEC_FLAG_WRITE    0x01  /* Writable section */
#define COF_SEC_FLAG_EXEC     0x02  /* Executable section */
#define COF_SEC_FLAG_ALLOC    0x04  /* Section occupies memory during execution */

/**
 * Binary format utility functions
 */
void write_u8(FILE *output, uint8_t value);
void write_u16(FILE *output, uint16_t value);
void write_u32(FILE *output, uint32_t value);
void write_i32(FILE *output, int32_t value);
void write_u64(FILE *output, uint64_t value);
void write_f32(FILE *output, float value);
void write_f64(FILE *output, double value);
void write_string(FILE *output, const char *str);

/**
 * COIL instruction emission functions
 */
void emit_instruction(FILE *output, uint8_t opcode, uint8_t qualifier, uint8_t operand_count);
void emit_register_operand(FILE *output, uint8_t type, uint8_t width, uint8_t reg_num);
void emit_immediate_operand_i32(FILE *output, uint8_t type, int32_t value);
void emit_label_operand(FILE *output, int label);
void emit_symbol_operand(FILE *output, const char *name);
void emit_memory_operand(FILE *output, uint8_t type, uint8_t width, uint8_t base_reg, int32_t offset);

/**
 * High-level COIL instruction emitters
 */
void emit_nop(FILE *output);
void emit_mov(FILE *output, uint8_t dest_reg, uint8_t src_reg);
void emit_movi(FILE *output, uint8_t dest_reg, int32_t value);
void emit_add(FILE *output, uint8_t dest_reg, uint8_t src1_reg, uint8_t src2_reg);
void emit_sub(FILE *output, uint8_t dest_reg, uint8_t src1_reg, uint8_t src2_reg);
void emit_mul(FILE *output, uint8_t dest_reg, uint8_t src1_reg, uint8_t src2_reg);
void emit_div(FILE *output, uint8_t dest_reg, uint8_t src1_reg, uint8_t src2_reg);
void emit_cmp(FILE *output, uint8_t src1_reg, uint8_t src2_reg);
void emit_jmp(FILE *output, int label);
void emit_jcc(FILE *output, uint8_t condition, int label);
void emit_call(FILE *output, const char *function_name);
void emit_ret(FILE *output);
void emit_load(FILE *output, uint8_t dest_reg, uint8_t addr_reg, int offset);
void emit_store(FILE *output, uint8_t src_reg, uint8_t addr_reg, int offset);
void emit_push(FILE *output, uint8_t src_reg);
void emit_pop(FILE *output, uint8_t dest_reg);
void emit_enter(FILE *output, int frame_size);
void emit_leave(FILE *output);
void emit_label(FILE *output, int label);

/**
 * COIL directive emission functions
 */
void emit_version_directive(FILE *output, uint8_t major, uint8_t minor, uint8_t patch);
void emit_target_directive(FILE *output, uint16_t target_id);
void emit_section_directive(FILE *output, uint8_t section_type, const char *name, uint8_t flags);
void emit_symbol_directive(FILE *output, uint8_t qualifier, const char *name, uint64_t value);

/**
 * COF file structure functions
 */
void generate_cof_header(FILE *output, Program *program);
void update_cof_header(FILE *output, uint32_t entrypoint, uint32_t code_size);

#endif /* COF_H */