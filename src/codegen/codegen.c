/**
 * Enhanced code generation for the COIL C Compiler
 * Ensures compliance with COIL 1.0.0 specification
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "cof.h"
#include "coil_constants.h"

/**
 * Initialize a code generator
 */
void generator_init(CodeGenerator *generator, FILE *output) {
    generator->output = output;
    generator->current_scope = symbol_table_create(NULL);
    generator->next_label = 0;
    generator->stack_offset = 0;
}

/**
 * Get the next label number for local jumps/branches
 */
int generator_next_label(CodeGenerator *generator) {
    return generator->next_label++;
}

/**
 * Allocate space on the stack for a variable
 * Returns the stack offset of the variable
 */
int allocate_local_variable(CodeGenerator *generator, Type *type) {
    // Calculate alignment - ensures variables are properly aligned
    int align = type->size >= 4 ? 4 : (type->size >= 2 ? 2 : 1);
    int aligned_offset = (generator->stack_offset + align - 1) & ~(align - 1);
    
    // Update stack offset
    generator->stack_offset = aligned_offset + type->size;
    
    // Return negative offset from base pointer
    return -aligned_offset;
}

/**
 * Emit code for function prologue
 * Sets up the stack frame and saves necessary registers
 */
void emit_function_prologue(CodeGenerator *generator, Function *function) {
    FILE *output = generator->output;
    
    // Define the function symbol
    emit_symbol_directive(output, SYM_BIND_GLOBAL, function->name, 0);
    
    // Setup stack frame with ENTER instruction
    // This saves old base pointer and sets up new stack frame
    emit_enter(output, 0); // Frame size will be determined later
    
    // Allocate space for parameters
    for (int i = 0; i < function->parameter_count; i++) {
        // Parameters are at positive offsets from the base pointer (already on stack)
        int param_size = function->parameter_types[i]->size;
        int aligned_size = (param_size + 3) & ~3; // Align to 4 bytes
        int offset = 8 + (i * aligned_size); // 8 bytes for return address and saved base pointer
        
        // Add parameter to symbol table
        symbol_table_add(generator->current_scope, function->parameter_names[i], 
                         function->parameter_types[i], offset);
    }
}

/**
 * Emit code for function epilogue
 * Cleans up the stack frame and returns
 */
void emit_function_epilogue(CodeGenerator *generator) {
    FILE *output = generator->output;
    
    // Restore stack pointer and base pointer
    emit_leave(output);
    
    // Return from function
    emit_ret(output);
}

/**
 * Generate COIL code for a binary operation
 */
int generate_binary_operation(CodeGenerator *generator, Expression *expr, int dest_reg) {
    FILE *output = generator->output;
    
    // Generate code for left and right operands
    int left_reg = generate_expression(generator, expr->value.binary.left, REG_RQ0);
    int right_reg = generate_expression(generator, expr->value.binary.right, REG_RQ1);
    
    // Perform the operation based on the operator
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
            // Compare the operands
            emit_cmp(output, left_reg, right_reg);
            
            // Create labels for the true and end branches
            int true_label = generator_next_label(generator);
            int end_label = generator_next_label(generator);
            
            // Determine the branch condition
            uint8_t condition;
            switch (expr->value.binary.operator) {
                case TOKEN_EQ:  condition = BR_EQ; break;
                case TOKEN_NEQ: condition = BR_NE; break;
                case TOKEN_LT:  condition = BR_LT; break;
                case TOKEN_LE:  condition = BR_LE; break;
                case TOKEN_GT:  condition = BR_GT; break;
                case TOKEN_GE:  condition = BR_GE; break;
                default: condition = BR_ALWAYS; break;
            }
            
            // Jump to true_label if condition is met
            emit_jcc(output, condition, true_label);
            
            // False case: load 0
            emit_movi(output, dest_reg, 0);
            emit_jmp(output, end_label);
            
            // True case: load 1
            emit_label(output, true_label);
            emit_movi(output, dest_reg, 1);
            
            // End label
            emit_label(output, end_label);
            break;
            
        default:
            fprintf(stderr, "Unsupported binary operator: %d\n", expr->value.binary.operator);
            exit(1);
    }
    
    return dest_reg;
}

/**
 * Generate COIL code for a function call
 */
