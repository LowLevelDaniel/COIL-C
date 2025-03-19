/**
 * @file coil_constants.h
 * @brief COIL instruction set and format constants
 *
 * Defines constants from the COIL 1.0.0 specification including opcodes,
 * types, and binary format definitions.
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
#define COF_SEC_FLAG_MERGE    0x08  /* Section can be merged */
#define COF_SEC_FLAG_STRINGS  0x10  /* Section contains null-terminated strings */

/* Symbol types */
#define SYM_TYPE_NOTYPE  0x00  /* No type specified */
#define SYM_TYPE_OBJECT  0x01  /* Data object (variable, array, etc.) */
#define SYM_TYPE_FUNC    0x02  /* Function or executable code */
#define SYM_TYPE_SECTION 0x03  /* Section symbol */
#define SYM_TYPE_FILE    0x04  /* Source file name */

/* Symbol bindings */
#define SYM_BIND_LOCAL  0x00  /* Local symbol, not visible outside object file */
#define SYM_BIND_GLOBAL 0x01  /* Global symbol, visible to all object files */
#define SYM_BIND_WEAK   0x02  /* Global scope but with lower precedence */
#define SYM_BIND_EXTERN 0x03  /* External symbol reference */

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
#define OP_IRET   0x07  /* Return from interrupt */
#define OP_WFI    0x08  /* Wait for interrupt */
#define OP_SYSC   0x09  /* System call */
#define OP_WFE    0x0A  /* Wait for event */
#define OP_SEV    0x0B  /* Send event */
#define OP_TRAP   0x0C  /* Software trap */
#define OP_HLT    0x0D  /* Halt execution */

/* Arithmetic Operations */
#define OP_ADD    0x10  /* Addition */
#define OP_SUB    0x11  /* Subtraction */
#define OP_MUL    0x12  /* Multiplication */
#define OP_DIV    0x13  /* Division */
#define OP_MOD    0x14  /* Modulus */
#define OP_NEG    0x15  /* Negation */
#define OP_ABS    0x16  /* Absolute value */
#define OP_INC    0x17  /* Increment */
#define OP_DEC    0x18  /* Decrement */
#define OP_ADDC   0x19  /* Add with carry */
#define OP_SUBB   0x1A  /* Subtract with borrow */
#define OP_MULH   0x1B  /* Multiplication high bits */

/* Bit Manipulation */
#define OP_AND    0x20  /* Bitwise AND */
#define OP_OR     0x21  /* Bitwise OR */
#define OP_XOR    0x22  /* Bitwise XOR */
#define OP_NOT    0x23  /* Bitwise NOT */
#define OP_SHL    0x24  /* Shift left */
#define OP_SHR    0x25  /* Shift right logical */
#define OP_SAR    0x26  /* Shift right arithmetic */
#define OP_ROL    0x27  /* Rotate left */
#define OP_ROR    0x28  /* Rotate right */
#define OP_CLZ    0x29  /* Count leading zeros */
#define OP_CTZ    0x2A  /* Count trailing zeros */
#define OP_POPC   0x2B  /* Population count */
#define OP_BSWP   0x2C  /* Byte swap */
#define OP_BEXT   0x2D  /* Bit extraction */
#define OP_BINS   0x2E  /* Bit insertion */
#define OP_BMSK   0x2F  /* Bit mask */

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
#define OP_LDMUL  0x48  /* Load multiple */
#define OP_STMUL  0x49  /* Store multiple */
#define OP_LDSTR  0x4A  /* Load string */
#define OP_STSTR  0x4B  /* Store string */

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
#define OP_VARGET 0x64  /* Get variable value */
#define OP_VARSET 0x65  /* Set variable value */
#define OP_VARREF 0x66  /* Get variable reference */

