/**
 * Enhanced Lexical Analyzer for the COIL C Compiler
 * Converts C source code into a stream of tokens with robust error handling
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

/**
 * Initialize a lexer with source code
 */
void lexer_init(Lexer *lexer, const char *source) {
    lexer->source = source;
    lexer->position = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->current = source[0];
}

/**
 * Report a lexer error and exit
 */
static void lexer_error(Lexer *lexer, const char *message) {
    fprintf(stderr, "Lexical error (line %d, column %d): %s\n", 
            lexer->line, lexer->column, message);
    exit(1);
}

/**
 * Advance to the next character in the source
 */
static void lexer_advance(Lexer *lexer) {
    if (lexer->current == '\n') {
        lexer->line++;
        lexer->column = 0;
    }
    
    if (lexer->current != '\0') {
        lexer->position++;
        lexer->current = lexer->source[lexer->position];
        lexer->column++;
    }
}

/**
 * Peek at the next character without advancing
 */
static char lexer_peek(Lexer *lexer) {
    if (lexer->current == '\0') {
        return '\0';
    }
    return lexer->source[lexer->position + 1];
}

/**
 * Skip whitespace characters
 */
static void lexer_skip_whitespace(Lexer *lexer) {
    while (isspace(lexer->current)) {
        lexer_advance(lexer);
    }
}

/**
 * Skip comments (both line and block comments)
 */
static void lexer_skip_comment(Lexer *lexer) {
    // Skip single-line comment
    if (lexer->current == '/' && lexer_peek(lexer) == '/') {
        lexer_advance(lexer); // Skip first '/'
        lexer_advance(lexer); // Skip second '/'
        
        while (lexer->current != '\n' && lexer->current != '\0') {
            lexer_advance(lexer);
        }
    }
    // Skip multi-line comment
    else if (lexer->current == '/' && lexer_peek(lexer) == '*') {
        lexer_advance(lexer); // Skip '/'
        lexer_advance(lexer); // Skip '*'
        
        while (!(lexer->current == '*' && lexer_peek(lexer) == '/') && 
               lexer->current != '\0') {
            lexer_advance(lexer);
        }
        
        if (lexer->current == '\0') {
            lexer_error(lexer, "Unterminated multi-line comment");
        } else {
            lexer_advance(lexer); // Skip '*'
            lexer_advance(lexer); // Skip '/'
        }
    }
}

/**
 * Check if a string is a C keyword
 */
static bool is_keyword(const char *str) {
    static const char *keywords[] = {
        "int", "char", "float", "double", "void",
        "if", "else", "while", "for", "return",
        "break", "continue", "struct", "union",
        "typedef", "enum", "sizeof", "static",
        "extern", "const", "volatile", "register", 
        "auto", "goto", NULL
    };
    
    for (int i = 0; keywords[i] != NULL; i++) {
        if (strcmp(str, keywords[i]) == 0) {
            return true;
        }
    }
    return false;
}

/**
 * Map a keyword string to its token type
 */
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
    
    // Note: Other keywords are recognized as identifiers for now
    // since we don't need them in our simplified C implementation
    return TOKEN_IDENTIFIER;
}

/**
 * Scan an identifier or keyword
 */
static Token lexer_scan_identifier(Lexer *lexer) {
    Token token;
    int start_position = lexer->position;
    int start_column = lexer->column;
    
    // Record the starting line in case it spans multiple lines (which is invalid)
    int start_line = lexer->line;
    
    // Scan the identifier or keyword
    while (isalnum(lexer->current) || lexer->current == '_') {
        lexer_advance(lexer);
    }
    
    // Check for invalid multi-line identifier
    if (lexer->line != start_line) {
        lexer_error(lexer, "Identifier cannot span multiple lines");
    }
    
    // Extract the text
    int length = lexer->position - start_position;
    char *lexeme = malloc(length + 1);
    if (lexeme == NULL) {
        fprintf(stderr, "Memory allocation failed for identifier\n");
        exit(1);
    }
    
    strncpy(lexeme, lexer->source + start_position, length);
    lexeme[length] = '\0';
    
    // Set up the token
    token.lexeme = lexeme;
    token.line = start_line;
    token.column = start_column;
    
    // Determine if it's a keyword or identifier
    if (is_keyword(lexeme)) {
        token.type = get_keyword_token(lexeme);
    } else {
        token.type = TOKEN_IDENTIFIER;
    }
    
    return token;
}

/**
 * Scan a number (integer or floating-point)
 */