int generate_function_call(CodeGenerator *generator, Expression *expr, int dest_reg) {
    FILE *output = generator->output;
    
    // Evaluate arguments in reverse order and push them onto the stack
    for (int i = expr->value.call.argument_count - 1; i >= 0; i--) {
        int arg_reg = generate_expression(generator, expr->value.call.arguments[i], REG_RQ0);
        emit_push(output, arg_reg);
    }
    
    // Call the function
    emit_call(output, expr->value.call.function_name);
    
    // Clean up the stack (caller cleanup)
    // Calculate total size of arguments
    int arg_size = 0;
    for (int i = 0; i < expr->value.call.argument_count; i++) {
        Type *arg_type = expr->value.call.arguments[i]->data_type;
        int aligned_size = (arg_type->size + 3) & ~3; // Align to 4 bytes
        arg_size += aligned_size;
    }
    
    // Adjust stack pointer if needed
    if (arg_size > 0) {
        emit_instruction(output, OP_ADJSP, 0x00, 0x01);
        emit_immediate_operand_i32(output, COIL_TYPE_INT, arg_size);
    }
    
    // Move result to destination register if needed
    if (dest_reg != REG_RQ0) {
        emit_mov(output, dest_reg, REG_RQ0);
    }
    
    return dest_reg;
}

/**
 * Generate COIL code for a variable reference
 */
int generate_variable_reference(CodeGenerator *generator, Expression *expr, int dest_reg) {
    FILE *output = generator->output;
    
    // Look up the variable in the symbol table
    Symbol *symbol = symbol_table_lookup(generator->current_scope, expr->value.variable_name);
    if (symbol == NULL) {
        fprintf(stderr, "Error: Undefined variable '%s'\n", expr->value.variable_name);
        exit(1);
    }
    
    // Load the variable's value
    emit_load(output, dest_reg, REG_RBP, symbol->offset);
    
    return dest_reg;
}

/**
 * Generate COIL code for a variable assignment
 */
int generate_variable_assignment(CodeGenerator *generator, Expression *expr, int dest_reg) {
    FILE *output = generator->output;
    
    // Look up the variable in the symbol table
    Symbol *symbol = symbol_table_lookup(generator->current_scope, expr->value.assign.variable_name);
    if (symbol == NULL) {
        fprintf(stderr, "Error: Undefined variable '%s'\n", expr->value.assign.variable_name);
        exit(1);
    }
    
    // Generate code for the right-hand side expression
    int value_reg = generate_expression(generator, expr->value.assign.value, dest_reg);
    
    // Store the value in the variable
    emit_store(output, value_reg, REG_RBP, symbol->offset);
    
    return value_reg;
}

/**
 * Generate COIL code for an expression
 */
int generate_expression(CodeGenerator *generator, Expression *expr, int dest_reg) {
    if (expr == NULL) {
        return dest_reg;
    }
    
    FILE *output = generator->output;
    
    switch (expr->type) {
        case EXPR_LITERAL_INT:
            emit_movi(output, dest_reg, expr->value.int_value);
            return dest_reg;
            
        case EXPR_LITERAL_FLOAT: {
            // For simplicity, we'll treat floats as integers in this version
            // A more complete implementation would use COIL's floating-point support
            int int_val = (int)expr->value.float_value;
            emit_movi(output, dest_reg, int_val);
            return dest_reg;
        }
            
        case EXPR_BINARY:
            return generate_binary_operation(generator, expr, dest_reg);
            
        case EXPR_VARIABLE:
            return generate_variable_reference(generator, expr, dest_reg);
            
        case EXPR_ASSIGN:
            return generate_variable_assignment(generator, expr, dest_reg);
            
        case EXPR_CALL:
            return generate_function_call(generator, expr, dest_reg);
            
        case EXPR_UNARY: {
            // Generate code for the operand
            int operand_reg = generate_expression(generator, expr->value.unary.operand, dest_reg);
            
            // Perform the unary operation
            switch (expr->value.unary.operator) {
                case TOKEN_MINUS:
                    // Negate the value (0 - value)
                    emit_movi(output, REG_RQ1, 0);
                    emit_sub(output, dest_reg, REG_RQ1, operand_reg);
                    break;
                    
                case TOKEN_NOT:
                    // Logical NOT (compare with 0)
                    emit_movi(output, REG_RQ1, 0);
                    emit_cmp(output, operand_reg, REG_RQ1);
                    
                    // Create labels for true (value is 0) and end
                    int true_label = generator_next_label(generator);
                    int end_label = generator_next_label(generator);
                    
                    emit_jcc(output, BR_EQ, true_label);
                    
                    // Value is not 0, so result is 0
                    emit_movi(output, dest_reg, 0);
                    emit_jmp(output, end_label);
                    
                    // Value is 0, so result is 1
                    emit_label(output, true_label);
                    emit_movi(output, dest_reg, 1);
                    
                    emit_label(output, end_label);
                    break;
                    
                case TOKEN_BIT_NOT:
                    // Bitwise NOT (XOR with -1)
                    emit_movi(output, REG_RQ1, -1);
                    emit_instruction(output, OP_XOR, 0x00, 0x03);
                    emit_register_operand(output, COIL_TYPE_INT, 0x04, dest_reg);
                    emit_register_operand(output, COIL_TYPE_INT, 0x04, operand_reg);
                    emit_register_operand(output, COIL_TYPE_INT, 0x04, REG_RQ1);
                    break;
                    
                default:
                    fprintf(stderr, "Unsupported unary operator: %d\n", expr->value.unary.operator);
                    exit(1);
            }
            
            return dest_reg;
        }
            
        default:
            fprintf(stderr, "Unsupported expression type: %d\n", expr->type);
            exit(1);
    }
    
    return dest_reg;
}