/* Conversion Operations */
#define OP_TRUNC  0x70  /* Truncate value */
#define OP_ZEXT   0x71  /* Zero extend value */
#define OP_SEXT   0x72  /* Sign extend value */
#define OP_FTOI   0x73  /* Float to integer */
#define OP_ITOF   0x74  /* Integer to float */
#define OP_FTOB   0x75  /* Float to bits */
#define OP_BTOF   0x76  /* Bits to float */
#define OP_F32F64 0x77  /* Float32 to Float64 */
#define OP_F64F32 0x78  /* Float64 to Float32 */

/* Atomic Operations */
#define OP_ATOMLD 0x80  /* Atomic load */
#define OP_ATOMST 0x81  /* Atomic store */
#define OP_ATOMXC 0x82  /* Atomic exchange */
#define OP_ATOMCMP 0x83  /* Atomic compare-exchange */
#define OP_ATOMADD 0x84  /* Atomic add */
#define OP_ATOMSUB 0x85  /* Atomic subtract */
#define OP_ATOMAND 0x86  /* Atomic AND */
#define OP_ATOMOR  0x87  /* Atomic OR */
#define OP_ATOMXOR 0x88  /* Atomic XOR */
#define OP_ATOMMAX 0x89  /* Atomic maximum */
#define OP_ATOMMIN 0x8A  /* Atomic minimum */
#define OP_FENCE   0x8B  /* Memory fence */
#define OP_BARRIER 0x8C  /* Memory barrier */

/* Special Operations */
#define OP_SQRT   0x90  /* Square root */
#define OP_FMA    0x91  /* Fused multiply-add */
#define OP_CEIL   0x92  /* Ceiling */
#define OP_FLOOR  0x93  /* Floor */
#define OP_ROUND  0x94  /* Round */

/* Conditional Operations */
#define OP_SELECT 0xA0  /* Conditional select */
#define OP_CMOV   0xA1  /* Conditional move */

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
#define DIR_OPCODE_FEATURE  0xD7  /* Feature control */
#define DIR_OPCODE_OPTIMIZE 0xD8  /* Optimization control */

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
#define BR_PARITY  0x0A  /* Parity flag set */
#define BR_NCARRY  0x0B  /* Carry flag not set */
#define BR_NOFLOW  0x0C  /* Overflow flag not set */
#define BR_NSIGN   0x0D  /* Sign flag not set */
#define BR_NPARITY 0x0E  /* Parity flag not set */

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
#define MEM_READONLY     0x20  /* Read-only memory hint */
#define MEM_WRITEONLY    0x40  /* Write-only memory hint */

/* Virtual Register System - Based on reg.md */
#define REG_RQ0  0  /* 64-bit general purpose register 0 */
#define REG_RQ1  1  /* 64-bit general purpose register 1 */
#define REG_RQ2  2  /* 64-bit general purpose register 2 */
#define REG_RQ3  3  /* 64-bit general purpose register 3 */
#define REG_RQ4  4  /* 64-bit general purpose register 4 */
#define REG_RQ5  5  /* 64-bit general purpose register 5 */
#define REG_RQ6  6  /* 64-bit general purpose register 6 */
#define REG_RQ7  7  /* 64-bit general purpose register 7 */
#define REG_RQ8  8  /* 64-bit general purpose register 8 */
#define REG_RQ9  9  /* 64-bit general purpose register 9 */
#define REG_RQ10 10 /* 64-bit general purpose register 10 */
#define REG_RQ11 11 /* 64-bit general purpose register 11 */
#define REG_RQ12 12 /* 64-bit general purpose register 12 */
#define REG_RQ13 13 /* 64-bit general purpose register 13 */
#define REG_RQ14 14 /* 64-bit general purpose register 14 */
#define REG_RQ15 15 /* 64-bit general purpose register 15 */

/* Special register aliases */
#define REG_RSP  6  /* Stack pointer (aliases RQ6) */
#define REG_RBP  7  /* Base pointer (aliases RQ7) */
#define REG_RF   16 /* Flags register */
#define REG_RIP  17 /* Instruction pointer register */

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
#define COF_SECTION_DIRECTIVE 10 /* Directives for the assembler */

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