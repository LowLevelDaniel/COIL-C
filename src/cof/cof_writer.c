/**
 * Enhanced COIL Object Format (COF) writer
 * Fully compliant with COIL 1.0.0 specification from obj.md
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "cof.h"

/* String table for symbol names and other strings */
typedef struct {
    char *data;
    size_t size;
    size_t capacity;
} StringTable;

static StringTable str_table = {NULL, 0, 0};
static uint32_t code_section_size = 0;
static uint32_t symbol_count = 0;
static long symbol_table_offset = 0;
static long string_table_offset = 0;

/**
 * Initialize the string table
 */
void string_table_init() {
    str_table.capacity = 256;
    str_table.data = malloc(str_table.capacity);
    str_table.size = 1; // First byte is always null
    str_table.data[0] = '\0';
}

/**
 * Add a string to the string table and return its offset
 */
uint32_t string_table_add(const char *str) {
    if (str_table.data == NULL) {
        string_table_init();
    }
    
    // Check if string already exists in table
    for (size_t i = 0; i < str_table.size; i++) {
        if (strcmp(str_table.data + i, str) == 0) {
            return (uint32_t)i;
        }
        // Skip to next string
        while (i < str_table.size && str_table.data[i] != '\0') {
            i++;
        }
    }
    
    // Add the string to the table
    size_t str_len = strlen(str) + 1; // Include null terminator
    
    // Ensure we have enough space
    if (str_table.size + str_len > str_table.capacity) {
        str_table.capacity = (str_table.capacity + str_len) * 2;
        str_table.data = realloc(str_table.data, str_table.capacity);
    }
    
    uint32_t offset = (uint32_t)str_table.size;
    memcpy(str_table.data + str_table.size, str, str_len);
    str_table.size += str_len;
    
    return offset;
}

/**
 * Write the string table to the output file
 */
void string_table_write(FILE *output) {
    if (str_table.data == NULL) {
        return;
    }
    
    // Remember the string table position
    string_table_offset = ftell(output);
    
    // Write the string table section header
    // Name offset - index 0 (empty string) or use .strtab
    write_u32(output, string_table_add(".strtab"));
    // Section type - COF_SECTION_STRTAB (5)
    write_u32(output, COF_SECTION_STRTAB);
    // Flags - none (0)
    write_u32(output, 0);
    // Offset - immediately follows header
    write_u32(output, string_table_offset + COF_SECTION_HEADER_SIZE);
    // Size - size of the string table
    write_u32(output, (uint32_t)str_table.size);
    // Link, Info, Alignment, Entsize - all 0
    write_u32(output, 0);
    write_u32(output, 0);
    write_u32(output, 1); // Alignment: 1 byte
    write_u32(output, 0);
    
    // Write the string table data
    fwrite(str_table.data, 1, str_table.size, output);
}

/**
 * Free the string table
 */
void string_table_free() {
    if (str_table.data != NULL) {
        free(str_table.data);
        str_table.data = NULL;
        str_table.size = 0;
        str_table.capacity = 0;
    }
}

/**
 * Binary format utility functions that explicitly handle endianness
 * to ensure consistent output regardless of host platform
 */

void write_u8(FILE *output, uint8_t value) {
    fwrite(&value, sizeof(uint8_t), 1, output);
}

void write_u16(FILE *output, uint16_t value) {
    // Write in little-endian format to match x86 native format
    uint8_t bytes[2];
    bytes[0] = value & 0xFF;
    bytes[1] = (value >> 8) & 0xFF;
    fwrite(bytes, 1, 2, output);
}

void write_u32(FILE *output, uint32_t value) {
    // Write in little-endian format to match x86 native format
    uint8_t bytes[4];
    bytes[0] = value & 0xFF;
    bytes[1] = (value >> 8) & 0xFF;
    bytes[2] = (value >> 16) & 0xFF;
    bytes[3] = (value >> 24) & 0xFF;
    fwrite(bytes, 1, 4, output);
}

void write_i32(FILE *output, int32_t value) {
    write_u32(output, (uint32_t)value);
}

void write_u64(FILE *output, uint64_t value) {
    // Write in little-endian format to match x86 native format
    uint8_t bytes[8];
    for (int i = 0; i < 8; i++) {
        bytes[i] = (value >> (i * 8)) & 0xFF;
    }
    fwrite(bytes, 1, 8, output);
}