/**
 * Generate COIL code for a declaration statement
 */
void generate_declaration(CodeGenerator *generator, Statement *stmt) {
    FILE *output = generator->output;
    
    // Allocate space for the variable on the stack
    int offset = allocate_local_variable(generator, stmt->value.declaration.type);
    
    // Add the variable to the symbol table
    symbol_table_add(generator->current_scope, stmt->value.declaration.name, 
                     stmt->value.declaration.type, offset);
    
    // Initialize the variable if there's an initializer
    if (stmt->value.declaration.initializer != NULL) {
        // Generate code for the initializer
        int value_reg = generate_expression(generator, stmt->value.declaration.initializer, REG_RQ0);
        
        // Store the value in the variable
        emit_store(output, value_reg, REG_RBP, offset);
    }
}

/**
 * Generate COIL code for an if statement
 */
void generate_if_statement(CodeGenerator *generator, Statement *stmt) {
    FILE *output = generator->output;
    
    // Generate labels for the else and end branches
    int else_label = generator_next_label(generator);
    int end_label = generator_next_label(generator);
    
    // Generate code for the condition
    generate_expression(generator, stmt->value.if_statement.condition, REG_RQ0);
    
    // Compare the condition with 0
    emit_movi(output, REG_RQ1, 0);
    emit_cmp(output, REG_RQ0, REG_RQ1);
    
    // Jump to else_label if condition is false (equal to 0)
    emit_jcc(output, BR_EQ, else_label);
    
    // Generate code for the "then" branch
    generate_statement(generator, stmt->value.if_statement.then_branch);
    
    // Jump to end_label to skip the else branch
    emit_jmp(output, end_label);
    
    // Generate code for the "else" branch
    emit_label(output, else_label);
    if (stmt->value.if_statement.else_branch != NULL) {
        generate_statement(generator, stmt->value.if_statement.else_branch);
    }
    
    // End label
    emit_label(output, end_label);
}

/**
 * Generate COIL code for a while statement
 */
void generate_while_statement(CodeGenerator *generator, Statement *stmt) {
    FILE *output = generator->output;
    
    // Generate labels for the loop start and end
    int start_label = generator_next_label(generator);
    int end_label = generator_next_label(generator);
    
    // Start label
    emit_label(output, start_label);
    
    // Generate code for the condition
    generate_expression(generator, stmt->value.while_statement.condition, REG_RQ0);
    
    // Compare the condition with 0
    emit_movi(output, REG_RQ1, 0);
    emit_cmp(output, REG_RQ0, REG_RQ1);
    
    // Jump to end_label if condition is false (equal to 0)
    emit_jcc(output, BR_EQ, end_label);
    
    // Generate code for the loop body
    generate_statement(generator, stmt->value.while_statement.body);
    
    // Jump back to the start label
    emit_jmp(output, start_label);
    
    // End label
    emit_label(output, end_label);
}

/**
 * Generate COIL code for a for statement
 */
void generate_for_statement(CodeGenerator *generator, Statement *stmt) {
    FILE *output = generator->output;
    
    // Generate labels for the loop condition, increment, and end
    int cond_label = generator_next_label(generator);
    int incr_label = generator_next_label(generator);
    int end_label = generator_next_label(generator);
    
    // Generate code for the initializer if present
    if (stmt->value.for_statement.initializer != NULL) {
        generate_expression(generator, stmt->value.for_statement.initializer, REG_RQ0);
    }
    
    // Condition label
    emit_label(output, cond_label);
    
    // Generate code for the condition if present
    if (stmt->value.for_statement.condition != NULL) {
        generate_expression(generator, stmt->value.for_statement.condition, REG_RQ0);
        
        // Compare the condition with 0
        emit_movi(output, REG_RQ1, 0);
        emit_cmp(output, REG_RQ0, REG_RQ1);
        
        // Jump to end_label if condition is false (equal to 0)
        emit_jcc(output, BR_EQ, end_label);
    }
    
    // Generate code for the loop body
    generate_statement(generator, stmt->value.for_statement.body);
    
    // Increment label
    emit_label(output, incr_label);
    
    // Generate code for the increment if present
    if (stmt->value.for_statement.increment != NULL) {
        generate_expression(generator, stmt->value.for_statement.increment, REG_RQ0);
    }
    
    // Jump back to the condition label
    emit_jmp(output, cond_label);
    
    // End label
    emit_label(output, end_label);
}

