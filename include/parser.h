/**
* Parser for the COIL C Compiler
*/

#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
  Lexer *lexer;
  Token current_token;
  Token previous_token;
} Parser;

/**
* Initialize a parser with a lexer
*/
void parser_init(Parser *parser, Lexer *lexer);

/**
* Parse a complete C program
* @param lexer The lexer to read tokens from
* @return The parsed program AST
*/
Program *parse_program(Lexer *lexer);

/**
* Parse a single type specifier
* @param parser The parser
* @return The parsed type
*/
Type *parse_type(Parser *parser);

/**
* Parse an expression
* @param parser The parser
* @return The parsed expression
*/
static Expression *parse_expression(Parser *parser);

/**
* Parse a statement
* @param parser The parser
* @return The parsed statement
*/
static Statement *parse_statement(Parser *parser);

/**
* Parse a function definition
* @param parser The parser
* @return The parsed function
*/
static Function *parse_function(Parser *parser);

#endif /* PARSER_H */