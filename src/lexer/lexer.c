/**
* Lexical analyzer implementation for the COIL C Compiler
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

void lexer_init(Lexer *lexer, const char *source) {
  lexer->source = source;
  lexer->position = 0;
  lexer->line = 1;
  lexer->current = source[0];
}

static void lexer_advance(Lexer *lexer) {
  if (lexer->current != '\0') {
      lexer->position++;
      lexer->current = lexer->source[lexer->position];
  }
}

static void lexer_skip_whitespace(Lexer *lexer) {
  while (isspace(lexer->current)) {
      if (lexer->current == '\n') {
          lexer->line++;
      }
      lexer_advance(lexer);
  }
}

static void lexer_skip_comment(Lexer *lexer) {
  // Skip single-line comment
  if (lexer->current == '/' && lexer->source[lexer->position + 1] == '/') {
      while (lexer->current != '\n' && lexer->current != '\0') {
          lexer_advance(lexer);
      }
  }
  // Skip multi-line comment
  else if (lexer->current == '/' && lexer->source[lexer->position + 1] == '*') {
      lexer_advance(lexer); // Skip '/'
      lexer_advance(lexer); // Skip '*'
      while (!(lexer->current == '*' && lexer->source[lexer->position + 1] == '/') && 
              lexer->current != '\0') {
          if (lexer->current == '\n') {
              lexer->line++;
          }
          lexer_advance(lexer);
      }
      if (lexer->current != '\0') {
          lexer_advance(lexer); // Skip '*'
          lexer_advance(lexer); // Skip '/'
      }
  }
}

static bool is_keyword(const char *str) {
  static const char *keywords[] = {
      "int", "char", "float", "double", "void",
      "if", "else", "while", "for", "return",
      NULL
  };
  
  for (int i = 0; keywords[i] != NULL; i++) {
      if (strcmp(str, keywords[i]) == 0) {
          return true;
      }
  }
  return false;
}

static TokenType get_keyword_token(const char *str) {
  if (strcmp(str, "int") == 0) return TOKEN_INT;
  if (strcmp(str, "char") == 0) return TOKEN_CHAR_KW;
  if (strcmp(str, "float") == 0) return TOKEN_FLOAT;
  if (strcmp(str, "double") == 0) return TOKEN_DOUBLE;
  if (strcmp(str, "void") == 0) return TOKEN_VOID;
  if (strcmp(str, "if") == 0) return TOKEN_IF;
  if (strcmp(str, "else") == 0) return TOKEN_ELSE;
  if (strcmp(str, "while") == 0) return TOKEN_WHILE;
  if (strcmp(str, "for") == 0) return TOKEN_FOR;
  if (strcmp(str, "return") == 0) return TOKEN_RETURN;
  
  return TOKEN_IDENTIFIER;
}

static Token lexer_scan_identifier(Lexer *lexer) {
  Token token;
  int start = lexer->position;
  
  while (isalnum(lexer->current) || lexer->current == '_') {
      lexer_advance(lexer);
  }
  
  int length = lexer->position - start;
  char *lexeme = malloc(length + 1);
  strncpy(lexeme, lexer->source + start, length);
  lexeme[length] = '\0';
  
  token.lexeme = lexeme;
  token.line = lexer->line;
  
  if (is_keyword(lexeme)) {
      token.type = get_keyword_token(lexeme);
  } else {
      token.type = TOKEN_IDENTIFIER;
  }
  
  return token;
}

static Token lexer_scan_number(Lexer *lexer) {
  Token token;
  int start = lexer->position;
  bool is_float = false;
  
  while (isdigit(lexer->current)) {
      lexer_advance(lexer);
  }
  
  if (lexer->current == '.' && isdigit(lexer->source[lexer->position + 1])) {
      is_float = true;
      lexer_advance(lexer); // Skip '.'
      
      while (isdigit(lexer->current)) {
          lexer_advance(lexer);
      }
  }
  
  int length = lexer->position - start;
  char *lexeme = malloc(length + 1);
  strncpy(lexeme, lexer->source + start, length);
  lexeme[length] = '\0';
  
  token.lexeme = lexeme;
  token.line = lexer->line;
  
  if (is_float) {
      token.type = TOKEN_NUMBER_FLOAT;
      token.value.float_value = atof(lexeme);
  } else {
      token.type = TOKEN_NUMBER_INT;
      token.value.int_value = atoi(lexeme);
  }
  
  return token;
}

static Token lexer_scan_string(Lexer *lexer) {
  Token token;
  lexer_advance(lexer); // Skip opening quote
  
  int start = lexer->position;
  while (lexer->current != '"' && lexer->current != '\0') {
      // Handle escape sequences
      if (lexer->current == '\\') {
          lexer_advance(lexer);
      }
      if (lexer->current == '\n') {
          lexer->line++;
      }
      lexer_advance(lexer);
  }
  
  int length = lexer->position - start;
  char *lexeme = malloc(length + 1);
  strncpy(lexeme, lexer->source + start, length);
  lexeme[length] = '\0';
  
  token.type = TOKEN_STRING;
  token.lexeme = lexeme;
  token.value.str_value = lexeme;
  token.line = lexer->line;
  
  if (lexer->current == '"') {
      lexer_advance(lexer); // Skip closing quote
  }
  
  return token;
}

static Token lexer_scan_char(Lexer *lexer) {
  Token token;
  lexer_advance(lexer); // Skip opening quote
  
  char value = lexer->current;
  // Handle escape sequences
  if (value == '\\') {
      lexer_advance(lexer);
      switch (lexer->current) {
          case 'n': value = '\n'; break;
          case 't': value = '\t'; break;
          case 'r': value = '\r'; break;
          case '0': value = '\0'; break;
          default: value = lexer->current;
      }
  }
  
  token.type = TOKEN_CHAR;
  token.lexeme = malloc(2);
  token.lexeme[0] = value;
  token.lexeme[1] = '\0';
  token.value.char_value = value;
  token.line = lexer->line;
  
  lexer_advance(lexer); // Skip the character
  
  if (lexer->current == '\'') {
      lexer_advance(lexer); // Skip closing quote
  }
  
  return token;
}

Token lexer_next_token(Lexer *lexer) {
  lexer_skip_whitespace(lexer);
  
  while (lexer->current == '/' && (lexer->source[lexer->position + 1] == '/' || 
                                    lexer->source[lexer->position + 1] == '*')) {
      lexer_skip_comment(lexer);
      lexer_skip_whitespace(lexer);
  }
  
  Token token;
  token.line = lexer->line;
  
  if (lexer->current == '\0') {
      token.type = TOKEN_EOF;
      token.lexeme = strdup("EOF");
      return token;
  }
  
  // Identifiers and keywords
  if (isalpha(lexer->current) || lexer->current == '_') {
      return lexer_scan_identifier(lexer);
  }
  
  // Numbers
  if (isdigit(lexer->current)) {
      return lexer_scan_number(lexer);
  }
  
  // Strings
  if (lexer->current == '"') {
      return lexer_scan_string(lexer);
  }
  
  // Characters
  if (lexer->current == '\'') {
      return lexer_scan_char(lexer);
  }
  
  // Operators and punctuation
  switch (lexer->current) {
      case '+': 
          token.type = TOKEN_PLUS; 
          token.lexeme = strdup("+");
          break;
      case '-': 
          if (lexer->source[lexer->position + 1] == '>') {
              token.type = TOKEN_ARROW;
              token.lexeme = strdup("->");
              lexer_advance(lexer);
          } else {
              token.type = TOKEN_MINUS;
              token.lexeme = strdup("-");
          }
          break;
      case '*': 
          token.type = TOKEN_MULTIPLY; 
          token.lexeme = strdup("*");
          break;
      case '/': 
          token.type = TOKEN_DIVIDE; 
          token.lexeme = strdup("/");
          break;
      case '%': 
          token.type = TOKEN_MODULO; 
          token.lexeme = strdup("%");
          break;
      case '=': 
          if (lexer->source[lexer->position + 1] == '=') {
              token.type = TOKEN_EQ;
              token.lexeme = strdup("==");
              lexer_advance(lexer);
          } else {
              token.type = TOKEN_ASSIGN;
              token.lexeme = strdup("=");
          }
          break;
      case '!': 
          if (lexer->source[lexer->position + 1] == '=') {
              token.type = TOKEN_NEQ;
              token.lexeme = strdup("!=");
              lexer_advance(lexer);
          } else {
              token.type = TOKEN_NOT;
              token.lexeme = strdup("!");
          }
          break;
      case '<': 
          if (lexer->source[lexer->position + 1] == '=') {
              token.type = TOKEN_LE;
              token.lexeme = strdup("<=");
              lexer_advance(lexer);
          } else if (lexer->source[lexer->position + 1] == '<') {
              token.type = TOKEN_BIT_SHL;
              token.lexeme = strdup("<<");
              lexer_advance(lexer);
          } else {
              token.type = TOKEN_LT;
              token.lexeme = strdup("<");
          }
          break;
      case '>': 
          if (lexer->source[lexer->position + 1] == '=') {
              token.type = TOKEN_GE;
              token.lexeme = strdup(">=");
              lexer_advance(lexer);
          } else if (lexer->source[lexer->position + 1] == '>') {
              token.type = TOKEN_BIT_SHR;
              token.lexeme = strdup(">>");
              lexer_advance(lexer);
          } else {
              token.type = TOKEN_GT;
              token.lexeme = strdup(">");
          }
          break;
      case '&': 
          if (lexer->source[lexer->position + 1] == '&') {
              token.type = TOKEN_AND;
              token.lexeme = strdup("&&");
              lexer_advance(lexer);
          } else {
              token.type = TOKEN_BIT_AND;
              token.lexeme = strdup("&");
          }
          break;
      case '|': 
          if (lexer->source[lexer->position + 1] == '|') {
              token.type = TOKEN_OR;
              token.lexeme = strdup("||");
              lexer_advance(lexer);
          } else {
              token.type = TOKEN_BIT_OR;
              token.lexeme = strdup("|");
          }
          break;
      case '^': 
          token.type = TOKEN_BIT_XOR; 
          token.lexeme = strdup("^");
          break;
      case '~': 
          token.type = TOKEN_BIT_NOT; 
          token.lexeme = strdup("~");
          break;
      case '(': 
          token.type = TOKEN_LPAREN; 
          token.lexeme = strdup("(");
          break;
      case ')': 
          token.type = TOKEN_RPAREN; 
          token.lexeme = strdup(")");
          break;
      case '{': 
          token.type = TOKEN_LBRACE; 
          token.lexeme = strdup("{");
          break;
      case '}': 
          token.type = TOKEN_RBRACE; 
          token.lexeme = strdup("}");
          break;
      case '[': 
          token.type = TOKEN_LBRACKET; 
          token.lexeme = strdup("[");
          break;
      case ']': 
          token.type = TOKEN_RBRACKET; 
          token.lexeme = strdup("]");
          break;
      case ';': 
          token.type = TOKEN_SEMICOLON; 
          token.lexeme = strdup(";");
          break;
      case ',': 
          token.type = TOKEN_COMMA; 
          token.lexeme = strdup(",");
          break;
      case '.': 
          token.type = TOKEN_DOT; 
          token.lexeme = strdup(".");
          break;
      default:
          token.type = TOKEN_UNKNOWN;
          token.lexeme = malloc(2);
          token.lexeme[0] = lexer->current;
          token.lexeme[1] = '\0';
  }
  
  lexer_advance(lexer);
  return token;
}

void token_free(Token token) {
  if (token.lexeme) {
      free(token.lexeme);
  }
}