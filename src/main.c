/**
 * C to COIL Compiler (c2coil)
 * 
 * A basic C compiler that targets COIL v1.0.0 as its backend
 * 
 * This compiler implements:
 * - Basic C syntax parsing
 * - Symbol table management
 * - Type checking
 * - COIL instruction generation
 * - COF (COIL Object Format) output
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

/* ========== Lexer ========== */

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

typedef struct {
    const char *source;
    int position;
    int line;
    char current;
} Lexer;

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

/* ========== Parser ========== */

typedef enum {
    TYPE_VOID,
    TYPE_INT,
    TYPE_CHAR,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_ARRAY,
    TYPE_POINTER
} DataType;

typedef struct Type {
    DataType base_type;
    struct Type *pointer_to;  // For pointer types
    int array_size;          // For array types
    int size;                // Size in bytes
} Type;

typedef enum {
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_LITERAL_INT,
    EXPR_LITERAL_FLOAT,
    EXPR_LITERAL_CHAR,
    EXPR_VARIABLE,
    EXPR_CALL,
    EXPR_ASSIGN,
    EXPR_SUBSCRIPT,
    EXPR_CAST
} ExpressionType;

typedef struct Expression {
    ExpressionType type;
    Type *data_type;
    union {
        struct {
            struct Expression *left;
            struct Expression *right;
            TokenType operator;
        } binary;
        struct {
            struct Expression *operand;
            TokenType operator;
        } unary;
        int int_value;
        float float_value;
        char char_value;
        char *variable_name;
        struct {
            char *function_name;
            struct Expression **arguments;
            int argument_count;
        } call;
        struct {
            char *variable_name;
            struct Expression *value;
        } assign;
        struct {
            struct Expression *array;
            struct Expression *index;
        } subscript;
        struct {
            Type *cast_type;
            struct Expression *expression;
        } cast;
    } value;
} Expression;

typedef enum {
    STMT_EXPRESSION,
    STMT_RETURN,
    STMT_IF,
    STMT_WHILE,
    STMT_FOR,
    STMT_BLOCK,
    STMT_DECLARATION
} StatementType;

typedef struct Statement {
    StatementType type;
    union {
        Expression *expression;
        struct {
            Expression *condition;
            struct Statement *then_branch;
            struct Statement *else_branch;
        } if_statement;
        struct {
            Expression *condition;
            struct Statement *body;
        } while_statement;
        struct {
            Expression *initializer;
            Expression *condition;
            Expression *increment;
            struct Statement *body;
        } for_statement;
        struct {
            struct Statement **statements;
            int statement_count;
        } block;
        struct {
            Type *type;
            char *name;
            Expression *initializer;
        } declaration;
    } value;
} Statement;

typedef struct {
    char *name;
    Type *return_type;
    Type **parameter_types;
    char **parameter_names;
    int parameter_count;
    Statement *body;
} Function;

typedef struct {
    Function **functions;
    int function_count;
    int function_capacity;
} Program;

typedef struct {
    Lexer *lexer;
    Token current_token;
    Token previous_token;
} Parser;

void parser_init(Parser *parser, Lexer *lexer) {
    parser->lexer = lexer;
    parser->current_token = lexer_next_token(lexer);
    
    // Initialize previous_token to prevent freeing uninitialized memory
    parser->previous_token.type = TOKEN_EOF;
    parser->previous_token.lexeme = NULL;
    parser->previous_token.line = 0;
}

static void parser_consume(Parser *parser) {
    token_free(parser->previous_token);
    parser->previous_token = parser->current_token;
    parser->current_token = lexer_next_token(parser->lexer);
}

static bool parser_match(Parser *parser, TokenType type) {
    if (parser->current_token.type == type) {
        parser_consume(parser);
        return true;
    }
    return false;
}

static void parser_expect(Parser *parser, TokenType type) {
    if (parser->current_token.type != type) {
        fprintf(stderr, "Line %d: Expected token type %d, got %d\n", 
                parser->current_token.line, type, parser->current_token.type);
        exit(1);
    }
    parser_consume(parser);
}

static Type *create_type(DataType base_type) {
    Type *type = malloc(sizeof(Type));
    type->base_type = base_type;
    type->pointer_to = NULL;
    type->array_size = 0;
    
    switch (base_type) {
        case TYPE_INT:
            type->size = 4;
            break;
        case TYPE_CHAR:
            type->size = 1;
            break;
        case TYPE_FLOAT:
            type->size = 4;
            break;
        case TYPE_DOUBLE:
            type->size = 8;
            break;
        case TYPE_VOID:
            type->size = 0;
            break;
        case TYPE_POINTER:
            type->size = 8; // Assuming 64-bit pointers
            break;
        case TYPE_ARRAY:
            // Size will be set when array dimensions are known
            type->size = 0;
            break;
    }
    
    return type;
}

static Type *parse_type(Parser *parser) {
    Type *type = NULL;
    
    switch (parser->current_token.type) {
        case TOKEN_VOID:
            type = create_type(TYPE_VOID);
            parser_consume(parser);
            break;
        case TOKEN_INT:
            type = create_type(TYPE_INT);
            parser_consume(parser);
            break;
        case TOKEN_CHAR_KW:
            type = create_type(TYPE_CHAR);
            parser_consume(parser);
            break;
        case TOKEN_FLOAT:
            type = create_type(TYPE_FLOAT);
            parser_consume(parser);
            break;
        case TOKEN_DOUBLE:
            type = create_type(TYPE_DOUBLE);
            parser_consume(parser);
            break;
        default:
            fprintf(stderr, "Line %d: Expected type specifier\n", parser->current_token.line);
            exit(1);
    }
    
    // Handle pointers
    while (parser_match(parser, TOKEN_MULTIPLY)) {
        Type *pointer_type = create_type(TYPE_POINTER);
        pointer_type->pointer_to = type;
        type = pointer_type;
    }
    
    return type;
}

static Expression *parse_primary_expression(Parser *parser);
static Expression *parse_expression(Parser *parser);
static Statement *parse_statement(Parser *parser);
static Statement *parse_block(Parser *parser);

