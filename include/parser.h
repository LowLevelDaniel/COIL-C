/**
 * @file parser.h
 * @brief Parser for C source code
 */

#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"

// Forward declaration for Arena
struct Arena;

/**
 * @brief Parser state for parsing C source code
 */
typedef struct {
  Lexer* lexer;
  struct Arena* arena;
  Program* program;
  
  // Symbol table for typedefs
  struct SymbolTable* typedefs;
  
  // Error handling
  bool has_error;
  char* error_message;
} Parser;

/**
 * @brief Initialize a parser for the given lexer
 * @param lexer The lexer to parse tokens from
 * @param arena Memory arena for allocations
 * @return New parser instance
 */
Parser* parser_create(Lexer* lexer, struct Arena* arena);

/**
 * @brief Parse a complete C program
 * @param parser The parser to use
 * @return The parsed program AST
 */
Program* parser_parse_program(Parser* parser);

/**
 * @brief Parse a single declaration
 * @param parser The parser to use
 * @return The parsed declaration AST node
 */
Decl* parser_parse_declaration(Parser* parser);

/**
 * @brief Parse a statement
 * @param parser The parser to use
 * @return The parsed statement AST node
 */
Stmt* parser_parse_statement(Parser* parser);

/**
 * @brief Parse an expression
 * @param parser The parser to use
 * @return The parsed expression AST node
 */
Expr* parser_parse_expression(Parser* parser);

/**
 * @brief Parse a type
 * @param parser The parser to use
 * @return The parsed type AST node
 */
Type* parser_parse_type(Parser* parser);

/**
 * @brief Get a descriptive error message if there was an error
 * @param parser The parser with the error
 * @return Error message or NULL if no error
 */
const char* parser_error(Parser* parser);

#endif /* PARSER_H */