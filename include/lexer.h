/**
 * @file lexer.h
 * @brief Lexical analyzer for the COIL C Compiler
 *
 * Defines the lexer that converts C source code into a stream of tokens.
 */

#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>
#include "token.h"

/**
 * Lexer state for tokenizing C source code
 */
typedef struct {
    const char *source;  /**< Source code being processed */
    int position;        /**< Current position in source */
    int line;            /**< Current line number (1-based) */
    int column;          /**< Current column number (1-based) */
    char current;        /**< Current character */
} Lexer;

/**
 * Initialize a lexer with source code
 * @param lexer Pointer to the lexer to initialize
 * @param source Source code string
 */
void lexer_init(Lexer *lexer, const char *source);

/**
 * Get the next token from the source
 * @param lexer Pointer to the lexer
 * @return The next token
 */
Token lexer_next_token(Lexer *lexer);

/**
 * Peek at the next token without consuming it
 * @param lexer Pointer to the lexer
 * @return The next token (caller must free with token_free)
 */
Token lexer_peek_token(Lexer *lexer);

/**
 * Get the current source position as a string for error reporting
 * @param lexer Pointer to the lexer
 * @return A string like "line X, column Y" (must be freed by caller)
 */
char *lexer_position_string(Lexer *lexer);

#endif /* LEXER_H */