static Expression *parse_primary_expression(Parser *parser) {
    Expression *expr = malloc(sizeof(Expression));
    
    switch (parser->current_token.type) {
        case TOKEN_NUMBER_INT:
            expr->type = EXPR_LITERAL_INT;
            expr->data_type = create_type(TYPE_INT);
            expr->value.int_value = parser->current_token.value.int_value;
            parser_consume(parser);
            break;
        case TOKEN_NUMBER_FLOAT:
            expr->type = EXPR_LITERAL_FLOAT;
            expr->data_type = create_type(TYPE_FLOAT);
            expr->value.float_value = parser->current_token.value.float_value;
            parser_consume(parser);
            break;
        case TOKEN_CHAR:
            expr->type = EXPR_LITERAL_CHAR;
            expr->data_type = create_type(TYPE_CHAR);
            expr->value.char_value = parser->current_token.value.char_value;
            parser_consume(parser);
            break;
        case TOKEN_IDENTIFIER:
            if (parser->lexer->current == '(') {
                // Function call
                expr->type = EXPR_CALL;
                expr->value.call.function_name = strdup(parser->current_token.lexeme);
                parser_consume(parser);
                parser_expect(parser, TOKEN_LPAREN);
                
                // Parse arguments
                expr->value.call.arguments = malloc(10 * sizeof(Expression*)); // Arbitrary limit
                expr->value.call.argument_count = 0;
                
                if (parser->current_token.type != TOKEN_RPAREN) {
                    do {
                        expr->value.call.arguments[expr->value.call.argument_count++] = parse_expression(parser);
                    } while (parser_match(parser, TOKEN_COMMA));
                }
                
                parser_expect(parser, TOKEN_RPAREN);
                
                // Return type will be resolved during semantic analysis
                expr->data_type = create_type(TYPE_INT); // Placeholder
            } else {
                // Variable reference
                expr->type = EXPR_VARIABLE;
                expr->value.variable_name = strdup(parser->current_token.lexeme);
                parser_consume(parser);
                
                // Type will be resolved during semantic analysis
                expr->data_type = create_type(TYPE_INT); // Placeholder
            }
            break;
        case TOKEN_LPAREN:
            parser_consume(parser);
            expr = parse_expression(parser);
            parser_expect(parser, TOKEN_RPAREN);
            break;
        default:
            fprintf(stderr, "Line %d: Unexpected token in expression\n", parser->current_token.line);
            exit(1);
    }
    
    // Handle array subscript
    if (parser_match(parser, TOKEN_LBRACKET)) {
        Expression *array = expr;
        expr = malloc(sizeof(Expression));
        expr->type = EXPR_SUBSCRIPT;
        expr->value.subscript.array = array;
        expr->value.subscript.index = parse_expression(parser);
        parser_expect(parser, TOKEN_RBRACKET);
        
        // Type will be the element type of the array
        if (array->data_type->base_type == TYPE_ARRAY) {
            expr->data_type = array->data_type->pointer_to;
        } else if (array->data_type->base_type == TYPE_POINTER) {
            expr->data_type = array->data_type->pointer_to;
        } else {
            fprintf(stderr, "Line %d: Cannot index non-array type\n", parser->current_token.line);
            exit(1);
        }
    }
    
    return expr;
}

static Expression *parse_unary_expression(Parser *parser) {
    if (parser->current_token.type == TOKEN_PLUS ||
        parser->current_token.type == TOKEN_MINUS ||
        parser->current_token.type == TOKEN_NOT ||
        parser->current_token.type == TOKEN_BIT_NOT) {
        Expression *expr = malloc(sizeof(Expression));
        expr->type = EXPR_UNARY;
        expr->value.unary.operator = parser->current_token.type;
        parser_consume(parser);
        expr->value.unary.operand = parse_unary_expression(parser);
        
        // Determine result type based on operand type
        switch (expr->value.unary.operator) {
            case TOKEN_PLUS:
            case TOKEN_MINUS:
                expr->data_type = expr->value.unary.operand->data_type;
                break;
            case TOKEN_NOT:
                expr->data_type = create_type(TYPE_INT); // Boolean result
                break;
            case TOKEN_BIT_NOT:
                expr->data_type = expr->value.unary.operand->data_type;
                break;
            default:
                fprintf(stderr, "Unexpected unary operator\n");
                exit(1);
        }
        
        return expr;
    }
    
    return parse_primary_expression(parser);
}

static Expression *parse_multiplicative_expression(Parser *parser) {
    Expression *expr = parse_unary_expression(parser);
    
    while (parser->current_token.type == TOKEN_MULTIPLY ||
           parser->current_token.type == TOKEN_DIVIDE ||
           parser->current_token.type == TOKEN_MODULO) {
        TokenType op = parser->current_token.type;
        parser_consume(parser);
        
        Expression *right = parse_unary_expression(parser);
        
        Expression *binary = malloc(sizeof(Expression));
        binary->type = EXPR_BINARY;
        binary->value.binary.operator = op;
        binary->value.binary.left = expr;
        binary->value.binary.right = right;
        
        // Determine result type
        if (expr->data_type->base_type == TYPE_FLOAT || right->data_type->base_type == TYPE_FLOAT) {
            binary->data_type = create_type(TYPE_FLOAT);
        } else {
            binary->data_type = create_type(TYPE_INT);
        }
        
        expr = binary;
    }
    
    return expr;
}

static Expression *parse_additive_expression(Parser *parser) {
    Expression *expr = parse_multiplicative_expression(parser);
    
    while (parser->current_token.type == TOKEN_PLUS ||
           parser->current_token.type == TOKEN_MINUS) {
        TokenType op = parser->current_token.type;
        parser_consume(parser);
        
        Expression *right = parse_multiplicative_expression(parser);
        
        Expression *binary = malloc(sizeof(Expression));
        binary->type = EXPR_BINARY;
        binary->value.binary.operator = op;
        binary->value.binary.left = expr;
        binary->value.binary.right = right;
        
        // Determine result type
        if (expr->data_type->base_type == TYPE_FLOAT || right->data_type->base_type == TYPE_FLOAT) {
            binary->data_type = create_type(TYPE_FLOAT);
        } else {
            binary->data_type = create_type(TYPE_INT);
        }
        
        expr = binary;
    }
    
    return expr;
}

