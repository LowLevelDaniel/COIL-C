/**
 * @file lexer.c
 * @brief Lexical analyzer implementation for C source code
 */

#include "../include/lexer.h"
#include "../include/arena.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// Helper functions for character checking
static inline bool is_alpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static inline bool is_digit(char c) {
  return c >= '0' && c <= '9';
}

static inline bool is_alnum(char c) {
  return is_alpha(c) || is_digit(c);
}

static inline bool is_hex_digit(char c) {
  return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static inline bool is_octal_digit(char c) {
  return c >= '0' && c <= '7';
}

static inline bool is_whitespace(char c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

// Keyword table
typedef struct {
  const char* keyword;
  TokenType type;
} Keyword;

static const Keyword keywords[] = {
  {"auto", TOKEN_AUTO},
  {"break", TOKEN_BREAK},
  {"case", TOKEN_CASE},
  {"char", TOKEN_CHAR},
  {"const", TOKEN_CONST},
  {"continue", TOKEN_CONTINUE},
  {"default", TOKEN_DEFAULT},
  {"do", TOKEN_DO},
  {"double", TOKEN_DOUBLE},
  {"else", TOKEN_ELSE},
  {"enum", TOKEN_ENUM},
  {"extern", TOKEN_EXTERN},
  {"float", TOKEN_FLOAT},
  {"for", TOKEN_FOR},
  {"goto", TOKEN_GOTO},
  {"if", TOKEN_IF},
  {"int", TOKEN_INT},
  {"long", TOKEN_LONG},
  {"register", TOKEN_REGISTER},
  {"return", TOKEN_RETURN},
  {"short", TOKEN_SHORT},
  {"signed", TOKEN_SIGNED},
  {"sizeof", TOKEN_SIZEOF},
  {"static", TOKEN_STATIC},
  {"struct", TOKEN_STRUCT},
  {"switch", TOKEN_SWITCH},
  {"typedef", TOKEN_TYPEDEF},
  {"union", TOKEN_UNION},
  {"unsigned", TOKEN_UNSIGNED},
  {"void", TOKEN_VOID},
  {"volatile", TOKEN_VOLATILE},
  {"while", TOKEN_WHILE},
  {NULL, TOKEN_EOF}
};

// Helper functions for lexer
static char lexer_peek(Lexer* lexer) {
  if (lexer->position >= lexer->source_length) {
    return '\0';
  }
  return lexer->source[lexer->position];
}

static char lexer_peek_next(Lexer* lexer) {
  if (lexer->position + 1 >= lexer->source_length) {
    return '\0';
  }
  return lexer->source[lexer->position + 1];
}

static char lexer_advance(Lexer* lexer) {
  char c = lexer_peek(lexer);
  if (c == '\0') return c;
  
  lexer->position++;
  
  if (c == '\n') {
    lexer->line++;
    lexer->column = 0;
  } else {
    lexer->column++;
  }
  
  return c;
}

static bool lexer_match(Lexer* lexer, char expected) {
  if (lexer_peek(lexer) != expected) {
    return false;
  }
  
  lexer_advance(lexer);
  return true;
}

static void lexer_skip_whitespace(Lexer* lexer) {
  for (;;) {
    char c = lexer_peek(lexer);
    
    if (c == ' ' || c == '\r' || c == '\t') {
      lexer_advance(lexer);
    } else if (c == '\n') {
      lexer_advance(lexer);
    } else {
      break;
    }
  }
}

static void lexer_skip_comment(Lexer* lexer) {
  // Line comment
  if (lexer_peek(lexer) == '/' && lexer_peek_next(lexer) == '/') {
    lexer_advance(lexer); // Skip '/'
    lexer_advance(lexer); // Skip '/'
    
    // Read until end of line or end of file
    while (lexer_peek(lexer) != '\n' && lexer_peek(lexer) != '\0') {
      lexer_advance(lexer);
    }
  }
  // Block comment
  else if (lexer_peek(lexer) == '/' && lexer_peek_next(lexer) == '*') {
    lexer_advance(lexer); // Skip '/'
    lexer_advance(lexer); // Skip '*'
    
    // Read until */ or end of file
    while (!(lexer_peek(lexer) == '*' && lexer_peek_next(lexer) == '/') &&
           lexer_peek(lexer) != '\0') {
      lexer_advance(lexer);
    }
    
    if (lexer_peek(lexer) != '\0') {
      lexer_advance(lexer); // Skip '*'
      lexer_advance(lexer); // Skip '/'
    }
  }
}

static Token lexer_make_token(Lexer* lexer, TokenType type) {
  Token token;
  token.type = type;
  token.location.file = lexer->filename;
  token.location.line = lexer->line;
  token.location.column = lexer->column;
  
  // For now, we don't store the lexeme directly
  token.lexeme = NULL;
  token.length = 0;
  
  return token;
}

static Token lexer_error_token(Lexer* lexer, const char* message) {
  lexer->has_error = true;
  lexer->error_message = arena_strdup(lexer->arena, message);
  
  Token token = lexer_make_token(lexer, TOKEN_EOF);
  return token;
}

static TokenType check_keyword(const char* identifier, int length) {
  for (int i = 0; keywords[i].keyword != NULL; i++) {
    if (strncmp(keywords[i].keyword, identifier, length) == 0 &&
        keywords[i].keyword[length] == '\0') {
      return keywords[i].type;
    }
  }
  
  return TOKEN_IDENTIFIER;
}

static Token lexer_identifier(Lexer* lexer) {
  size_t start = lexer->position - 1; // We've already consumed the first character
  
  while (is_alnum(lexer_peek(lexer))) {
    lexer_advance(lexer);
  }
  
  // Determine if it's a keyword or identifier
  size_t length = lexer->position - start;
  TokenType type = check_keyword(lexer->source + start, length);
  
  Token token = lexer_make_token(lexer, type);
  token.lexeme = lexer->source + start;
  token.length = length;
  
  return token;
}

static Token lexer_number(Lexer* lexer) {
  size_t start = lexer->position - 1; // We've already consumed the first digit
  bool is_float = false;
  
  // Check for hex or octal
  bool is_hex = false;
  bool is_octal = false;
  
  if (lexer->source[start] == '0') {
    if (lexer_peek(lexer) == 'x' || lexer_peek(lexer) == 'X') {
      is_hex = true;
      lexer_advance(lexer);
      
      // Read hex digits
      while (is_hex_digit(lexer_peek(lexer))) {
        lexer_advance(lexer);
      }
    } else if (is_octal_digit(lexer_peek(lexer))) {
      is_octal = true;
      
      // Read octal digits
      while (is_octal_digit(lexer_peek(lexer))) {
        lexer_advance(lexer);
      }
    }
  }
  
  // If not hex or octal, parse as decimal or float
  if (!is_hex && !is_octal) {
    // Read integer part
    while (is_digit(lexer_peek(lexer))) {
      lexer_advance(lexer);
    }
    
    // Read decimal part
    if (lexer_peek(lexer) == '.' && is_digit(lexer_peek_next(lexer))) {
      is_float = true;
      lexer_advance(lexer); // Skip '.'
      
      // Read fractional part
      while (is_digit(lexer_peek(lexer))) {
        lexer_advance(lexer);
      }
    }
    
    // Read exponent
    if (lexer_peek(lexer) == 'e' || lexer_peek(lexer) == 'E') {
      is_float = true;
      lexer_advance(lexer);
      
      // Optional sign
      if (lexer_peek(lexer) == '+' || lexer_peek(lexer) == '-') {
        lexer_advance(lexer);
      }
      
      // Exponent digits
      if (!is_digit(lexer_peek(lexer))) {
        return lexer_error_token(lexer, "Invalid float literal: expected digit after exponent");
      }
      
      while (is_digit(lexer_peek(lexer))) {
        lexer_advance(lexer);
      }
    }
  }
  
  // Check for suffix (L, U, F, etc.)
  if (lexer_peek(lexer) == 'L' || lexer_peek(lexer) == 'l' ||
      lexer_peek(lexer) == 'U' || lexer_peek(lexer) == 'u' ||
      lexer_peek(lexer) == 'F' || lexer_peek(lexer) == 'f') {
    lexer_advance(lexer);
    
    // Check for LL or UL combinations
    if ((lexer_peek(lexer) == 'L' || lexer_peek(lexer) == 'l') &&
        (lexer->source[lexer->position - 1] == 'L' || lexer->source[lexer->position - 1] == 'l')) {
      lexer_advance(lexer);
    } else if ((lexer_peek(lexer) == 'U' || lexer_peek(lexer) == 'u') &&
               (lexer->source[lexer->position - 1] == 'L' || lexer->source[lexer->position - 1] == 'l')) {
      lexer_advance(lexer);
    } else if ((lexer_peek(lexer) == 'L' || lexer_peek(lexer) == 'l') &&
               (lexer->source[lexer->position - 1] == 'U' || lexer->source[lexer->position - 1] == 'u')) {
      lexer_advance(lexer);
    }
  }
  
  // Create token
  TokenType type = is_float ? TOKEN_FLOAT_LITERAL : TOKEN_INTEGER_LITERAL;
  Token token = lexer_make_token(lexer, type);
  token.lexeme = lexer->source + start;
  token.length = lexer->position - start;
  
  // Parse value
  if (type == TOKEN_INTEGER_LITERAL) {
    char* temp = arena_alloc(lexer->arena, token.length + 1);
    memcpy(temp, token.lexeme, token.length);
    temp[token.length] = '\0';
    
    if (is_hex) {
      token.value.int_value = strtoll(temp + 2, NULL, 16); // Skip "0x"
    } else if (is_octal) {
      token.value.int_value = strtoll(temp, NULL, 8);
    } else {
      token.value.int_value = strtoll(temp, NULL, 10);
    }
  } else {
    char* temp = arena_alloc(lexer->arena, token.length + 1);
    memcpy(temp, token.lexeme, token.length);
    temp[token.length] = '\0';
    
    token.value.float_value = strtod(temp, NULL);
  }
  
  return token;
}

static char parse_escape_sequence(Lexer* lexer) {
  switch (lexer_peek(lexer)) {
    case '\'': lexer_advance(lexer); return '\'';
    case '"':  lexer_advance(lexer); return '"';
    case '?':  lexer_advance(lexer); return '?';
    case '\\': lexer_advance(lexer); return '\\';
    case 'a':  lexer_advance(lexer); return '\a';
    case 'b':  lexer_advance(lexer); return '\b';
    case 'f':  lexer_advance(lexer); return '\f';
    case 'n':  lexer_advance(lexer); return '\n';
    case 'r':  lexer_advance(lexer); return '\r';
    case 't':  lexer_advance(lexer); return '\t';
    case 'v':  lexer_advance(lexer); return '\v';
    case 'x': {
      lexer_advance(lexer); // Skip 'x'
      
      // Parse up to 2 hex digits
      if (!is_hex_digit(lexer_peek(lexer))) {
        lexer->has_error = true;
        lexer->error_message = arena_strdup(lexer->arena, "Invalid hex escape sequence");
        return '\0';
      }
      
      int value = 0;
      for (int i = 0; i < 2 && is_hex_digit(lexer_peek(lexer)); i++) {
        char c = lexer_advance(lexer);
        value = value * 16 + (is_digit(c) ? c - '0' :
                              (c >= 'a' ? c - 'a' + 10 : c - 'A' + 10));
      }
      
      return (char)value;
    }
    case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7': {
      // Parse up to 3 octal digits
      int value = 0;
      for (int i = 0; i < 3 && is_octal_digit(lexer_peek(lexer)); i++) {
        char c = lexer_advance(lexer);
        value = value * 8 + (c - '0');
      }
      
      return (char)value;
    }
    default:
      lexer->has_error = true;
      lexer->error_message = arena_strdup(lexer->arena, "Invalid escape sequence");
      return '\0';
  }
}

static Token lexer_string(Lexer* lexer) {
  // Start with empty string
  size_t capacity = 16;
  char* buffer = arena_alloc(lexer->arena, capacity);
  size_t length = 0;
  
  while (lexer_peek(lexer) != '"' && lexer_peek(lexer) != '\0') {
    char c;
    
    if (lexer_peek(lexer) == '\\') {
      lexer_advance(lexer); // Skip '\'
      c = parse_escape_sequence(lexer);
      if (lexer->has_error) {
        Token token = lexer_make_token(lexer, TOKEN_EOF);
        return token;
      }
    } else {
      c = lexer_advance(lexer);
    }
    
    // Resize buffer if needed
    if (length + 1 >= capacity) {
      size_t new_capacity = capacity * 2;
      char* new_buffer = arena_alloc(lexer->arena, new_capacity);
      memcpy(new_buffer, buffer, length);
      buffer = new_buffer;
      capacity = new_capacity;
    }
    
    buffer[length++] = c;
  }
  
  if (lexer_peek(lexer) == '\0') {
    return lexer_error_token(lexer, "Unterminated string literal");
  }
  
  lexer_advance(lexer); // Skip closing quote
  
  // Null-terminate the string
  buffer[length] = '\0';
  
  Token token = lexer_make_token(lexer, TOKEN_STRING_LITERAL);
  token.value.string_value = buffer;
  
  return token;
}

static Token lexer_char(Lexer* lexer) {
  char c;
  
  if (lexer_peek(lexer) == '\\') {
    lexer_advance(lexer); // Skip '\'
    c = parse_escape_sequence(lexer);
    if (lexer->has_error) {
      Token token = lexer_make_token(lexer, TOKEN_EOF);
      return token;
    }
  } else {
    c = lexer_advance(lexer);
  }
  
  if (lexer_peek(lexer) != '\'') {
    return lexer_error_token(lexer, "Unterminated character literal");
  }
  
  lexer_advance(lexer); // Skip closing quote
  
  Token token = lexer_make_token(lexer, TOKEN_CHAR_LITERAL);
  token.value.char_value = c;
  
  return token;
}

Lexer* lexer_create(const char* source, const char* filename, struct Arena* arena) {
  Lexer* lexer = arena_alloc(arena, sizeof(Lexer));
  lexer->source = source;
  lexer->filename = filename;
  lexer->source_length = strlen(source);
  lexer->position = 0;
  lexer->line = 1;
  lexer->column = 0;
  lexer->arena = arena;
  lexer->has_error = false;
  lexer->error_message = NULL;
  
  // Pre-load the first token
  lexer->current = lexer_next_token(lexer);
  
  return lexer;
}

Token lexer_next_token(Lexer* lexer) {
  // Skip whitespace and comments
  for (;;) {
    lexer_skip_whitespace(lexer);
    
    if (lexer_peek(lexer) == '/' && (lexer_peek_next(lexer) == '/' || lexer_peek_next(lexer) == '*')) {
      lexer_skip_comment(lexer);
    } else {
      break;
    }
  }
  
  // Check for end of file
  if (lexer_peek(lexer) == '\0') {
    return lexer_make_token(lexer, TOKEN_EOF);
  }
  
  char c = lexer_advance(lexer);
  
  // Identifier or keyword
  if (is_alpha(c)) {
    return lexer_identifier(lexer);
  }
  
  // Number
  if (is_digit(c)) {
    return lexer_number(lexer);
  }
  
  // String literal
  if (c == '"') {
    return lexer_string(lexer);
  }
  
  // Character literal
  if (c == '\'') {
    return lexer_char(lexer);
  }
  
  // Single-character tokens
  switch (c) {
    case '(': return lexer_make_token(lexer, TOKEN_LEFT_PAREN);
    case ')': return lexer_make_token(lexer, TOKEN_RIGHT_PAREN);
    case '{': return lexer_make_token(lexer, TOKEN_LEFT_BRACE);
    case '}': return lexer_make_token(lexer, TOKEN_RIGHT_BRACE);
    case '[': return lexer_make_token(lexer, TOKEN_LEFT_BRACKET);
    case ']': return lexer_make_token(lexer, TOKEN_RIGHT_BRACKET);
    case ';': return lexer_make_token(lexer, TOKEN_SEMICOLON);
    case ',': return lexer_make_token(lexer, TOKEN_COMMA);
    case '.': return lexer_make_token(lexer, TOKEN_DOT);
    case '?': return lexer_make_token(lexer, TOKEN_QUESTION);
    case ':': return lexer_make_token(lexer, TOKEN_COLON);
    case '~': return lexer_make_token(lexer, TOKEN_TILDE);
  }
  
  // Two-character tokens
  switch (c) {
    case '+':
      if (lexer_match(lexer, '+')) return lexer_make_token(lexer, TOKEN_PLUS_PLUS);
      if (lexer_match(lexer, '=')) return lexer_make_token(lexer, TOKEN_PLUS_EQUAL);
      return lexer_make_token(lexer, TOKEN_PLUS);
    
    case '-':
      if (lexer_match(lexer, '-')) return lexer_make_token(lexer, TOKEN_MINUS_MINUS);
      if (lexer_match(lexer, '=')) return lexer_make_token(lexer, TOKEN_MINUS_EQUAL);
      if (lexer_match(lexer, '>')) return lexer_make_token(lexer, TOKEN_ARROW);
      return lexer_make_token(lexer, TOKEN_MINUS);
    
    case '*':
      if (lexer_match(lexer, '=')) return lexer_make_token(lexer, TOKEN_STAR_EQUAL);
      return lexer_make_token(lexer, TOKEN_STAR);
    
    case '/':
      if (lexer_match(lexer, '=')) return lexer_make_token(lexer, TOKEN_SLASH_EQUAL);
      return lexer_make_token(lexer, TOKEN_SLASH);
    
    case '%':
      if (lexer_match(lexer, '=')) return lexer_make_token(lexer, TOKEN_PERCENT_EQUAL);
      return lexer_make_token(lexer, TOKEN_PERCENT);
    
    case '&':
      if (lexer_match(lexer, '&')) return lexer_make_token(lexer, TOKEN_AMPERSAND_AMPERSAND);
      if (lexer_match(lexer, '=')) return lexer_make_token(lexer, TOKEN_AMPERSAND_EQUAL);
      return lexer_make_token(lexer, TOKEN_AMPERSAND);
    
    case '|':
      if (lexer_match(lexer, '|')) return lexer_make_token(lexer, TOKEN_PIPE_PIPE);
      if (lexer_match(lexer, '=')) return lexer_make_token(lexer, TOKEN_PIPE_EQUAL);
      return lexer_make_token(lexer, TOKEN_PIPE);
    
    case '^':
      if (lexer_match(lexer, '=')) return lexer_make_token(lexer, TOKEN_CARET_EQUAL);
      return lexer_make_token(lexer, TOKEN_CARET);
    
    case '!':
      if (lexer_match(lexer, '=')) return lexer_make_token(lexer, TOKEN_EXCLAIM_EQUAL);
      return lexer_make_token(lexer, TOKEN_EXCLAIM);
    
    case '=':
      if (lexer_match(lexer, '=')) return lexer_make_token(lexer, TOKEN_EQUAL_EQUAL);
      return lexer_make_token(lexer, TOKEN_EQUAL);
    
    case '<':
      if (lexer_match(lexer, '=')) return lexer_make_token(lexer, TOKEN_LESS_EQUAL);
      if (lexer_match(lexer, '<')) {
        if (lexer_match(lexer, '=')) return lexer_make_token(lexer, TOKEN_LESS_LESS_EQUAL);
        return lexer_make_token(lexer, TOKEN_LESS_LESS);
      }
      return lexer_make_token(lexer, TOKEN_LESS);
    
    case '>':
      if (lexer_match(lexer, '=')) return lexer_make_token(lexer, TOKEN_GREATER_EQUAL);
      if (lexer_match(lexer, '>')) {
        if (lexer_match(lexer, '=')) return lexer_make_token(lexer, TOKEN_GREATER_GREATER_EQUAL);
        return lexer_make_token(lexer, TOKEN_GREATER_GREATER);
      }
      return lexer_make_token(lexer, TOKEN_GREATER);
  }
  
  return lexer_error_token(lexer, "Unexpected character");
}

Token lexer_peek_token(Lexer* lexer) {
  return lexer->current;
}

bool lexer_check(Lexer* lexer, TokenType type) {
  return lexer->current.type == type;
}

bool lexer_expect(Lexer* lexer, TokenType type) {
  if (lexer_check(lexer, type)) {
    lexer_next_token(lexer);
    return true;
  }
  
  lexer->has_error = true;
  lexer->error_message = arena_strdup(lexer->arena, "Unexpected token");
  return false;
}

bool lexer_consume(Lexer* lexer, TokenType type) {
  if (lexer_check(lexer, type)) {
    lexer_next_token(lexer);
    return true;
  }
  
  return false;
}

SourceLocation lexer_location(Lexer* lexer) {
  SourceLocation location;
  location.file = lexer->filename;
  location.line = lexer->line;
  location.column = lexer->column;
  return location;
}

const char* lexer_error(Lexer* lexer) {
  if (lexer->has_error) {
    return lexer->error_message;
  }
  
  return NULL;
}