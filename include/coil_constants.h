/**
 * COIL instruction set and format constants based on the COIL 1.0.0 specification
 * Follows the ISA defined in isa.md
 */

#ifndef COIL_CONSTANTS_H
#define COIL_CONSTANTS_H

/* Magic number - 'COIL' in ASCII */
#define COF_MAGIC       0x434F494C  /* Byte sequence: 43 4F 49 4C */

/* COF file structure constants */
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

/* Opcode ranges */
#define OPCODE_RANGE_INSTRUCTION_MIN  0x00
#define OPCODE_RANGE_INSTRUCTION_MAX  0xCF
#define OPCODE_RANGE_DIRECTIVE_MIN    0xD0
#define OPCODE_RANGE_DIRECTIVE_MAX    0xDF
#define OPCODE_RANGE_EXTENSION_MIN    0xE0
#define OPCODE_RANGE_EXTENSION_MAX    0xFF

/* Instruction Opcodes - From COIL ISA */
/* Control Flow Operations */
#define OP_NOP    0x00  /* No operation */
#define OP_SYMB   0x01  /* Define a symbol */
#define OP_BR     0x02  /* Unconditional branch */
#define OP_BRC    0x03  /* Conditional branch */
#define OP_CALL   0x04  /* Call subroutine */
#define OP_RET    0x05  /* Return from subroutine */
#define OP_INT    0x06  /* Trigger interrupt */
#define OP_SYSC   0x09  /* System call */

/* Arithmetic Operations */
#define OP_ADD    0x10  /* Addition */
#define OP_SUB    0x11  /* Subtraction */
#define OP_MUL    0x12  /* Multiplication */
#define OP_DIV    0x13  /* Division */
#define OP_MOD    0x14  /* Modulus */
#define OP_NEG    0x15  /* Negation */
#define OP_INC    0x17  /* Increment */
#define OP_DEC    0x18  /* Decrement */

/* Bit Manipulation */
#define OP_AND    0x20  /* Bitwise AND */
#define OP_OR     0x21  /* Bitwise OR */
#define OP_XOR    0x22  /* Bitwise XOR */
#define OP_NOT    0x23  /* Bitwise NOT */
#define OP_SHL    0x24  /* Shift left */
#define OP_SHR    0x25  /* Shift right logical */
#define OP_SAR    0x26  /* Shift right arithmetic */

/* Comparison Operations */
#define OP_CMP    0x30  /* Compare values and set flags */
#define OP_TEST   0x31  /* Test bits */

/* Data Movement */
#define OP_MOV    0x40  /* Move data */
#define OP_LOAD   0x41  /* Load from memory */
#define OP_STORE  0x42  /* Store to memory */
#define OP_XCHG   0x43  /* Exchange data */
#define OP_LEA    0x44  /* Load effective address */
#define OP_MOVI   0x45  /* Move immediate value */
#define OP_MOVZ   0x46  /* Move with zero extension */
#define OP_MOVS   0x47  /* Move with sign extension */

/* Stack Operations */
#define OP_PUSH   0x50  /* Push onto stack */
#define OP_POP    0x51  /* Pop from stack */
#define OP_PUSHA  0x52  /* Push all registers */
#define OP_POPA   0x53  /* Pop all registers */
#define OP_PUSHF  0x54  /* Push flags */
#define OP_POPF   0x55  /* Pop flags */
#define OP_ADJSP  0x56  /* Adjust stack pointer */

/* Variable Operations */
#define OP_VARCR  0x60  /* Create variable */
#define OP_VARDL  0x61  /* Delete variable */
#define OP_VARSC  0x62  /* Create variable scope */
#define OP_VAREND 0x63  /* End variable scope */

/* Conversion Operations */
#define OP_TRUNC  0x70  /* Truncate value */
#define OP_ZEXT   0x71  /* Zero extend value */
#define OP_SEXT   0x72  /* Sign extend value */
#define OP_FTOI   0x73  /* Float to integer */
#define OP_ITOF   0x74  /* Integer to float */

/* Function Support */
#define OP_ENTER  0xC0  /* Function prologue */
#define OP_LEAVE  0xC1  /* Function epilogue */
#define OP_PARAM  0xC2  /* Function parameter */
#define OP_RESULT 0xC3  /* Function result */
#define OP_ALLOCA 0xC4  /* Allocate stack memory */

/* Directive Opcodes */
#define DIR_OPCODE_VERSION  0xD0  /* Version specification */
#define DIR_OPCODE_TARGET   0xD1  /* Target architecture */
#define DIR_OPCODE_SECTION  0xD2  /* Section definition */
#define DIR_OPCODE_SYMBOL   0xD3  /* Symbol definition */
#define DIR_OPCODE_ALIGN    0xD4  /* Alignment directive */
#define DIR_OPCODE_DATA     0xD5  /* Data definition */
#define DIR_OPCODE_ABI      0xD6  /* ABI definition */