static Expression *parse_relational_expression(Parser *parser) {
    Expression *expr = parse_additive_expression(parser);
    
    while (parser->current_token.type == TOKEN_LT ||
           parser->current_token.type == TOKEN_LE ||
           parser->current_token.type == TOKEN_GT ||
           parser->current_token.type == TOKEN_GE) {
        TokenType op = parser->current_token.type;
        parser_consume(parser);
        
        Expression *right = parse_additive_expression(parser);
        
        Expression *binary = malloc(sizeof(Expression));
        binary->type = EXPR_BINARY;
        binary->value.binary.operator = op;
        binary->value.binary.left = expr;
        binary->value.binary.right = right;
        
        // Comparison results in boolean value (represented as int)
        binary->data_type = create_type(TYPE_INT);
        
        expr = binary;
    }
    
    return expr;
}

static Expression *parse_equality_expression(Parser *parser) {
    Expression *expr = parse_relational_expression(parser);
    
    while (parser->current_token.type == TOKEN_EQ ||
           parser->current_token.type == TOKEN_NEQ) {
        TokenType op = parser->current_token.type;
        parser_consume(parser);
        
        Expression *right = parse_relational_expression(parser);
        
        Expression *binary = malloc(sizeof(Expression));
        binary->type = EXPR_BINARY;
        binary->value.binary.operator = op;
        binary->value.binary.left = expr;
        binary->value.binary.right = right;
        
        // Comparison results in boolean value (represented as int)
        binary->data_type = create_type(TYPE_INT);
        
        expr = binary;
    }
    
    return expr;
}

static Expression *parse_logical_and_expression(Parser *parser) {
    Expression *expr = parse_equality_expression(parser);
    
    while (parser->current_token.type == TOKEN_AND) {
        parser_consume(parser);
        
        Expression *right = parse_equality_expression(parser);
        
        Expression *binary = malloc(sizeof(Expression));
        binary->type = EXPR_BINARY;
        binary->value.binary.operator = TOKEN_AND;
        binary->value.binary.left = expr;
        binary->value.binary.right = right;
        
        // Logical operations result in boolean value (represented as int)
        binary->data_type = create_type(TYPE_INT);
        
        expr = binary;
    }
    
    return expr;
}

static Expression *parse_logical_or_expression(Parser *parser) {
    Expression *expr = parse_logical_and_expression(parser);
    
    while (parser->current_token.type == TOKEN_OR) {
        parser_consume(parser);
        
        Expression *right = parse_logical_and_expression(parser);
        
        Expression *binary = malloc(sizeof(Expression));
        binary->type = EXPR_BINARY;
        binary->value.binary.operator = TOKEN_OR;
        binary->value.binary.left = expr;
        binary->value.binary.right = right;
        
        // Logical operations result in boolean value (represented as int)
        binary->data_type = create_type(TYPE_INT);
        
        expr = binary;
    }
    
    return expr;
}

static Expression *parse_assignment_expression(Parser *parser) {
    Expression *expr = parse_logical_or_expression(parser);
    
    if (parser->current_token.type == TOKEN_ASSIGN) {
        parser_consume(parser);
        
        Expression *value = parse_assignment_expression(parser);
        
        // Only variables and array elements can be assigned to
        if (expr->type == EXPR_VARIABLE || expr->type == EXPR_SUBSCRIPT) {
            Expression *assign = malloc(sizeof(Expression));
            assign->type = EXPR_ASSIGN;
            
            if (expr->type == EXPR_VARIABLE) {
                assign->value.assign.variable_name = strdup(expr->value.variable_name);
                free(expr->value.variable_name);
                free(expr);
            } else {
                // Convert subscript to equivalent assign operation
                // This is simplified; a real compiler would handle this differently
                assign->value.assign.variable_name = strdup("array_element");
                // In a real compiler, we'd keep the subscript information
            }
            
            assign->value.assign.value = value;
            assign->data_type = value->data_type;
            
            return assign;
        } else {
            fprintf(stderr, "Line %d: Invalid assignment target\n", parser->previous_token.line);
            exit(1);
        }
    }
    
    return expr;
}

static Expression *parse_expression(Parser *parser) {
    return parse_assignment_expression(parser);
}

static Statement *parse_expression_statement(Parser *parser) {
    Statement *stmt = malloc(sizeof(Statement));
    stmt->type = STMT_EXPRESSION;
    stmt->value.expression = parse_expression(parser);
    parser_expect(parser, TOKEN_SEMICOLON);
    return stmt;
}

static Statement *parse_return_statement(Parser *parser) {
    Statement *stmt = malloc(sizeof(Statement));
    stmt->type = STMT_RETURN;
    parser_consume(parser); // Consume 'return'
    
    if (parser->current_token.type != TOKEN_SEMICOLON) {
        stmt->value.expression = parse_expression(parser);
    } else {
        // Return void
        Expression *expr = malloc(sizeof(Expression));
        expr->type = EXPR_LITERAL_INT;
        expr->data_type = create_type(TYPE_VOID);
        expr->value.int_value = 0;
        stmt->value.expression = expr;
    }
    
    parser_expect(parser, TOKEN_SEMICOLON);
    return stmt;
}

static Statement *parse_if_statement(Parser *parser) {
    Statement *stmt = malloc(sizeof(Statement));
    stmt->type = STMT_IF;
    parser_consume(parser); // Consume 'if'
    
    parser_expect(parser, TOKEN_LPAREN);
    stmt->value.if_statement.condition = parse_expression(parser);
    parser_expect(parser, TOKEN_RPAREN);
    
    stmt->value.if_statement.then_branch = parse_statement(parser);
    
    if (parser_match(parser, TOKEN_ELSE)) {
        stmt->value.if_statement.else_branch = parse_statement(parser);
    } else {
        stmt->value.if_statement.else_branch = NULL;
    }
    
    return stmt;
}

static Statement *parse_while_statement(Parser *parser) {
    Statement *stmt = malloc(sizeof(Statement));
    stmt->type = STMT_WHILE;
    parser_consume(parser); // Consume 'while'
    
    parser_expect(parser, TOKEN_LPAREN);
    stmt->value.while_statement.condition = parse_expression(parser);
    parser_expect(parser, TOKEN_RPAREN);
    
    stmt->value.while_statement.body = parse_statement(parser);
    
    return stmt;
}