static Token lexer_scan_number(Lexer *lexer) {
    Token token;
    int start_position = lexer->position;
    int start_column = lexer->column;
    int start_line = lexer->line;
    bool is_float = false;
    
    // Scan the integer part
    while (isdigit(lexer->current)) {
        lexer_advance(lexer);
    }
    
    // Check for decimal point
    if (lexer->current == '.' && isdigit(lexer_peek(lexer))) {
        is_float = true;
        lexer_advance(lexer); // Skip '.'
        
        // Scan the fractional part
        while (isdigit(lexer->current)) {
            lexer_advance(lexer);
        }
    }
    
    // Check for exponent (e.g., 1.23e+45)
    if ((lexer->current == 'e' || lexer->current == 'E') && 
        (isdigit(lexer_peek(lexer)) || 
         (lexer_peek(lexer) == '+' || lexer_peek(lexer) == '-'))) {
        
        is_float = true;
        lexer_advance(lexer); // Skip 'e' or 'E'
        
        // Skip the sign if present
        if (lexer->current == '+' || lexer->current == '-') {
            lexer_advance(lexer);
        }
        
        // There must be at least one digit after the exponent
        if (!isdigit(lexer->current)) {
            lexer_error(lexer, "Expected at least one digit after exponent");
        }
        
        // Scan the exponent
        while (isdigit(lexer->current)) {
            lexer_advance(lexer);
        }
    }
    
    // Check for invalid multi-line number
    if (lexer->line != start_line) {
        lexer_error(lexer, "Number cannot span multiple lines");
    }
    
    // Extract the text
    int length = lexer->position - start_position;
    char *lexeme = malloc(length + 1);
    if (lexeme == NULL) {
        fprintf(stderr, "Memory allocation failed for number\n");
        exit(1);
    }
    
    strncpy(lexeme, lexer->source + start_position, length);
    lexeme[length] = '\0';
    
    // Set up the token
    token.lexeme = lexeme;
    token.line = start_line;
    token.column = start_column;
    
    if (is_float) {
        token.type = TOKEN_NUMBER_FLOAT;
        token.value.float_value = (float)atof(lexeme);
    } else {
        token.type = TOKEN_NUMBER_INT;
        token.value.int_value = atoi(lexeme);
    }
    
    return token;
}

/**
 * Scan a string literal
 */
static Token lexer_scan_string(Lexer *lexer) {
    Token token;
    int start_line = lexer->line;
    int start_column = lexer->column;
    
    lexer_advance(lexer); // Skip opening quote
    
    int start_position = lexer->position;
    
    // Scan until closing quote or end of file
    while (lexer->current != '"' && lexer->current != '\0') {
        // Handle escape sequences
        if (lexer->current == '\\') {
            lexer_advance(lexer);
            if (lexer->current == '\0') {
                lexer_error(lexer, "Unterminated string literal");
            }
        }
        
        // Check for newline in string
        if (lexer->current == '\n') {
            lexer_error(lexer, "Newline in string literal");
        }
        
        lexer_advance(lexer);
    }
    
    // Check for unterminated string
    if (lexer->current == '\0') {
        lexer_error(lexer, "Unterminated string literal");
    }
    
    // Extract the string contents (without the quotes)
    int length = lexer->position - start_position;
    char *lexeme = malloc(length + 1);
    if (lexeme == NULL) {
        fprintf(stderr, "Memory allocation failed for string literal\n");
        exit(1);
    }
    
    strncpy(lexeme, lexer->source + start_position, length);
    lexeme[length] = '\0';
    
    // Set up the token
    token.type = TOKEN_STRING;
    token.lexeme = lexeme;
    token.value.str_value = lexeme; // Points to the same memory
    token.line = start_line;
    token.column = start_column;
    
    lexer_advance(lexer); // Skip closing quote
    
    return token;
}

/**
 * Scan a character literal
 */
static Token lexer_scan_char(Lexer *lexer) {
    Token token;
    int start_line = lexer->line;
    int start_column = lexer->column;
    
    lexer_advance(lexer); // Skip opening quote
    
    char value;
    
    // Handle escape sequences
    if (lexer->current == '\\') {
        lexer_advance(lexer);
        
        switch (lexer->current) {
            case 'n': value = '\n'; break;
            case 't': value = '\t'; break;
            case 'r': value = '\r'; break;
            case '0': value = '\0'; break;
            case '\\': value = '\\'; break;
            case '\'': value = '\''; break;
            case '"': value = '"'; break;
            default:
                lexer_error(lexer, "Invalid escape sequence");
                value = lexer->current; // Not reached, but makes compiler happy
        }
    } else if (lexer->current == '\n' || lexer->current == '\0') {
        lexer_error(lexer, "Unterminated character literal");
        value = '\0'; // Not reached, but makes compiler happy
    } else {
        value = lexer->current;
    }
    
    // Create token
    token.type = TOKEN_CHAR;
    token.lexeme = malloc(2);
    if (token.lexeme == NULL) {
        fprintf(stderr, "Memory allocation failed for character literal\n");
        exit(1);
    }
    
    token.lexeme[0] = value;
    token.lexeme[1] = '\0';
    token.value.char_value = value;
    token.line = start_line;
    token.column = start_column;
    
    lexer_advance(lexer); // Skip the character
    
    // Expect closing quote
    if (lexer->current != '\'') {
        lexer_error(lexer, "Expected closing quote after character literal");
    }
    
    lexer_advance(lexer); // Skip closing quote
    
    return token;
}

