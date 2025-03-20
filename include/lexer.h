/**
 * @file lexer.h
 * @brief Lexical analyzer for C source code
 */

#ifndef LEXER_H
#define LEXER_H

#include "ast.h"
#include <stdio.h>

// Forward declaration for Arena
struct Arena;

/**
 * @brief Lexer state for tokenizing C source code
 */
typedef struct {
  const char* source;
  const char* filename;
  size_t source_length;
  size_t position;
  size_t line;
  size_t column;
  struct Arena* arena;
  
  // Current token
  Token current;
  
  // Error handling
  bool has_error;
  char* error_message;
} Lexer;

/**
 * @brief Initialize a lexer for the given source code
 * @param source The C source code to tokenize
 * @param filename The source filename (for error reporting)
 * @param arena Memory arena for allocations
 * @return New lexer instance
 */
Lexer* lexer_create(const char* source, const char* filename, struct Arena* arena);

/**
 * @brief Advance to the next token
 * @param lexer The lexer to advance
 * @return The next token
 */
Token lexer_next_token(Lexer* lexer);

/**
 * @brief Peek at the current token
 * @param lexer The lexer to peek at
 * @return The current token
 */
Token lexer_peek_token(Lexer* lexer);

/**
 * @brief Check if the current token is of the given type
 * @param lexer The lexer to check
 * @param type The token type to check for
 * @return True if the current token is of the given type
 */
bool lexer_check(Lexer* lexer, TokenType type);

/**
 * @brief Expect the current token to be of the given type and advance
 * @param lexer The lexer to check
 * @param type The token type to expect
 * @return True if the current token is of the expected type
 */
bool lexer_expect(Lexer* lexer, TokenType type);

/**
 * @brief Consume the current token if it's of the given type
 * @param lexer The lexer to consume from
 * @param type The token type to consume
 * @return True if a token was consumed
 */
bool lexer_consume(Lexer* lexer, TokenType type);

/**
 * @brief Get the current source location
 * @param lexer The lexer to get the location from
 * @return The current source location
 */
SourceLocation lexer_location(Lexer* lexer);

/**
 * @brief Get a descriptive error message if there was an error
 * @param lexer The lexer with the error
 * @return Error message or NULL if no error
 */
const char* lexer_error(Lexer* lexer);

#endif /* LEXER_H */