static Statement *parse_for_statement(Parser *parser) {
    Statement *stmt = malloc(sizeof(Statement));
    stmt->type = STMT_FOR;
    parser_consume(parser); // Consume 'for'
    
    parser_expect(parser, TOKEN_LPAREN);
    
    // Initializer
    if (parser->current_token.type != TOKEN_SEMICOLON) {
        stmt->value.for_statement.initializer = parse_expression(parser);
    } else {
        stmt->value.for_statement.initializer = NULL;
    }
    parser_expect(parser, TOKEN_SEMICOLON);
    
    // Condition
    if (parser->current_token.type != TOKEN_SEMICOLON) {
        stmt->value.for_statement.condition = parse_expression(parser);
    } else {
        // Default to true condition
        Expression *expr = malloc(sizeof(Expression));
        expr->type = EXPR_LITERAL_INT;
        expr->data_type = create_type(TYPE_INT);
        expr->value.int_value = 1;
        stmt->value.for_statement.condition = expr;
    }
    parser_expect(parser, TOKEN_SEMICOLON);
    
    // Increment
    if (parser->current_token.type != TOKEN_RPAREN) {
        stmt->value.for_statement.increment = parse_expression(parser);
    } else {
        stmt->value.for_statement.increment = NULL;
    }
    parser_expect(parser, TOKEN_RPAREN);
    
    stmt->value.for_statement.body = parse_statement(parser);
    
    return stmt;
}

static Statement *parse_block(Parser *parser) {
    Statement *stmt = malloc(sizeof(Statement));
    stmt->type = STMT_BLOCK;
    parser_expect(parser, TOKEN_LBRACE);
    
    // Parse statements in the block
    stmt->value.block.statements = malloc(100 * sizeof(Statement*)); // Arbitrary limit
    stmt->value.block.statement_count = 0;
    
    while (parser->current_token.type != TOKEN_RBRACE) {
        stmt->value.block.statements[stmt->value.block.statement_count++] = parse_statement(parser);
    }
    
    parser_expect(parser, TOKEN_RBRACE);
    return stmt;
}

static Statement *parse_declaration(Parser *parser) {
    Statement *stmt = malloc(sizeof(Statement));
    stmt->type = STMT_DECLARATION;
    stmt->value.declaration.type = parse_type(parser);
    
    parser_expect(parser, TOKEN_IDENTIFIER);
    stmt->value.declaration.name = strdup(parser->previous_token.lexeme);
    
    // Check for array declaration
    if (parser_match(parser, TOKEN_LBRACKET)) {
        if (parser->current_token.type == TOKEN_NUMBER_INT) {
            int size = parser->current_token.value.int_value;
            parser_consume(parser);
            
            Type *array_type = create_type(TYPE_ARRAY);
            array_type->pointer_to = stmt->value.declaration.type;
            array_type->array_size = size;
            array_type->size = size * stmt->value.declaration.type->size;
            
            stmt->value.declaration.type = array_type;
        } else {
            fprintf(stderr, "Line %d: Expected array size\n", parser->current_token.line);
            exit(1);
        }
        parser_expect(parser, TOKEN_RBRACKET);
    }
    
    // Check for initializer
    if (parser_match(parser, TOKEN_ASSIGN)) {
        stmt->value.declaration.initializer = parse_expression(parser);
    } else {
        stmt->value.declaration.initializer = NULL;
    }
    
    parser_expect(parser, TOKEN_SEMICOLON);
    return stmt;
}

static Statement *parse_statement(Parser *parser) {
    switch (parser->current_token.type) {
        case TOKEN_LBRACE:
            return parse_block(parser);
        case TOKEN_IF:
            return parse_if_statement(parser);
        case TOKEN_WHILE:
            return parse_while_statement(parser);
        case TOKEN_FOR:
            return parse_for_statement(parser);
        case TOKEN_RETURN:
            return parse_return_statement(parser);
        case TOKEN_INT:
        case TOKEN_CHAR_KW:
        case TOKEN_FLOAT:
        case TOKEN_DOUBLE:
        case TOKEN_VOID:
            return parse_declaration(parser);
        default:
            return parse_expression_statement(parser);
    }
}

static Function *parse_function(Parser *parser) {
    Function *function = malloc(sizeof(Function));
    
    // Parse return type
    function->return_type = parse_type(parser);
    
    // Parse function name
    parser_expect(parser, TOKEN_IDENTIFIER);
    function->name = strdup(parser->previous_token.lexeme);
    
    // Parse parameters
    parser_expect(parser, TOKEN_LPAREN);
    
    function->parameter_types = malloc(10 * sizeof(Type*)); // Arbitrary limit
    function->parameter_names = malloc(10 * sizeof(char*));
    function->parameter_count = 0;
    
    if (parser->current_token.type != TOKEN_RPAREN) {
        do {
            Type *param_type = parse_type(parser);
            parser_expect(parser, TOKEN_IDENTIFIER);
            
            function->parameter_types[function->parameter_count] = param_type;
            function->parameter_names[function->parameter_count] = strdup(parser->previous_token.lexeme);
            function->parameter_count++;
        } while (parser_match(parser, TOKEN_COMMA));
    }
    
    parser_expect(parser, TOKEN_RPAREN);
    
    // Parse function body
    function->body = parse_block(parser);
    
    return function;
}

Program *parse_program(Lexer *lexer) {
    Parser parser;
    parser_init(&parser, lexer);
    
    Program *program = malloc(sizeof(Program));
    program->functions = malloc(100 * sizeof(Function*)); // Arbitrary limit
    program->function_count = 0;
    program->function_capacity = 100;
    
    while (parser.current_token.type != TOKEN_EOF) {
        Function *function = parse_function(&parser);
        program->functions[program->function_count++] = function;
    }
    
    return program;
}

/* ========== Symbol Table ========== */

typedef struct {
    char *name;
    Type *type;
    int offset;  // Stack offset or register number
} Symbol;

typedef struct SymbolTable {
    Symbol **symbols;
    int symbol_count;
    int capacity;
    struct SymbolTable *parent;
} SymbolTable;

SymbolTable *symbol_table_create(SymbolTable *parent) {
    SymbolTable *table = malloc(sizeof(SymbolTable));
    table->symbols = malloc(100 * sizeof(Symbol*)); // Arbitrary limit
    table->symbol_count = 0;
    table->capacity = 100;
    table->parent = parent;
    return table;
}

