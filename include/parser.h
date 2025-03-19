/**
 * @file parser.h
 * @brief Parser for the COIL C Compiler
 *
 * Defines the parser that converts tokens into an Abstract Syntax Tree (AST).
 */

#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

/**
 * Parser state for building an AST from tokens
 */
typedef struct {
    Lexer *lexer;            /**< Lexer for getting tokens */
    Token current_token;     /**< Current token being processed */
    Token previous_token;    /**< Previous token (for error reporting) */
    bool had_error;          /**< Whether an error occurred during parsing */
    char *error_message;     /**< Error message if an error occurred */
} Parser;

/**
 * Initialize a parser with a lexer
 * @param parser Pointer to the parser to initialize
 * @param lexer Lexer to use for getting tokens
 */
void parser_init(Parser *parser, Lexer *lexer);

/**
 * Parse a complete C program
 * @param lexer The lexer to read tokens from
 * @return The parsed program AST, or NULL if parsing failed
 */
Program *parse_program(Lexer *lexer);

/**
 * Parse a single type specifier
 * @param parser The parser
 * @return The parsed type
 */
Type *parse_type(Parser *parser);

/**
 * Free parser resources
 * @param parser The parser to free
 */
void parser_free(Parser *parser);

/**
 * Check if the parser encountered an error
 * @param parser The parser
 * @return true if an error occurred, false otherwise
 */
bool parser_had_error(Parser *parser);

/**
 * Get the parser's error message
 * @param parser The parser
 * @return The error message or NULL if no error occurred
 */
const char *parser_get_error(Parser *parser);

/**
 * Create a formatted error message
 * @param parser The parser
 * @param format Format string (printf style)
 * @param ... Additional arguments for format string
 */
void parser_error(Parser *parser, const char *format, ...);

#endif /* PARSER_H */