void write_f32(FILE *output, float value) {
    union {
        float f;
        uint32_t u;
    } converter;
    converter.f = value;
    write_u32(output, converter.u);
}

void write_f64(FILE *output, double value) {
    union {
        double d;
        uint64_t u;
    } converter;
    converter.d = value;
    write_u64(output, converter.u);
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
    code_section_size += 3;
}

void emit_register_operand(FILE *output, uint8_t type, uint8_t width, uint8_t reg_num) {
    write_u8(output, OPQUAL_REG);
    write_u8(output, type);
    write_u8(output, width);
    write_u8(output, reg_num);
    code_section_size += 4;
}

void emit_immediate_operand_i32(FILE *output, uint8_t type, int32_t value) {
    write_u8(output, OPQUAL_IMM);
    write_u8(output, type);
    write_u8(output, 0x04); // 32-bit width
    write_i32(output, value);
    code_section_size += 7;
}

void emit_label_operand(FILE *output, int label) {
    write_u8(output, OPQUAL_LBL);
    write_u8(output, COIL_TYPE_VOID);
    write_u8(output, 0x00); // No additional type info
    write_i32(output, label);
    code_section_size += 7;
}

void emit_symbol_operand(FILE *output, const char *name) {
    uint32_t name_offset = string_table_add(name);
    write_u8(output, OPQUAL_SYM);
    write_u8(output, COIL_TYPE_VOID);
    write_u8(output, 0x00); // No additional type info
    write_u32(output, name_offset);
    code_section_size += 7;
}