void symbol_table_add(SymbolTable *table, const char *name, Type *type, int offset) {
    Symbol *symbol = malloc(sizeof(Symbol));
    symbol->name = strdup(name);
    symbol->type = type;
    symbol->offset = offset;
    
    table->symbols[table->symbol_count++] = symbol;
}

Symbol *symbol_table_lookup(SymbolTable *table, const char *name) {
    // Check current scope
    for (int i = 0; i < table->symbol_count; i++) {
        if (strcmp(table->symbols[i]->name, name) == 0) {
            return table->symbols[i];
        }
    }
    
    // Check parent scopes
    if (table->parent != NULL) {
        return symbol_table_lookup(table->parent, name);
    }
    
    return NULL;
}

void symbol_table_free(SymbolTable *table) {
    for (int i = 0; i < table->symbol_count; i++) {
        free(table->symbols[i]->name);
        free(table->symbols[i]);
    }
    free(table->symbols);
    free(table);
}

/* ========== COIL Code Generation ========== */

typedef struct {
    FILE *output;
    SymbolTable *current_scope;
    int next_label;
    int stack_offset;
} CodeGenerator;

void generator_init(CodeGenerator *generator, FILE *output) {
    generator->output = output;
    generator->current_scope = symbol_table_create(NULL);
    generator->next_label = 0;
    generator->stack_offset = 0;
}

int generator_next_label(CodeGenerator *generator) {
    return generator->next_label++;
}

// Binary format utility functions
void write_u8(FILE *output, uint8_t value) {
    fwrite(&value, sizeof(uint8_t), 1, output);
}

void write_u16(FILE *output, uint16_t value) {
    fwrite(&value, sizeof(uint16_t), 1, output);
}

void write_u32(FILE *output, uint32_t value) {
    fwrite(&value, sizeof(uint32_t), 1, output);
}

void write_i32(FILE *output, int32_t value) {
    fwrite(&value, sizeof(int32_t), 1, output);
}

void write_u64(FILE *output, uint64_t value) {
    fwrite(&value, sizeof(uint64_t), 1, output);
}

void write_f32(FILE *output, float value) {
    fwrite(&value, sizeof(float), 1, output);
}

void write_f64(FILE *output, double value) {
    fwrite(&value, sizeof(double), 1, output);
}

void write_string(FILE *output, const char *str) {
    uint8_t len = strlen(str);
    write_u8(output, len);
    fwrite(str, sizeof(char), len, output);
}

// COIL binary instruction format helpers
// These helpers ensure we're following the format exactly

// Write a complete instruction with operands
void emit_instruction(FILE *output, uint8_t opcode, uint8_t qualifier, uint8_t operand_count) {
    write_u8(output, opcode);
    write_u8(output, qualifier);
    write_u8(output, operand_count);
}

// Write a register operand
void emit_register_operand(FILE *output, uint8_t type, uint8_t width, uint8_t reg_num) {
    write_u8(output, 0x03); // OPQUAL_REG
    write_u8(output, type); // Type (e.g., COIL_TYPE_INT)
    write_u8(output, width); // Width in bytes
    write_u8(output, reg_num); // Register number
}

// Write an immediate operand (32-bit)
void emit_immediate_operand_i32(FILE *output, uint8_t type, int32_t value) {
    write_u8(output, 0x01); // OPQUAL_IMM
    write_u8(output, type); // Type (e.g., COIL_TYPE_INT)
    write_u8(output, 0x04); // 32-bit
    write_i32(output, value); // Value
}

// Write a label operand
void emit_label_operand(FILE *output, int label) {
    write_u8(output, 0x05); // OPQUAL_LBL
    write_u8(output, 0xF0); // COIL_TYPE_VOID
    write_u8(output, 0x00); // No additional type info
    write_i32(output, label); // Label identifier
}

// Write a symbol operand
void emit_symbol_operand(FILE *output, const char *name) {
    write_u8(output, 0x07); // OPQUAL_SYM
    write_u8(output, 0xF0); // COIL_TYPE_VOID
    write_u8(output, 0x00); // No additional type info
    write_string(output, name); // Symbol name
}

// Write a memory operand
void emit_memory_operand(FILE *output, uint8_t type, uint8_t width, uint8_t base_reg, int32_t offset) {
    write_u8(output, 0x04); // OPQUAL_MEM
    write_u8(output, type); // Type (e.g., COIL_TYPE_INT)
    write_u8(output, width); // Width in bytes
    write_u8(output, base_reg); // Base register
    write_i32(output, offset); // Offset
}

// COIL instruction emitters using the new helper functions
void emit_nop(FILE *output) {
    emit_instruction(output, 0x00, 0x00, 0x00); // NOP with no operands
}

void emit_mov(FILE *output, uint8_t dest_reg, uint8_t src_reg) {
    emit_instruction(output, 0x40, 0x00, 0x02); // MOV with 2 operands
    emit_register_operand(output, 0x00, 0x04, dest_reg); // Destination register (32-bit int)
    emit_register_operand(output, 0x00, 0x04, src_reg); // Source register (32-bit int)
}

void emit_movi(FILE *output, uint8_t dest_reg, int32_t value) {
    emit_instruction(output, 0x45, 0x00, 0x02); // MOVI with 2 operands
    emit_register_operand(output, 0x00, 0x04, dest_reg); // Destination register (32-bit int)
    emit_immediate_operand_i32(output, 0x00, value); // Immediate value (32-bit int)
}

void emit_add(FILE *output, uint8_t dest_reg, uint8_t src1_reg, uint8_t src2_reg) {
    emit_instruction(output, 0x10, 0x00, 0x03); // ADD with 3 operands
    emit_register_operand(output, 0x00, 0x04, dest_reg); // Destination register (32-bit int)
    emit_register_operand(output, 0x00, 0x04, src1_reg); // Source1 register (32-bit int)
    emit_register_operand(output, 0x00, 0x04, src2_reg); // Source2 register (32-bit int)
}

void emit_sub(FILE *output, uint8_t dest_reg, uint8_t src1_reg, uint8_t src2_reg) {
    emit_instruction(output, 0x11, 0x00, 0x03); // SUB with 3 operands
    emit_register_operand(output, 0x00, 0x04, dest_reg); // Destination register (32-bit int)
    emit_register_operand(output, 0x00, 0x04, src1_reg); // Source1 register (32-bit int)
    emit_register_operand(output, 0x00, 0x04, src2_reg); // Source2 register (32-bit int)
}

