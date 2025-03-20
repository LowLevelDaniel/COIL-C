/**
 * @file colc.h
 * @brief COIL C Compiler main interface
 */

#ifndef COLC_H
#define COLC_H

#include <stdbool.h>

/**
 * @brief Compilation options
 */
typedef struct {
  const char* input_file;
  const char* output_file;
  bool print_ast;
  bool print_tokens;
  bool optimize;
  int optimization_level;
  bool emit_debug_info;
  bool verbose;
} CompilerOptions;

/**
 * @brief Initialize default compiler options
 * @return Default compiler options
 */
CompilerOptions compiler_default_options();

/**
 * @brief Compile a C source file to COIL binary format
 * @param options Compiler options
 * @return True if compilation succeeded
 */
bool compiler_compile_file(CompilerOptions options);

/**
 * @brief Compile C source string to COIL binary format
 * @param source C source code
 * @param source_name Source name (for error reporting)
 * @param output_file Output filename for COIL binary
 * @param options Compiler options
 * @return True if compilation succeeded
 */
bool compiler_compile_string(const char* source, const char* source_name, 
                           const char* output_file, CompilerOptions options);

/**
 * @brief Get the last error message if compilation failed
 * @return Error message or NULL if no error
 */
const char* compiler_error();

/**
 * @brief Print compiler version and build information
 */
void compiler_print_version();

#endif /* COLC_H */