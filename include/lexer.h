/**
* Lexical analyzer for the COIL C Compiler
*/

#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>
#include "token.h"

typedef struct {
  const char *source;
  int position;
  int line;
  char current;
} Lexer;

/**
* Initialize a lexer with source code
*/
void lexer_init(Lexer *lexer, const char *source);

/**
* Get the next token from the source
*/
Token lexer_next_token(Lexer *lexer);

#endif /* LEXER_H */