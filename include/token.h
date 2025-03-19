/**
* Token definitions for the COIL C Compiler
*/

#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
  // Basic tokens
  TOKEN_EOF,
  TOKEN_IDENTIFIER,
  TOKEN_NUMBER_INT,
  TOKEN_NUMBER_FLOAT,
  TOKEN_STRING,
  TOKEN_CHAR,
  
  // Keywords
  TOKEN_INT,
  TOKEN_CHAR_KW,
  TOKEN_FLOAT,
  TOKEN_DOUBLE,
  TOKEN_VOID,
  TOKEN_IF,
  TOKEN_ELSE,
  TOKEN_WHILE,
  TOKEN_FOR,
  TOKEN_RETURN,
  
  // Operators
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_MULTIPLY,
  TOKEN_DIVIDE,
  TOKEN_MODULO,
  TOKEN_ASSIGN,
  TOKEN_EQ,
  TOKEN_NEQ,
  TOKEN_LT,
  TOKEN_LE,
  TOKEN_GT,
  TOKEN_GE,
  TOKEN_AND,
  TOKEN_OR,
  TOKEN_NOT,
  TOKEN_BIT_AND,
  TOKEN_BIT_OR,
  TOKEN_BIT_XOR,
  TOKEN_BIT_NOT,
  TOKEN_BIT_SHL,
  TOKEN_BIT_SHR,
  
  // Punctuation
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_LBRACE,
  TOKEN_RBRACE,
  TOKEN_LBRACKET,
  TOKEN_RBRACKET,
  TOKEN_SEMICOLON,
  TOKEN_COMMA,
  TOKEN_DOT,
  TOKEN_ARROW,
  
  // Special
  TOKEN_UNKNOWN
} TokenType;

typedef struct {
  TokenType type;
  char *lexeme;
  int line;
  union {
    int int_value;
    float float_value;
    char char_value;
    char *str_value;
  } value;
} Token;

/**
* Free resources associated with a token
*/
void token_free(Token token);

#endif /* TOKEN_H */