void emit_mul(FILE *output, uint8_t dest_reg, uint8_t src1_reg, uint8_t src2_reg) {
    emit_instruction(output, 0x12, 0x00, 0x03); // MUL with 3 operands
    emit_register_operand(output, 0x00, 0x04, dest_reg); // Destination register (32-bit int)
    emit_register_operand(output, 0x00, 0x04, src1_reg); // Source1 register (32-bit int)
    emit_register_operand(output, 0x00, 0x04, src2_reg); // Source2 register (32-bit int)
}

void emit_div(FILE *output, uint8_t dest_reg, uint8_t src1_reg, uint8_t src2_reg) {
    emit_instruction(output, 0x13, 0x00, 0x03); // DIV with 3 operands
    emit_register_operand(output, 0x00, 0x04, dest_reg); // Destination register (32-bit int)
    emit_register_operand(output, 0x00, 0x04, src1_reg); // Source1 register (32-bit int)
    emit_register_operand(output, 0x00, 0x04, src2_reg); // Source2 register (32-bit int)
}

void emit_cmp(FILE *output, uint8_t src1_reg, uint8_t src2_reg) {
    emit_instruction(output, 0x30, 0x00, 0x02); // CMP with 2 operands
    emit_register_operand(output, 0x00, 0x04, src1_reg); // Source1 register (32-bit int)
    emit_register_operand(output, 0x00, 0x04, src2_reg); // Source2 register (32-bit int)
}

void emit_jmp(FILE *output, int label) {
    emit_instruction(output, 0x02, 0x00, 0x01); // BR (unconditional branch) with 1 operand
    emit_label_operand(output, label); // Label operand
}

void emit_jcc(FILE *output, uint8_t condition, int label) {
    emit_instruction(output, 0x03, condition, 0x01); // BRC (conditional branch) with 1 operand
    emit_label_operand(output, label); // Label operand
}

void emit_call(FILE *output, const char *function_name) {
    emit_instruction(output, 0x04, 0x00, 0x01); // CALL with 1 operand
    emit_symbol_operand(output, function_name); // Function name operand
}

void emit_ret(FILE *output) {
    emit_instruction(output, 0x05, 0x00, 0x00); // RET with no operands
}

void emit_load(FILE *output, uint8_t dest_reg, uint8_t addr_reg, int offset) {
    emit_instruction(output, 0x41, 0x00, 0x02); // LOAD with 2 operands
    emit_register_operand(output, 0x00, 0x04, dest_reg); // Destination register (32-bit int)
    emit_memory_operand(output, 0x00, 0x04, addr_reg, offset); // Memory address
}

void emit_store(FILE *output, uint8_t src_reg, uint8_t addr_reg, int offset) {
    emit_instruction(output, 0x42, 0x00, 0x02); // STORE with 2 operands
    emit_register_operand(output, 0x00, 0x04, src_reg); // Source register (32-bit int)
    emit_memory_operand(output, 0x00, 0x04, addr_reg, offset); // Memory address
}

void emit_push(FILE *output, uint8_t src_reg) {
    emit_instruction(output, 0x50, 0x00, 0x01); // PUSH with 1 operand
    emit_register_operand(output, 0x00, 0x04, src_reg); // Source register (32-bit int)
}

void emit_pop(FILE *output, uint8_t dest_reg) {
    emit_instruction(output, 0x51, 0x00, 0x01); // POP with 1 operand
    emit_register_operand(output, 0x00, 0x04, dest_reg); // Destination register (32-bit int)
}

void emit_enter(FILE *output, int frame_size) {
    emit_instruction(output, 0xC0, 0x00, 0x01); // ENTER with 1 operand
    emit_immediate_operand_i32(output, 0x00, frame_size); // Frame size
}

void emit_leave(FILE *output) {
    emit_instruction(output, 0xC1, 0x00, 0x00); // LEAVE with no operands
}

void emit_label(FILE *output, int label) {
    emit_instruction(output, 0x01, 0x00, 0x01); // SYMB with 1 operand
    emit_label_operand(output, label); // Label operand
}

// Code generation for expressions
int generate_expression(CodeGenerator *generator, Expression *expr, int dest_reg) {
    FILE *output = generator->output;
    
    switch (expr->type) {
        case EXPR_LITERAL_INT:
            emit_movi(output, dest_reg, expr->value.int_value);
            return dest_reg;
        
        case EXPR_BINARY: {
            int left_result = generate_expression(generator, expr->value.binary.left, dest_reg);
            int right_result = generate_expression(generator, expr->value.binary.right, dest_reg + 1);
            
            switch (expr->value.binary.operator) {
                case TOKEN_PLUS:
                    emit_add(output, dest_reg, left_result, right_result);
                    break;
                case TOKEN_MINUS:
                    emit_sub(output, dest_reg, left_result, right_result);
                    break;
                case TOKEN_MULTIPLY:
                    emit_mul(output, dest_reg, left_result, right_result);
                    break;
                case TOKEN_DIVIDE:
                    emit_div(output, dest_reg, left_result, right_result);
                    break;
                case TOKEN_EQ:
                case TOKEN_NEQ:
                case TOKEN_LT:
                case TOKEN_LE:
                case TOKEN_GT:
                case TOKEN_GE:
                    emit_cmp(output, left_result, right_result);
                    // TODO: Set dest_reg based on condition
                    break;
                default:
                    fprintf(stderr, "Unsupported binary operator\n");
                    exit(1);
            }
            
            return dest_reg;
        }
        
        case EXPR_VARIABLE: {
            Symbol *symbol = symbol_table_lookup(generator->current_scope, expr->value.variable_name);
            if (symbol == NULL) {
                fprintf(stderr, "Undefined variable: %s\n", expr->value.variable_name);
                exit(1);
            }
            
            // For simplicity, we use stack-based variables
            emit_load(output, dest_reg, 7, symbol->offset); // RQ7 is base pointer
            return dest_reg;
        }
        
        case EXPR_ASSIGN: {
            Symbol *symbol = symbol_table_lookup(generator->current_scope, expr->value.assign.variable_name);
            if (symbol == NULL) {
                fprintf(stderr, "Undefined variable: %s\n", expr->value.assign.variable_name);
                exit(1);
            }
            
            int value_reg = generate_expression(generator, expr->value.assign.value, dest_reg);
            emit_store(output, value_reg, 7, symbol->offset); // RQ7 is base pointer
            return value_reg;
        }
        
        case EXPR_CALL: {
            // Push arguments (simplified; real ABI would be more complex)
            for (int i = expr->value.call.argument_count - 1; i >= 0; i--) {
                int arg_reg = generate_expression(generator, expr->value.call.arguments[i], dest_reg + i);
                emit_push(output, arg_reg);
            }
            
            emit_call(output, expr->value.call.function_name);
            
            // Return value is in RQ0, move to dest_reg if needed
            if (dest_reg != 0) {
                emit_mov(output, dest_reg, 0);
            }
            
            return dest_reg;
        }
        
        default:
            fprintf(stderr, "Unsupported expression type\n");
            exit(1);
    }
}