/**
 * Get the next token from the source
 */
Token lexer_next_token(Lexer *lexer) {
    // Skip whitespace and comments
    while (true) {
        lexer_skip_whitespace(lexer);
        
        // Check for comments
        if (lexer->current == '/' && (lexer_peek(lexer) == '/' || lexer_peek(lexer) == '*')) {
            lexer_skip_comment(lexer);
            continue;
        }
        
        break;
    }
    
    // Record token starting position
    int start_line = lexer->line;
    int start_column = lexer->column;
    
    // End of file
    if (lexer->current == '\0') {
        Token token;
        token.type = TOKEN_EOF;
        token.lexeme = strdup("EOF");
        token.line = start_line;
        token.column = start_column;
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
    
    // String literals
    if (lexer->current == '"') {
        return lexer_scan_string(lexer);
    }
    
    // Character literals
    if (lexer->current == '\'') {
        return lexer_scan_char(lexer);
    }
    
    // Create a token for a single character
    Token token;
    token.line = start_line;
    token.column = start_column;
    
    // Handle operators and punctuation
    switch (lexer->current) {
        case '+': 
            token.type = TOKEN_PLUS; 
            token.lexeme = strdup("+");
            break;
        case '-': 
            if (lexer_peek(lexer) == '>') {
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
            if (lexer_peek(lexer) == '=') {
                token.type = TOKEN_EQ;
                token.lexeme = strdup("==");
                lexer_advance(lexer);
            } else {
                token.type = TOKEN_ASSIGN;
                token.lexeme = strdup("=");
            }
            break;
        case '!': 
            if (lexer_peek(lexer) == '=') {
                token.type = TOKEN_NEQ;
                token.lexeme = strdup("!=");
                lexer_advance(lexer);
            } else {
                token.type = TOKEN_NOT;
                token.lexeme = strdup("!");
            }
            break;
        case '<': 
            if (lexer_peek(lexer) == '=') {
                token.type = TOKEN_LE;
                token.lexeme = strdup("<=");
                lexer_advance(lexer);
            } else if (lexer_peek(lexer) == '<') {
                token.type = TOKEN_BIT_SHL;
                token.lexeme = strdup("<<");
                lexer_advance(lexer);
            } else {
                token.type = TOKEN_LT;
                token.lexeme = strdup("<");
            }
            break;
        case '>': 
            if (lexer_peek(lexer) == '=') {
                token.type = TOKEN_GE;
                token.lexeme = strdup(">=");
                lexer_advance(lexer);
            } else if (lexer_peek(lexer) == '>') {
                token.type = TOKEN_BIT_SHR;
                token.lexeme = strdup(">>");
                lexer_advance(lexer);
            } else {
                token.type = TOKEN_GT;
                token.lexeme = strdup(">");
            }
            break;
        case '&': 
            if (lexer_peek(lexer) == '&') {
                token.type = TOKEN_AND;
                token.lexeme = strdup("&&");
                lexer_advance(lexer);
            } else {
                token.type = TOKEN_BIT_AND;
                token.lexeme = strdup("&");
            }
            break;
        case '|': 
            if (lexer_peek(lexer) == '|') {
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
            if (token.lexeme == NULL) {
                fprintf(stderr, "Memory allocation failed for token\n");
                exit(1);
            }
            token.lexeme[0] = lexer->current;
            token.lexeme[1] = '\0';
            
            // Print a warning for unknown characters
            fprintf(stderr, "Warning: Unknown character '%c' (ASCII %d) at line %d, column %d\n", 
                    lexer->current, lexer->current, lexer->line, lexer->column);
    }
    
    lexer_advance(lexer);
    return token;
}

/**
 * Free resources associated with a token
 */
void token_free(Token token) {
    if (token.lexeme) {
        free(token.lexeme);
    }
}