/**
 * Generate COIL code for a return statement
 */
void generate_return_statement(CodeGenerator *generator, Statement *stmt) {
    // Generate code for the return value if present
    if (stmt->value.expression != NULL) {
        generate_expression(generator, stmt->value.expression, REG_RQ0);
    }
    
    // Clean up and return
    emit_function_epilogue(generator);
}

/**
 * Generate COIL code for a block statement
 */
void generate_block(CodeGenerator *generator, Statement *stmt) {
    // Create a new scope for the block
    SymbolTable *old_scope = generator->current_scope;
    generator->current_scope = symbol_table_create(old_scope);
    
    // Generate code for each statement in the block
    for (int i = 0; i < stmt->value.block.statement_count; i++) {
        generate_statement(generator, stmt->value.block.statements[i]);
    }
    
    // Restore the previous scope
    generator->current_scope = old_scope;
}

/**
 * Generate COIL code for a statement
 */
void generate_statement(CodeGenerator *generator, Statement *stmt) {
    if (stmt == NULL) {
        return;
    }
    
    switch (stmt->type) {
        case STMT_EXPRESSION:
            generate_expression(generator, stmt->value.expression, REG_RQ0);
            break;
            
        case STMT_DECLARATION:
            generate_declaration(generator, stmt);
            break;
            
        case STMT_IF:
            generate_if_statement(generator, stmt);
            break;
            
        case STMT_WHILE:
            generate_while_statement(generator, stmt);
            break;
            
        case STMT_FOR:
            generate_for_statement(generator, stmt);
            break;
            
        case STMT_RETURN:
            generate_return_statement(generator, stmt);
            break;
            
        case STMT_BLOCK:
            generate_block(generator, stmt);
            break;
            
        default:
            fprintf(stderr, "Unsupported statement type: %d\n", stmt->type);
            exit(1);
    }
}

/**
 * Find the function offset in the code section
 */
uint32_t find_function_offset(Function *function, Program *program) {
    // TODO: track the actual offsets of functions
    (void)function;
    (void)program;
    return COF_HEADER_SIZE + COF_SECTION_HEADER_SIZE + 16; // Skip directives
}

/**
 * Generate COIL code for a function
 */
void generate_function(CodeGenerator *generator, Function *function) {
    // Reset stack offset for new function
    generator->stack_offset = 0;
    
    // Create a new scope for the function
    SymbolTable *old_scope = generator->current_scope;
    generator->current_scope = symbol_table_create(old_scope);
    
    // Generate function prologue
    emit_function_prologue(generator, function);
    
    // Generate code for function body
    generate_statement(generator, function->body);
    
    // Add implicit return if the function doesn't already have one
    if (function->body->type == STMT_BLOCK && 
        (function->body->value.block.statement_count == 0 || 
         function->body->value.block.statements[function->body->value.block.statement_count - 1]->type != STMT_RETURN)) {
        emit_function_epilogue(generator);
    }
    
    // Restore the previous scope
    generator->current_scope = old_scope;
}

/**
 * Generate COIL code for the entire program
 */
void generate_program(Program *program, const char *output_file) {
    FILE *output = fopen(output_file, "wb");
    if (output == NULL) {
        fprintf(stderr, "Error: Failed to open output file '%s'\n", output_file);
        exit(1);
    }
    
    // Write initial COF header and section headers
    generate_cof_header(output);
    
    // Generate code for each function
    CodeGenerator generator;
    generator_init(&generator, output);
    
    uint32_t entrypoint = 0;
    
    // First pass: generate function symbols
    for (int i = 0; i < program->function_count; i++) {
        Function *function = program->functions[i];
        
        // Define the function symbol
        emit_symbol_directive(output, SYM_BIND_GLOBAL, function->name, 0);
        
        // Check if this is the main function
        if (strcmp(function->name, "main") == 0) {
            // In a real compiler, we'd track the actual offset
            entrypoint = find_function_offset(function, program);
        }
    }
    
    // Second pass: generate function code
    for (int i = 0; i < program->function_count; i++) {
        generate_function(&generator, program->functions[i]);
    }
    
    // Update the COF header with the final sizes and offsets
    update_cof_header(output, entrypoint);
    
    fclose(output);
    
    printf("Generated COIL binary: %s\n", output_file);
}