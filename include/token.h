/**
 * @file token.h
 * @brief Token definitions for the COIL C Compiler
 *
 * Defines the token types and structures used by the lexer and parser.
 */

#ifndef TOKEN_H
#define TOKEN_H

/**
 * Token types recognized by the lexer
 */
typedef enum {
    // End of file
    TOKEN_EOF,               /**< End of file */
    
    // Basic tokens
    TOKEN_IDENTIFIER,        /**< Identifier (variable or function name) */
    TOKEN_NUMBER_INT,        /**< Integer literal */
    TOKEN_NUMBER_FLOAT,      /**< Floating-point literal */
    TOKEN_STRING,            /**< String literal */
    TOKEN_CHAR,              /**< Character literal */
    
    // Keywords
    TOKEN_INT,               /**< int keyword */
    TOKEN_CHAR_KW,           /**< char keyword */
    TOKEN_FLOAT,             /**< float keyword */
    TOKEN_DOUBLE,            /**< double keyword */
    TOKEN_VOID,              /**< void keyword */
    TOKEN_IF,                /**< if keyword */
    TOKEN_ELSE,              /**< else keyword */
    TOKEN_WHILE,             /**< while keyword */
    TOKEN_FOR,               /**< for keyword */
    TOKEN_RETURN,            /**< return keyword */
    TOKEN_STRUCT,            /**< struct keyword */
    TOKEN_UNION,             /**< union keyword */
    TOKEN_TYPEDEF,           /**< typedef keyword */
    TOKEN_ENUM,              /**< enum keyword */
    TOKEN_SIZEOF,            /**< sizeof keyword */
    TOKEN_BREAK,             /**< break keyword */
    TOKEN_CONTINUE,          /**< continue keyword */
    TOKEN_STATIC,            /**< static keyword */
    TOKEN_EXTERN,            /**< extern keyword */
    TOKEN_CONST,             /**< const keyword */
    TOKEN_VOLATILE,          /**< volatile keyword */
    
    // Operators
    TOKEN_PLUS,              /**< + (addition) */
    TOKEN_MINUS,             /**< - (subtraction) */
    TOKEN_MULTIPLY,          /**< * (multiplication) */
    TOKEN_DIVIDE,            /**< / (division) */
    TOKEN_MODULO,            /**< % (modulo) */
    TOKEN_ASSIGN,            /**< = (assignment) */
    TOKEN_EQ,                /**< == (equal to) */
    TOKEN_NEQ,               /**< != (not equal to) */
    TOKEN_LT,                /**< < (less than) */
    TOKEN_LE,                /**< <= (less than or equal to) */
    TOKEN_GT,                /**< > (greater than) */
    TOKEN_GE,                /**< >= (greater than or equal to) */
    TOKEN_AND,               /**< && (logical AND) */
    TOKEN_OR,                /**< || (logical OR) */
    TOKEN_NOT,               /**< ! (logical NOT) */
    TOKEN_BIT_AND,           /**< & (bitwise AND) */
    TOKEN_BIT_OR,            /**< | (bitwise OR) */
    TOKEN_BIT_XOR,           /**< ^ (bitwise XOR) */
    TOKEN_BIT_NOT,           /**< ~ (bitwise NOT) */
    TOKEN_BIT_SHL,           /**< << (left shift) */
    TOKEN_BIT_SHR,           /**< >> (right shift) */
    TOKEN_INC,               /**< ++ (increment) */
    TOKEN_DEC,               /**< -- (decrement) */
    TOKEN_PLUS_ASSIGN,       /**< += (add and assign) */
    TOKEN_MINUS_ASSIGN,      /**< -= (subtract and assign) */
    TOKEN_MUL_ASSIGN,        /**< *= (multiply and assign) */
    TOKEN_DIV_ASSIGN,        /**< /= (divide and assign) */
    TOKEN_MOD_ASSIGN,        /**< %= (modulo and assign) */
    TOKEN_AND_ASSIGN,        /**< &= (AND and assign) */
    TOKEN_OR_ASSIGN,         /**< |= (OR and assign) */
    TOKEN_XOR_ASSIGN,        /**< ^= (XOR and assign) */
    TOKEN_SHL_ASSIGN,        /**< <<= (shift left and assign) */
    TOKEN_SHR_ASSIGN,        /**< >>= (shift right and assign) */
    
    // Punctuation
    TOKEN_LPAREN,            /**< ( (left parenthesis) */
    TOKEN_RPAREN,            /**< ) (right parenthesis) */
    TOKEN_LBRACE,            /**< { (left brace) */
    TOKEN_RBRACE,            /**< } (right brace) */
    TOKEN_LBRACKET,          /**< [ (left bracket) */
    TOKEN_RBRACKET,          /**< ] (right bracket) */
    TOKEN_SEMICOLON,         /**< ; (semicolon) */
    TOKEN_COMMA,             /**< , (comma) */
    TOKEN_DOT,               /**< . (dot) */
    TOKEN_ARROW,             /**< -> (arrow) */
    TOKEN_COLON,             /**< : (colon) */
    TOKEN_QUESTION,          /**< ? (question mark) */
    
    // Special
    TOKEN_COMMENT,           /**< Comment */
    TOKEN_UNKNOWN            /**< Unknown token */
} TokenType;

/**
 * Token structure representing a lexical unit
 */
typedef struct {
    TokenType type;    /**< Type of the token */
    char *lexeme;      /**< Text of the token */
    int line;          /**< Line number in source (1-based) */
    int column;        /**< Column number in source (1-based) */
    union {
        int int_value;       /**< Value for integer literals */
        float float_value;   /**< Value for floating-point literals */
        char char_value;     /**< Value for character literals */
        char *str_value;     /**< Value for string literals */
    } value;           /**< Semantic value of the token */
} Token;

/**
 * Free resources associated with a token
 * @param token The token to free
 */
void token_free(Token token);

/**
 * Create a copy of a token
 * @param token The token to copy
 * @return A new copy of the token
 */
Token token_copy(Token token);

/**
 * Get a string representation of a token type
 * @param type The token type
 * @return A string representation (must not be freed)
 */
const char *token_type_to_string(TokenType type);

/**
 * Create a token with the given type and lexeme
 * @param type Token type
 * @param lexeme Token text (will be copied)
 * @param line Line number
 * @param column Column number
 * @return A new token
 */
Token token_create(TokenType type, const char *lexeme, int line, int column);

#endif /* TOKEN_H */