// Code generation for statements
void generate_statement(CodeGenerator *generator, Statement *stmt) {
    FILE *output = generator->output;
    
    switch (stmt->type) {
        case STMT_EXPRESSION:
            generate_expression(generator, stmt->value.expression, 0);
            break;
        
        case STMT_RETURN:
            if (stmt->value.expression != NULL) {
                generate_expression(generator, stmt->value.expression, 0); // Return value in RQ0
            }
            emit_leave(output);
            emit_ret(output);
            break;
        
        case STMT_IF: {
            int else_label = generator_next_label(generator);
            int end_label = generator_next_label(generator);
            
            // Generate condition
            generate_expression(generator, stmt->value.if_statement.condition, 0);
            emit_movi(output, 1, 0); // Load 0 into RQ1
            emit_cmp(output, 0, 1); // Compare condition to 0
            emit_jcc(output, 0x01, else_label); // Jump to else if equal (condition is false)
            
            // Generate then branch
            generate_statement(generator, stmt->value.if_statement.then_branch);
            emit_jmp(output, end_label);
            
            // Generate else branch
            emit_label(output, else_label);
            if (stmt->value.if_statement.else_branch != NULL) {
                generate_statement(generator, stmt->value.if_statement.else_branch);
            }
            
            emit_label(output, end_label);
            break;
        }
        
        case STMT_WHILE: {
            int start_label = generator_next_label(generator);
            int end_label = generator_next_label(generator);
            
            emit_label(output, start_label);
            
            // Generate condition
            generate_expression(generator, stmt->value.while_statement.condition, 0);
            emit_movi(output, 1, 0); // Load 0 into RQ1
            emit_cmp(output, 0, 1); // Compare condition to 0
            emit_jcc(output, 0x01, end_label); // Jump to end if equal (condition is false)
            
            // Generate body
            generate_statement(generator, stmt->value.while_statement.body);
            emit_jmp(output, start_label);
            
            emit_label(output, end_label);
            break;
        }
        
        case STMT_FOR: {
            int start_label = generator_next_label(generator);
            int end_label = generator_next_label(generator);
            
            // Generate initializer
            if (stmt->value.for_statement.initializer != NULL) {
                generate_expression(generator, stmt->value.for_statement.initializer, 0);
            }
            
            emit_label(output, start_label);
            
            // Generate condition
            if (stmt->value.for_statement.condition != NULL) {
                generate_expression(generator, stmt->value.for_statement.condition, 0);
                emit_movi(output, 1, 0); // Load 0 into RQ1
                emit_cmp(output, 0, 1); // Compare condition to 0
                emit_jcc(output, 0x01, end_label); // Jump to end if equal (condition is false)
            }
            
            // Generate body
            generate_statement(generator, stmt->value.for_statement.body);
            
            // Generate increment
            if (stmt->value.for_statement.increment != NULL) {
                generate_expression(generator, stmt->value.for_statement.increment, 0);
            }
            
            emit_jmp(output, start_label);
            
            emit_label(output, end_label);
            break;
        }
        
        case STMT_BLOCK: {
            // Create a new scope for the block
            SymbolTable *old_scope = generator->current_scope;
            generator->current_scope = symbol_table_create(old_scope);
            
            // Generate code for each statement in the block
            for (int i = 0; i < stmt->value.block.statement_count; i++) {
                generate_statement(generator, stmt->value.block.statements[i]);
            }
            
            // Restore the old scope
            generator->current_scope = old_scope;
            break;
        }
        
        case STMT_DECLARATION: {
            // Allocate space on the stack for the variable
            generator->stack_offset -= stmt->value.declaration.type->size;
            int offset = generator->stack_offset;
            
            // Add the variable to the symbol table
            symbol_table_add(generator->current_scope, stmt->value.declaration.name, 
                             stmt->value.declaration.type, offset);
            
            // Initialize the variable if needed
            if (stmt->value.declaration.initializer != NULL) {
                int reg = generate_expression(generator, stmt->value.declaration.initializer, 0);
                emit_store(output, reg, 7, offset); // RQ7 is base pointer
            }
            
            break;
        }
        
        default:
            fprintf(stderr, "Unsupported statement type\n");
            exit(1);
    }
}

// Code generation for function declarations
void generate_function(CodeGenerator *generator, Function *function) {
    FILE *output = generator->output;
    
    // Reset stack offset for the new function
    generator->stack_offset = 0;
    
    // Create a new scope for the function
    SymbolTable *old_scope = generator->current_scope;
    generator->current_scope = symbol_table_create(old_scope);
    
    // Add parameters to the symbol table
    for (int i = 0; i < function->parameter_count; i++) {
        // Parameters are at positive offsets from the base pointer
        int offset = (i + 1) * 4; // Assuming 32-bit parameters
        symbol_table_add(generator->current_scope, function->parameter_names[i], 
                         function->parameter_types[i], offset);
    }
    
    // Begin function
    // Use the SYMB instruction to create a function symbol
    write_u8(output, 0x01); // SYMB opcode
    write_u8(output, 0x00); // qualifier
    write_u8(output, 0x01); // 1 operand
    
    // Function name operand
    write_u8(output, 0x07); // OPQUAL_SYM
    write_u8(output, 0xF0); // COIL_TYPE_VOID
    write_u8(output, 0x00); // No type info
    write_string(output, function->name); // Function name
    
    // Setup stack frame
    emit_enter(output, 64); // Arbitrary frame size, should be calculated based on variables
    
    // Generate code for function body
    generate_statement(generator, function->body);
    
    // Add implicit return if the function doesn't already have one
    if (function->body->type == STMT_BLOCK && 
        (function->body->value.block.statement_count == 0 || 
         function->body->value.block.statements[function->body->value.block.statement_count - 1]->type != STMT_RETURN)) {
        emit_leave(output);
        emit_ret(output);
    }
    
    // Restore the old scope
    generator->current_scope = old_scope;
}