void emit_memory_operand(FILE *output, uint8_t type, uint8_t width, uint8_t base_reg, int32_t offset) {
    write_u8(output, OPQUAL_MEM);
    write_u8(output, type);
    write_u8(output, width);
    write_u8(output, base_reg);
    write_i32(output, offset);
    code_section_size += 8;
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

/* Symbol table functions */
void emit_symbol_entry(FILE *output, const char *name, uint32_t value, uint8_t type, uint8_t binding) {
    uint32_t name_offset = string_table_add(name);
    write_u32(output, name_offset);
    write_u32(output, value);
    write_u32(output, 0); // Size - we don't know this yet
    write_u8(output, type);
    write_u8(output, binding);
    write_u8(output, 0); // Visibility - default
    write_u8(output, 1); // Section index - 1 for code section
    symbol_count++;
}

/* Directive emission functions */
void emit_version_directive(FILE *output, uint8_t major, uint8_t minor, uint8_t patch) {
    write_u8(output, DIR_OPCODE_VERSION);
    write_u8(output, 0x00); // Reserved
    write_u16(output, 0x03); // Length (3 bytes)
    write_u8(output, major);
    write_u8(output, minor);
    write_u8(output, patch);
    code_section_size += 7;
}

void emit_target_directive(FILE *output, uint16_t target_id) {
    write_u8(output, DIR_OPCODE_TARGET);
    write_u8(output, 0x00); // Reserved
    write_u16(output, 0x02); // Length (2 bytes)
    write_u16(output, target_id);
    code_section_size += 6;
}

void emit_section_directive(FILE *output, uint8_t section_type, const char *name, uint8_t flags) {
    uint32_t name_offset = string_table_add(name);
    write_u8(output, DIR_OPCODE_SECTION);
    write_u8(output, section_type);
    
    // Calculate length of name + flags
    uint16_t length = 4 + 1; // name_offset (4 bytes) + flags (1 byte)
    write_u16(output, length);
    
    // Write name offset and flags
    write_u32(output, name_offset);
    write_u8(output, flags);
    
    code_section_size += 8 + length;
}

void emit_symbol_directive(FILE *output, uint8_t qualifier, const char *name, uint64_t value) {
    uint32_t name_offset = string_table_add(name);
    write_u8(output, DIR_OPCODE_SYMBOL);
    write_u8(output, qualifier);
    
    // Calculate length - name offset (4 bytes) + value (8 bytes)
    uint16_t length = 12;
    write_u16(output, length);
    
    write_u32(output, name_offset);
    write_u64(output, value);
    
    code_section_size += 4 + length;
}

/* Symbol Table Functions */
void write_symbol_table(FILE *output) {
    // Remember the symbol table position
    symbol_table_offset = ftell(output);
    
    // Write the symbol table section header
    write_u32(output, string_table_add(".symtab"));
    write_u32(output, COF_SECTION_SYMTAB);
    write_u32(output, 0); // Flags
    write_u32(output, symbol_table_offset + COF_SECTION_HEADER_SIZE);
    write_u32(output, symbol_count * 16); // Size = num_symbols * entry_size
    write_u32(output, 3); // Link to string table section (3)
    write_u32(output, 0); // Info
    write_u32(output, 4); // Alignment: 4 bytes
    write_u32(output, 16); // Entry size: 16 bytes
    
    // Symbol table entries would go here, but they're already written
    // during the code generation process
}

/* COF file structure functions */
void generate_cof_header(FILE *output) {
    // Initialize the string table
    string_table_init();
    
    // Calculate offsets
    uint32_t header_offset = 0;
    uint32_t section_header_offset = COF_HEADER_SIZE;
    uint32_t code_section_offset = COF_HEADER_SIZE + COF_SECTION_HEADER_SIZE;
    
    // --- Main COF Header (32 bytes) ---
    // Follows struct cof_header in obj.md
    fseek(output, header_offset, SEEK_SET);
    
    // Magic number: 'COIL' - correctly ordering bytes for spec compliance
    // ASCII 'C'=0x43, 'O'=0x4F, 'I'=0x49, 'L'=0x4C
    // We'll write this explicitly to ensure correct byte order
    uint8_t magic_bytes[4] = {0x43, 0x4F, 0x49, 0x4C};
    fwrite(magic_bytes, 1, 4, output);
    
    // Version: 1.0.0
    write_u8(output, 1);   // Major
    write_u8(output, 0);   // Minor
    write_u8(output, 0);   // Patch
    write_u8(output, COF_FLAG_EXECUTABLE | COF_FLAG_LINKABLE); // Flags
    
    // Architecture target: x86-64
    write_u16(output, TARGET_X86_64);
    
    // Section count: Placeholder - we'll update this later
    write_u16(output, 0);
    
    // Entrypoint: Placeholder - we'll update this after finding main()
    write_u32(output, 0);
    
    // String table: Placeholder - we'll update this later
    write_u32(output, 0);  // Offset - will be updated later
    write_u32(output, 0);  // Size - will be updated later
    
    // Symbol table: Placeholder - we'll update this later
    write_u32(output, 0);  // Offset - will be updated later
    write_u32(output, 0);  // Size - will be updated later
    
    // Padding to reach 32 bytes - exactly 8 bytes as per spec
    for (int i = 0; i < 8; i++) {
        write_u8(output, 0);
    }
    
    // --- Section Header (36 bytes) for CODE section ---
    // Follows struct cof_section_header in obj.md
    fseek(output, section_header_offset, SEEK_SET);
    
    // Section name: .text
    write_u32(output, string_table_add(".text"));
    
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
    
    // Reset the code section size counter
    code_section_size = 0;
    
    // Output the version directive
    emit_version_directive(output, 1, 0, 0);
    
    // Output the target directive
    emit_target_directive(output, TARGET_X86_64);
}

void update_cof_header(FILE *output, uint32_t entrypoint) {
    // Write the string table
    string_table_write(output);
    
    // Calculate how many sections we have (code, string table, possibly symbol table)
    uint16_t section_count = 2; // Code + String Table
    if (symbol_count > 0) {
        section_count++;
        write_symbol_table(output);
    }
    
    // Update section size in code section header
    fseek(output, COF_HEADER_SIZE + 16, SEEK_SET); // Size field in section header
    write_u32(output, code_section_size);
    
    // Update COF header fields
    fseek(output, 8, SEEK_SET); // Section count field in COF header
    write_u16(output, section_count);
    
    // Update entrypoint
    fseek(output, 12, SEEK_SET); // Entrypoint field in COF header
    write_u32(output, entrypoint);
    
    // Update string table info
    fseek(output, 16, SEEK_SET); // String table info in COF header
    write_u32(output, string_table_offset);
    write_u32(output, (uint32_t)str_table.size);
    
    // Update symbol table info if we have symbols
    if (symbol_count > 0) {
        fseek(output, 24, SEEK_SET); // Symbol table info in COF header
        write_u32(output, symbol_table_offset);
        write_u32(output, symbol_count * 16); // Each symbol entry is 16 bytes
    }
    
    // Clean up
    string_table_free();
}