/**
 * @file token.c
 * @brief Token handling for the COIL C Compiler
 * @details Implements token creation, manipulation and conversion functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"

/**
 * @brief Create a token with the given type and lexeme
 *
 * @param type Token type
 * @param lexeme Token text (will be copied)
 * @param line Line number
 * @param column Column number
 * @return A new token
 */
Token token_create(TokenType type, const char *lexeme, int line, int column) {
  Token token;
  
  token.type = type;
  token.line = line;
  token.column = column;
  
  /* Copy the lexeme if provided */
  if (lexeme != NULL) {
    token.lexeme = strdup(lexeme);
    if (token.lexeme == NULL) {
      fprintf(stderr, "Memory allocation failed for token lexeme\n");
      exit(1);
    }
  } else {
    token.lexeme = NULL;
  }
  
  /* Initialize value fields to zero */
  token.value.int_value = 0;
  
  return token;
}

/* Note: token_free is already implemented in lexer.c */

/**
 * @brief Create a copy of a token
 *
 * @param token The token to copy
 * @return A new copy of the token
 */
Token token_copy(Token token) {
  Token copy;
  
  copy.type = token.type;
  copy.line = token.line;
  copy.column = token.column;
  
  /* Copy the lexeme if it exists */
  if (token.lexeme != NULL) {
    copy.lexeme = strdup(token.lexeme);
    if (copy.lexeme == NULL) {
      fprintf(stderr, "Memory allocation failed for token copy\n");
      exit(1);
    }
  } else {
    copy.lexeme = NULL;
  }
  
  /* Copy the appropriate value based on token type */
  switch (token.type) {
    case TOKEN_NUMBER_INT:
      copy.value.int_value = token.value.int_value;
      break;
    case TOKEN_NUMBER_FLOAT:
      copy.value.float_value = token.value.float_value;
      break;
    case TOKEN_CHAR:
      copy.value.char_value = token.value.char_value;
      break;
    case TOKEN_STRING:
      /* str_value points to lexeme, which was already copied */
      copy.value.str_value = copy.lexeme;
      break;
    default:
      /* For other token types, no special handling needed */
      break;
  }
  
  return copy;
}

/**
 * @brief Get a string representation of a token type
 *
 * @param type The token type
 * @return A string representation (must not be freed)
 */
const char *token_type_to_string(TokenType type) {
  switch (type) {
    case TOKEN_EOF:               return "EOF";
    
    /* Basic tokens */
    case TOKEN_IDENTIFIER:        return "IDENTIFIER";
    case TOKEN_NUMBER_INT:        return "NUMBER_INT";
    case TOKEN_NUMBER_FLOAT:      return "NUMBER_FLOAT";
    case TOKEN_STRING:            return "STRING";
    case TOKEN_CHAR:              return "CHAR";
    
    /* Keywords */
    case TOKEN_INT:               return "int";
    case TOKEN_CHAR_KW:           return "char";
    case TOKEN_FLOAT:             return "float";
    case TOKEN_DOUBLE:            return "double";
    case TOKEN_VOID:              return "void";
    case TOKEN_IF:                return "if";
    case TOKEN_ELSE:              return "else";
    case TOKEN_WHILE:             return "while";
    case TOKEN_FOR:               return "for";
    case TOKEN_RETURN:            return "return";
    case TOKEN_STRUCT:            return "struct";
    case TOKEN_UNION:             return "union";
    case TOKEN_TYPEDEF:           return "typedef";
    case TOKEN_ENUM:              return "enum";
    case TOKEN_SIZEOF:            return "sizeof";
    case TOKEN_BREAK:             return "break";
    case TOKEN_CONTINUE:          return "continue";
    case TOKEN_STATIC:            return "static";
    case TOKEN_EXTERN:            return "extern";
    case TOKEN_CONST:             return "const";
    case TOKEN_VOLATILE:          return "volatile";
    
    /* Operators */
    case TOKEN_PLUS:              return "+";
    case TOKEN_MINUS:             return "-";
    case TOKEN_MULTIPLY:          return "*";
    case TOKEN_DIVIDE:            return "/";
    case TOKEN_MODULO:            return "%";
    case TOKEN_ASSIGN:            return "=";
    case TOKEN_EQ:                return "==";
    case TOKEN_NEQ:               return "!=";
    case TOKEN_LT:                return "<";
    case TOKEN_LE:                return "<=";
    case TOKEN_GT:                return ">";
    case TOKEN_GE:                return ">=";
    case TOKEN_AND:               return "&&";
    case TOKEN_OR:                return "||";
    case TOKEN_NOT:               return "!";
    case TOKEN_BIT_AND:           return "&";
    case TOKEN_BIT_OR:            return "|";
    case TOKEN_BIT_XOR:           return "^";
    case TOKEN_BIT_NOT:           return "~";
    case TOKEN_BIT_SHL:           return "<<";
    case TOKEN_BIT_SHR:           return ">>";
    case TOKEN_INC:               return "++";
    case TOKEN_DEC:               return "--";
    case TOKEN_PLUS_ASSIGN:       return "+=";
    case TOKEN_MINUS_ASSIGN:      return "-=";
    case TOKEN_MUL_ASSIGN:        return "*=";
    case TOKEN_DIV_ASSIGN:        return "/=";
    case TOKEN_MOD_ASSIGN:        return "%=";
    case TOKEN_AND_ASSIGN:        return "&=";
    case TOKEN_OR_ASSIGN:         return "|=";
    case TOKEN_XOR_ASSIGN:        return "^=";
    case TOKEN_SHL_ASSIGN:        return "<<=";
    case TOKEN_SHR_ASSIGN:        return ">>=";
    
    /* Punctuation */
    case TOKEN_LPAREN:            return "(";
    case TOKEN_RPAREN:            return ")";
    case TOKEN_LBRACE:            return "{";
    case TOKEN_RBRACE:            return "}";
    case TOKEN_LBRACKET:          return "[";
    case TOKEN_RBRACKET:          return "]";
    case TOKEN_SEMICOLON:         return ";";
    case TOKEN_COMMA:             return ",";
    case TOKEN_DOT:               return ".";
    case TOKEN_ARROW:             return "->";
    case TOKEN_COLON:             return ":";
    case TOKEN_QUESTION:          return "?";
    
    /* Special */
    case TOKEN_COMMENT:           return "COMMENT";
    case TOKEN_UNKNOWN:           return "UNKNOWN";
    
    default:
      return "UNKNOWN_TOKEN_TYPE";
  }
}