// Constants for COF file structure
#define COF_HEADER_SIZE 32
#define COF_SECTION_HEADER_SIZE 36

// Section types
#define COF_SECTION_NULL 0
#define COF_SECTION_CODE 1
#define COF_SECTION_DATA 2
#define COF_SECTION_BSS 3

// Section flags
#define COF_SEC_FLAG_WRITE 0x01
#define COF_SEC_FLAG_EXEC 0x02
#define COF_SEC_FLAG_ALLOC 0x04

// Generate COIL Object Format (COF) header and section headers
void generate_cof_header(FILE *output, Program *program) {
    // First pass - just write placeholders for everything
    // We'll come back and update them later
    
    // Remember starting position
    long start_pos = ftell(output);
    
    // Calculate offsets
    uint32_t header_offset = 0;
    uint32_t section_header_offset = COF_HEADER_SIZE;
    uint32_t code_section_offset = COF_HEADER_SIZE + COF_SECTION_HEADER_SIZE;
    
    // --- Main COF Header (32 bytes) ---
    fseek(output, header_offset, SEEK_SET);
    
    // Magic number: 'COIL'
    write_u32(output, 0x434F494C);
    
    // Version: 1.0.0
    write_u8(output, 1);   // Major
    write_u8(output, 0);   // Minor
    write_u8(output, 0);   // Patch
    write_u8(output, 0x01); // Flags: COF_FLAG_EXECUTABLE
    
    // Architecture target: x86-64
    write_u16(output, 0x0002);
    
    // Section count: 1 (just code for now)
    write_u16(output, 1);
    
    // Entrypoint: we'll update this after finding main()
    write_u32(output, 0);
    
    // String table: none for now
    write_u32(output, 0);  // Offset
    write_u32(output, 0);  // Size
    
    // Symbol table: none for now
    write_u32(output, 0);  // Offset
    write_u32(output, 0);  // Size
    
    // Padding to reach 32 bytes
    for (int i = 0; i < 8; i++) {
        write_u8(output, 0);
    }
    
    // --- Section Header (36 bytes) ---
    fseek(output, section_header_offset, SEEK_SET);
    
    // Section name: no name for now
    write_u32(output, 0);
    
    // Section type: CODE
    write_u32(output, COF_SECTION_CODE);
    
    // Section flags: EXEC | ALLOC
    write_u32(output, COF_SEC_FLAG_EXEC | COF_SEC_FLAG_ALLOC);
    
    // Section offset: starts right after section header
    write_u32(output, code_section_offset);
    
    // Section size: placeholder, we'll update this later
    write_u32(output, 0);
    
    // No link, info, or specific alignment requirements
    write_u32(output, 0);  // Link
    write_u32(output, 0);  // Info
    write_u32(output, 4);  // Alignment (4-byte)
    write_u32(output, 0);  // Entry size (not a table)
    
    // Move to the start of the code section
    fseek(output, code_section_offset, SEEK_SET);
}

// Generate the entire COIL program
void generate_program(Program *program, const char *output_file) {
    FILE *output = fopen(output_file, "wb");
    if (output == NULL) {
        fprintf(stderr, "Failed to open output file: %s\n", output_file);
        exit(1);
    }
    
    // Write initial headers with placeholders
    generate_cof_header(output, program);
    
    // Remember where the code section starts
    long code_start_pos = ftell(output);
    
    // Generate code
    CodeGenerator generator;
    generator_init(&generator, output);
    
    // Generate code for each function
    for (int i = 0; i < program->function_count; i++) {
        generate_function(&generator, program->functions[i]);
    }
    
    // End position after all code
    long code_end_pos = ftell(output);
    uint32_t code_size = (uint32_t)(code_end_pos - code_start_pos);
    
    // Find entrypoint (main function)
    uint32_t entrypoint = 0;
    for (int i = 0; i < program->function_count; i++) {
        if (strcmp(program->functions[i]->name, "main") == 0) {
            // Main is at the code start + its offset in the generated code
            // For simplicity in this demo, we'll just use the start of the code section
            entrypoint = COF_HEADER_SIZE + COF_SECTION_HEADER_SIZE;
            break;
        }
    }
    
    // Update section size
    fseek(output, COF_HEADER_SIZE + 16, SEEK_SET); // Size field in section header
    write_u32(output, code_size);
    
    // Update entrypoint
    fseek(output, 16, SEEK_SET); // Entrypoint field in COF header
    write_u32(output, entrypoint);
    
    // Done
    fclose(output);
    
    printf("Generated COIL binary: %s (%lu bytes)\n", 
           output_file, (unsigned long)(code_end_pos));
    printf("- Header size: %d bytes\n", COF_HEADER_SIZE);
    printf("- Section header size: %d bytes\n", COF_SECTION_HEADER_SIZE);
    printf("- Code size: %u bytes\n", code_size);
    printf("- Entrypoint: 0x%08X\n", entrypoint);
}

/* ========== Main ========== */

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input.c> <output.cof>\n", argv[0]);
        return 1;
    }
    
    // Read input file
    FILE *input_file = fopen(argv[1], "r");
    if (input_file == NULL) {
        fprintf(stderr, "Failed to open input file: %s\n", argv[1]);
        return 1;
    }
    
    // Get file size
    fseek(input_file, 0, SEEK_END);
    long file_size = ftell(input_file);
    fseek(input_file, 0, SEEK_SET);
    
    // Read file contents
    char *source = malloc(file_size + 1);
    size_t read_size = fread(source, 1, file_size, input_file);
    source[read_size] = '\0';
    fclose(input_file);
    
    // Initialize lexer
    Lexer lexer;
    lexer_init(&lexer, source);
    
    // Parse the program
    Program *program = parse_program(&lexer);
    
    // Generate COIL code
    generate_program(program, argv[2]);
    
    // Clean up
    free(source);
    
    printf("Compilation successful: %s -> %s\n", argv[1], argv[2]);
    return 0;
}