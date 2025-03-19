/**
* Main code generation for the COIL C Compiler
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "cof.h"
#include "coil_constants.h"

void generator_init(CodeGenerator *generator, FILE *output) {
  generator->output = output;
  generator->current_scope = symbol_table_create(NULL);
  generator->next_label = 0;
  generator->stack_offset = 0;
}

int generator_next_label(CodeGenerator *generator) {
  return generator->next_label++;
}

void generate_function(CodeGenerator *generator, Function *function) {
  FILE *output = generator->output;
  
  // Reset stack offset for the new function
  generator->stack_offset = 0;
  
  // Create a new scope for the function
  SymbolTable *old_scope = generator->current_scope;
  generator->current_scope = symbol_table_create(old_scope);
  
  // Add parameters to the symbol table
  for (int i = 0; i < function->parameter_count; i++) {
      // Parameters are at positive offsets from the base pointer
      int offset = (i + 1) * 4; // Assuming 32-bit parameters
      symbol_table_add(generator->current_scope, function->parameter_names[i], 
                        function->parameter_types[i], offset);
  }
  
  // Begin function
  // Use the SYMB instruction to create a function symbol
  emit_instruction(output, OP_SYMB, 0x00, 0x01);
  emit_symbol_operand(output, function->name);
  
  // Setup stack frame
  emit_enter(output, 64); // Arbitrary frame size, should be calculated based on variables
  
  // Generate code for function body
  generate_statement(generator, function->body);
  
  // Add implicit return if the function doesn't already have one
  if (function->body->type == STMT_BLOCK && 
      (function->body->value.block.statement_count == 0 || 
        function->body->value.block.statements[function->body->value.block.statement_count - 1]->type != STMT_RETURN)) {
      emit_leave(output);
      emit_ret(output);
  }
  
  // Restore the old scope
  generator->current_scope = old_scope;
}

void generate_program(Program *program, const char *output_file) {
  FILE *output = fopen(output_file, "wb");
  if (output == NULL) {
      fprintf(stderr, "Failed to open output file: %s\n", output_file);
      exit(1);
  }
  
  // Write initial headers with placeholders
  generate_cof_header(output, program);
  
  // Remember where the code section starts
  long code_start_pos = ftell(output);
  
  // Generate code
  CodeGenerator generator;
  generator_init(&generator, output);
  
  // Generate code for each function
  for (int i = 0; i < program->function_count; i++) {
      generate_function(&generator, program->functions[i]);
  }
  
  // End position after all code
  long code_end_pos = ftell(output);
  uint32_t code_size = (uint32_t)(code_end_pos - code_start_pos);
  
  // Find entrypoint (main function)
  uint32_t entrypoint = 0;
  for (int i = 0; i < program->function_count; i++) {
      if (strcmp(program->functions[i]->name, "main") == 0) {
          // Main is at the code start + its offset in the generated code
          // For simplicity in this demo, we'll just use the start of the code section
          entrypoint = COF_HEADER_SIZE + COF_SECTION_HEADER_SIZE;
          break;
      }
  }
  
  // Update header with correct sizes and offsets
  update_cof_header(output, entrypoint, code_size);
  
  // Done
  fclose(output);
  
  printf("Generated COIL binary: %s (%lu bytes)\n", 
          output_file, (unsigned long)(code_end_pos));
  printf("- Header size: %d bytes\n", COF_HEADER_SIZE);
  printf("- Section header size: %d bytes\n", COF_SECTION_HEADER_SIZE);
  printf("- Code size: %u bytes\n", code_size);
  printf("- Entrypoint: 0x%08X\n", entrypoint);
}