/* Operand Qualifiers */
#define OPQUAL_IMM    0x01  /* Immediate constant */
#define OPQUAL_VAR    0x02  /* Variable reference */
#define OPQUAL_REG    0x03  /* Register reference */
#define OPQUAL_MEM    0x04  /* Memory address */
#define OPQUAL_LBL    0x05  /* Label reference */
#define OPQUAL_STR    0x06  /* String literal */
#define OPQUAL_SYM    0x07  /* Symbol reference */
#define OPQUAL_REL    0x08  /* Relative offset */

/* Branch Condition Qualifiers */
#define BR_ALWAYS  0x00  /* Always branch (unconditional) */
#define BR_EQ      0x01  /* Equal / Zero */
#define BR_NE      0x02  /* Not equal / Not zero */
#define BR_LT      0x03  /* Less than */
#define BR_LE      0x04  /* Less than or equal */
#define BR_GT      0x05  /* Greater than */
#define BR_GE      0x06  /* Greater than or equal */
#define BR_CARRY   0x07  /* Carry flag set */
#define BR_OFLOW   0x08  /* Overflow flag set */
#define BR_SIGN    0x09  /* Sign flag set */

/* Arithmetic Operation Qualifiers */
#define ARITH_DEFAULT     0x00  /* Default behavior */
#define ARITH_SIGNED      0x01  /* Signed operation */
#define ARITH_SAT         0x02  /* Saturating operation */
#define ARITH_TRAP        0x10  /* Trap on overflow */
#define ARITH_APPROX      0x20  /* Allow approximation */
#define ARITH_UNCHECKED   0x40  /* Assume no overflow */

/* Memory Operation Qualifiers */
#define MEM_DEFAULT      0x00  /* Default memory access */
#define MEM_VOLATILE     0x01  /* Volatile access */
#define MEM_ATOMIC       0x02  /* Atomic access */
#define MEM_ALIGNED      0x04  /* Aligned access */
#define MEM_UNALIGNED    0x08  /* Unaligned access allowed */
#define MEM_NONTEMPORAL  0x10  /* Non-temporal access hint */

/* Virtual Register System - Based on reg.md */
#define REG_RQ0  0  /* 64-bit general purpose register 0 */
#define REG_RQ1  1  /* 64-bit general purpose register 1 */
#define REG_RQ2  2  /* 64-bit general purpose register 2 */
#define REG_RQ3  3  /* 64-bit general purpose register 3 */
#define REG_RQ4  4  /* 64-bit general purpose register 4 */
#define REG_RQ5  5  /* 64-bit general purpose register 5 */
#define REG_RQ6  6  /* 64-bit general purpose register 6 */
#define REG_RQ7  7  /* 64-bit general purpose register 7 (base pointer) */
#define REG_RSP  6  /* Stack pointer (aliases RQ6) */
#define REG_RBP  7  /* Base pointer (aliases RQ7) */

/* Section types from obj.md */
#define COF_SECTION_NULL     0  /* Unused section */
#define COF_SECTION_CODE     1  /* Executable code */
#define COF_SECTION_DATA     2  /* Initialized data */
#define COF_SECTION_BSS      3  /* Uninitialized data */
#define COF_SECTION_SYMTAB   4  /* Symbol table */
#define COF_SECTION_STRTAB   5  /* String table */
#define COF_SECTION_RELA     6  /* Relocation entries with addends */
#define COF_SECTION_REL      7  /* Relocation entries without addends */
#define COF_SECTION_METADATA 8  /* Metadata */
#define COF_SECTION_COMMENT  9  /* Comment section */

/* Architecture targets from obj.md */
#define TARGET_ANY      0x0000  /* Generic, architecture-independent */
#define TARGET_X86      0x0001  /* x86 (32-bit) */
#define TARGET_X86_64   0x0002  /* x86-64 (64-bit) */
#define TARGET_ARM      0x0003  /* ARM (32-bit) */
#define TARGET_ARM64    0x0004  /* ARM64 (64-bit) */
#define TARGET_RISCV32  0x0005  /* RISC-V (32-bit) */
#define TARGET_RISCV64  0x0006  /* RISC-V (64-bit) */

/* Type System - from type.md */
#define COIL_TYPE_INT     0x00  /* Signed integer */
#define COIL_TYPE_UINT    0x01  /* Unsigned integer */
#define COIL_TYPE_FLOAT   0x02  /* Floating-point */
#define COIL_TYPE_VOID    0xF0  /* Void type */
#define COIL_TYPE_BOOL    0xF1  /* Boolean */
#define COIL_TYPE_LINT    0xF2  /* Largest native integer */
#define COIL_TYPE_FINT    0xF3  /* Fastest native integer */
#define COIL_TYPE_PTR     0xF4  /* Native pointer */

#endif /* COIL_CONSTANTS_H */