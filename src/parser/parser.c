/**
 * Enhanced Parser Implementation for the COIL C Compiler
 * 
 * This parser builds an Abstract Syntax Tree (AST) from C code tokens
 * with robust error handling and type tracking.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "token.h"
#include "ast.h"
#include "type.h"

/**
 * Initialize a parser with a lexer
 */
void parser_init(Parser *parser, Lexer *lexer) {
    parser->lexer = lexer;
    parser->current_token = lexer_next_token(lexer);
    
    // Initialize previous_token to prevent freeing uninitialized memory
    parser->previous_token.type = TOKEN_EOF;
    parser->previous_token.lexeme = NULL;
    parser->previous_token.line = 0;
}

/**
 * Consume the current token and move to the next one
 */
static void parser_consume(Parser *parser) {
    token_free(parser->previous_token);
    parser->previous_token = parser->current_token;
    parser->current_token = lexer_next_token(parser->lexer);
}

/**
 * Check if the current token has the expected type and consume it if so
 * @return true if token matched, false otherwise
 */
static bool parser_match(Parser *parser, TokenType type) {
    if (parser->current_token.type == type) {
        parser_consume(parser);
        return true;
    }
    return false;
}

/**
 * Expect the current token to have the specified type, error if not
 * Consumes the token if it matches
 */
static void parser_expect(Parser *parser, TokenType type) {
    if (parser->current_token.type != type) {
        fprintf(stderr, "Line %d: Expected token type %d, got %d (%s)\n", 
                parser->current_token.line, type, parser->current_token.type,
                parser->current_token.lexeme);
        exit(1);
    }
    parser_consume(parser);
}

/**
 * Parse a type specifier (like int, char, float, double)
 * @return The parsed type structure
 */
Type *parse_type(Parser *parser) {
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
            fprintf(stderr, "Line %d: Expected type specifier, got %s\n", 
                    parser->current_token.line, parser->current_token.lexeme);
            exit(1);
    }
    
    // Handle pointers (e.g., int* x)
    while (parser_match(parser, TOKEN_MULTIPLY)) {
        type = create_pointer_type(type);
    }
    
    return type;
}

// Forward declarations for recursive parsing
static Expression *parse_expression(Parser *parser);
static Statement *parse_statement(Parser *parser);
static Statement *parse_block(Parser *parser);

/**
 * Parse a primary expression (literal, variable, or parenthesized)
 */
static Expression *parse_primary_expression(Parser *parser) {
    Expression *expr = malloc(sizeof(Expression));
    if (expr == NULL) {
        fprintf(stderr, "Memory allocation failed for expression\n");
        exit(1);
    }
    
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
            
        case TOKEN_IDENTIFIER: {
            const char *identifier = parser->current_token.lexeme;
            parser_consume(parser);
            
            // Check if it's a function call
            if (parser_match(parser, TOKEN_LPAREN)) {
                expr->type = EXPR_CALL;
                expr->value.call.function_name = strdup(identifier);
                
                // Parse arguments
                expr->value.call.arguments = malloc(10 * sizeof(Expression*)); // Initial limit
                if (expr->value.call.arguments == NULL) {
                    fprintf(stderr, "Memory allocation failed for function arguments\n");
                    exit(1);
                }
                expr->value.call.argument_count = 0;
                
                if (parser->current_token.type != TOKEN_RPAREN) {
                    do {
                        if (expr->value.call.argument_count >= 10) {
                            // Expand the arguments array if needed
                            int new_capacity = expr->value.call.argument_count * 2;
                            Expression **new_args = realloc(expr->value.call.arguments, 
                                                           new_capacity * sizeof(Expression*));
                            if (new_args == NULL) {
                                fprintf(stderr, "Memory reallocation failed for function arguments\n");
                                exit(1);
                            }
                            expr->value.call.arguments = new_args;
                        }
                        
                        expr->value.call.arguments[expr->value.call.argument_count++] = 
                            parse_expression(parser);
                    } while (parser_match(parser, TOKEN_COMMA));
                }
                
                parser_expect(parser, TOKEN_RPAREN);
                
                // Set return type - will be resolved during semantic analysis
                // For now, assume int
                expr->data_type = create_type(TYPE_INT);
            } else {
                // Variable reference
                expr->type = EXPR_VARIABLE;
                expr->value.variable_name = strdup(identifier);
                
                // Type will be resolved during semantic analysis
                // For now, assume int
                expr->data_type = create_type(TYPE_INT);
            }
            break;
        }
            
        case TOKEN_LPAREN:
            parser_consume(parser);
            free(expr); // We don't need this allocation since we'll use the one from parse_expression
            expr = parse_expression(parser);
            parser_expect(parser, TOKEN_RPAREN);
            break;
            
        default:
            fprintf(stderr, "Line %d: Unexpected token in expression: %s\n", 
                    parser->current_token.line, parser->current_token.lexeme);
            exit(1);
    }
    
    // Handle array subscript (e.g., a[i])
    if (parser_match(parser, TOKEN_LBRACKET)) {
        Expression *array = expr;
        expr = malloc(sizeof(Expression));
        if (expr == NULL) {
            fprintf(stderr, "Memory allocation failed for expression\n");
            exit(1);
        }
        
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

/**
 * Parse a unary expression (e.g., -x, !x, ~x)
 */
static Expression *parse_unary_expression(Parser *parser) {
    if (parser->current_token.type == TOKEN_PLUS ||
        parser->current_token.type == TOKEN_MINUS ||
        parser->current_token.type == TOKEN_NOT ||
        parser->current_token.type == TOKEN_BIT_NOT) {
        
        Expression *expr = malloc(sizeof(Expression));
        if (expr == NULL) {
            fprintf(stderr, "Memory allocation failed for expression\n");
            exit(1);
        }
        
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

/**
 * Parse a multiplicative expression (*, /, %)
 */
static Expression *parse_multiplicative_expression(Parser *parser) {
    Expression *expr = parse_unary_expression(parser);
    
    while (parser->current_token.type == TOKEN_MULTIPLY ||
            parser->current_token.type == TOKEN_DIVIDE ||
            parser->current_token.type == TOKEN_MODULO) {
        
        TokenType op = parser->current_token.type;
        parser_consume(parser);
        
        Expression *right = parse_unary_expression(parser);
        
        Expression *binary = malloc(sizeof(Expression));
        if (binary == NULL) {
            fprintf(stderr, "Memory allocation failed for expression\n");
            exit(1);
        }
        
        binary->type = EXPR_BINARY;
        binary->value.binary.operator = op;
        binary->value.binary.left = expr;
        binary->value.binary.right = right;
        
        // Determine result type - float if either operand is float, otherwise int
        if (expr->data_type->base_type == TYPE_FLOAT || right->data_type->base_type == TYPE_FLOAT ||
            expr->data_type->base_type == TYPE_DOUBLE || right->data_type->base_type == TYPE_DOUBLE) {
            binary->data_type = create_type(TYPE_FLOAT);
        } else {
            binary->data_type = create_type(TYPE_INT);
        }
        
        expr = binary;
    }
    
    return expr;
}

/**
 * Parse an additive expression (+, -)
 */
static Expression *parse_additive_expression(Parser *parser) {
    Expression *expr = parse_multiplicative_expression(parser);
    
    while (parser->current_token.type == TOKEN_PLUS ||
            parser->current_token.type == TOKEN_MINUS) {
        
        TokenType op = parser->current_token.type;
        parser_consume(parser);
        
        Expression *right = parse_multiplicative_expression(parser);
        
        Expression *binary = malloc(sizeof(Expression));
        if (binary == NULL) {
            fprintf(stderr, "Memory allocation failed for expression\n");
            exit(1);
        }
        
        binary->type = EXPR_BINARY;
        binary->value.binary.operator = op;
        binary->value.binary.left = expr;
        binary->value.binary.right = right;
        
        // Determine result type - float if either operand is float, otherwise int
        if (expr->data_type->base_type == TYPE_FLOAT || right->data_type->base_type == TYPE_FLOAT ||
            expr->data_type->base_type == TYPE_DOUBLE || right->data_type->base_type == TYPE_DOUBLE) {
            binary->data_type = create_type(TYPE_FLOAT);
        } else {
            binary->data_type = create_type(TYPE_INT);
        }
        
        expr = binary;
    }
    
    return expr;
}

/**
 * Parse a relational expression (<, <=, >, >=)
 */
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
        if (binary == NULL) {
            fprintf(stderr, "Memory allocation failed for expression\n");
            exit(1);
        }
        
        binary->type = EXPR_BINARY;
        binary->value.binary.operator = op;
        binary->value.binary.left = expr;
        binary->value.binary.right = right;
        
        // Comparison always results in boolean (represented as int)
        binary->data_type = create_type(TYPE_INT);
        
        expr = binary;
    }
    
    return expr;
}

/**
 * Parse an equality expression (==, !=)
 */
static Expression *parse_equality_expression(Parser *parser) {
    Expression *expr = parse_relational_expression(parser);
    
    while (parser->current_token.type == TOKEN_EQ ||
            parser->current_token.type == TOKEN_NEQ) {
        
        TokenType op = parser->current_token.type;
        parser_consume(parser);
        
        Expression *right = parse_relational_expression(parser);
        
        Expression *binary = malloc(sizeof(Expression));
        if (binary == NULL) {
            fprintf(stderr, "Memory allocation failed for expression\n");
            exit(1);
        }
        
        binary->type = EXPR_BINARY;
        binary->value.binary.operator = op;
        binary->value.binary.left = expr;
        binary->value.binary.right = right;
        
        // Comparison always results in boolean (represented as int)
        binary->data_type = create_type(TYPE_INT);
        
        expr = binary;
    }
    
    return expr;
}

/**
 * Parse a logical AND expression (&&)
 */
static Expression *parse_logical_and_expression(Parser *parser) {
    Expression *expr = parse_equality_expression(parser);
    
    while (parser->current_token.type == TOKEN_AND) {
        parser_consume(parser);
        
        Expression *right = parse_equality_expression(parser);
        
        Expression *binary = malloc(sizeof(Expression));
        if (binary == NULL) {
            fprintf(stderr, "Memory allocation failed for expression\n");
            exit(1);
        }
        
        binary->type = EXPR_BINARY;
        binary->value.binary.operator = TOKEN_AND;
        binary->value.binary.left = expr;
        binary->value.binary.right = right;
        
        // Logical operations result in boolean (represented as int)
        binary->data_type = create_type(TYPE_INT);
        
        expr = binary;
    }
    
    return expr;
}

/**
 * Parse a logical OR expression (||)
 */
static Expression *parse_logical_or_expression(Parser *parser) {
    Expression *expr = parse_logical_and_expression(parser);
    
    while (parser->current_token.type == TOKEN_OR) {
        parser_consume(parser);
        
        Expression *right = parse_logical_and_expression(parser);
        
        Expression *binary = malloc(sizeof(Expression));
        if (binary == NULL) {
            fprintf(stderr, "Memory allocation failed for expression\n");
            exit(1);
        }
        
        binary->type = EXPR_BINARY;
        binary->value.binary.operator = TOKEN_OR;
        binary->value.binary.left = expr;
        binary->value.binary.right = right;
        
        // Logical operations result in boolean (represented as int)
        binary->data_type = create_type(TYPE_INT);
        
        expr = binary;
    }
    
    return expr;
}

/**
 * Parse an assignment expression (=)
 */
static Expression *parse_assignment_expression(Parser *parser) {
    Expression *expr = parse_logical_or_expression(parser);
    
    if (parser->current_token.type == TOKEN_ASSIGN) {
        parser_consume(parser);
        
        Expression *value = parse_assignment_expression(parser);
        
        // Only variables and array elements can be assigned to
        if (expr->type == EXPR_VARIABLE || expr->type == EXPR_SUBSCRIPT) {
            Expression *assign = malloc(sizeof(Expression));
            if (assign == NULL) {
                fprintf(stderr, "Memory allocation failed for expression\n");
                exit(1);
            }
            
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

/**
 * Parse an expression
 */
Expression *parse_expression(Parser *parser) {
    return parse_assignment_expression(parser);
}

/**
 * Parse an expression statement (expression followed by semicolon)
 */
static Statement *parse_expression_statement(Parser *parser) {
    Statement *stmt = malloc(sizeof(Statement));
    if (stmt == NULL) {
        fprintf(stderr, "Memory allocation failed for statement\n");
        exit(1);
    }
    
    stmt->type = STMT_EXPRESSION;
    stmt->value.expression = parse_expression(parser);
    parser_expect(parser, TOKEN_SEMICOLON);
    
    return stmt;
}

/**
 * Parse a return statement
 */
static Statement *parse_return_statement(Parser *parser) {
    Statement *stmt = malloc(sizeof(Statement));
    if (stmt == NULL) {
        fprintf(stderr, "Memory allocation failed for statement\n");
        exit(1);
    }
    
    stmt->type = STMT_RETURN;
    parser_consume(parser); // Consume 'return'
    
    if (parser->current_token.type != TOKEN_SEMICOLON) {
        stmt->value.expression = parse_expression(parser);
    } else {
        // Return void
        Expression *expr = malloc(sizeof(Expression));
        if (expr == NULL) {
            fprintf(stderr, "Memory allocation failed for expression\n");
            exit(1);
        }
        
        expr->type = EXPR_LITERAL_INT;
        expr->data_type = create_type(TYPE_VOID);
        expr->value.int_value = 0;
        stmt->value.expression = expr;
    }
    
    parser_expect(parser, TOKEN_SEMICOLON);
    return stmt;
}

/**
 * Parse an if statement
 */
static Statement *parse_if_statement(Parser *parser) {
    Statement *stmt = malloc(sizeof(Statement));
    if (stmt == NULL) {
        fprintf(stderr, "Memory allocation failed for statement\n");
        exit(1);
    }
    
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

/**
 * Parse a while statement
 */
static Statement *parse_while_statement(Parser *parser) {
    Statement *stmt = malloc(sizeof(Statement));
    if (stmt == NULL) {
        fprintf(stderr, "Memory allocation failed for statement\n");
        exit(1);
    }
    
    stmt->type = STMT_WHILE;
    parser_consume(parser); // Consume 'while'
    
    parser_expect(parser, TOKEN_LPAREN);
    stmt->value.while_statement.condition = parse_expression(parser);
    parser_expect(parser, TOKEN_RPAREN);
    
    stmt->value.while_statement.body = parse_statement(parser);
    
    return stmt;
}

/**
 * Parse a for statement
 */
static Statement *parse_for_statement(Parser *parser) {
    Statement *stmt = malloc(sizeof(Statement));
    if (stmt == NULL) {
        fprintf(stderr, "Memory allocation failed for statement\n");
        exit(1);
    }
    
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
        if (expr == NULL) {
            fprintf(stderr, "Memory allocation failed for expression\n");
            exit(1);
        }
        
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

/**
 * Parse a block statement (multiple statements in curly braces)
 */
static Statement *parse_block(Parser *parser) {
    Statement *stmt = malloc(sizeof(Statement));
    if (stmt == NULL) {
        fprintf(stderr, "Memory allocation failed for statement\n");
        exit(1);
    }
    
    stmt->type = STMT_BLOCK;
    parser_expect(parser, TOKEN_LBRACE);
    
    // Parse statements in the block
    stmt->value.block.statements = malloc(100 * sizeof(Statement*)); // Initial capacity
    if (stmt->value.block.statements == NULL) {
        fprintf(stderr, "Memory allocation failed for block statements\n");
        exit(1);
    }
    
    stmt->value.block.statement_count = 0;
    
    while (parser->current_token.type != TOKEN_RBRACE) {
        if (stmt->value.block.statement_count >= 100) {
            // Expand the statements array if needed
            int new_capacity = stmt->value.block.statement_count * 2;
            Statement **new_stmts = realloc(stmt->value.block.statements, 
                                           new_capacity * sizeof(Statement*));
            if (new_stmts == NULL) {
                fprintf(stderr, "Memory reallocation failed for block statements\n");
                exit(1);
            }
            stmt->value.block.statements = new_stmts;
        }
        
        stmt->value.block.statements[stmt->value.block.statement_count++] = parse_statement(parser);
    }
    
    parser_expect(parser, TOKEN_RBRACE);
    return stmt;
}

/**
 * Parse a variable declaration
 */
static Statement *parse_declaration(Parser *parser) {
    Statement *stmt = malloc(sizeof(Statement));
    if (stmt == NULL) {
        fprintf(stderr, "Memory allocation failed for statement\n");
        exit(1);
    }
    
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

/**
 * Parse a statement (any kind)
 */
Statement *parse_statement(Parser *parser) {
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

/**
 * Parse a function definition
 */
static Function *parse_function(Parser *parser) {
    Function *function = malloc(sizeof(Function));
    if (function == NULL) {
        fprintf(stderr, "Memory allocation failed for function\n");
        exit(1);
    }
    
    // Parse return type
    function->return_type = parse_type(parser);
    
    // Parse function name
    parser_expect(parser, TOKEN_IDENTIFIER);
    function->name = strdup(parser->previous_token.lexeme);
    
    // Parse parameters
    parser_expect(parser, TOKEN_LPAREN);
    
    function->parameter_types = malloc(10 * sizeof(Type*)); // Initial capacity
    function->parameter_names = malloc(10 * sizeof(char*));
    if (function->parameter_types == NULL || function->parameter_names == NULL) {
        fprintf(stderr, "Memory allocation failed for function parameters\n");
        exit(1);
    }
    
    function->parameter_count = 0;
    
    if (parser->current_token.type != TOKEN_RPAREN) {
        do {
            if (function->parameter_count >= 10) {
                // Expand the parameters arrays if needed
                int new_capacity = function->parameter_count * 2;
                Type **new_types = realloc(function->parameter_types, 
                                          new_capacity * sizeof(Type*));
                char **new_names = realloc(function->parameter_names, 
                                          new_capacity * sizeof(char*));
                if (new_types == NULL || new_names == NULL) {
                    fprintf(stderr, "Memory reallocation failed for function parameters\n");
                    exit(1);
                }
                function->parameter_types = new_types;
                function->parameter_names = new_names;
            }
            
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

/**
 * Parse a complete C program
 */
Program *parse_program(Lexer *lexer) {
    Parser parser;
    parser_init(&parser, lexer);
    
    Program *program = malloc(sizeof(Program));
    if (program == NULL) {
        fprintf(stderr, "Memory allocation failed for program\n");
        exit(1);
    }
    
    program->functions = malloc(100 * sizeof(Function*)); // Initial capacity
    if (program->functions == NULL) {
        fprintf(stderr, "Memory allocation failed for functions\n");
        free(program);
        exit(1);
    }
    
    program->function_count = 0;
    program->function_capacity = 100;
    
    while (parser.current_token.type != TOKEN_EOF) {
        if (program->function_count >= program->function_capacity) {
            program->function_capacity *= 2;
            Function **new_functions = realloc(program->functions, 
                                             program->function_capacity * sizeof(Function*));
            if (new_functions == NULL) {
                fprintf(stderr, "Memory reallocation failed for functions\n");
                exit(1);
            }
            program->functions = new_functions;
        }
        
        program->functions[program->function_count++] = parse_function(&parser);
    }
    
    return program;
}

/**
 * Free resources used by an expression
 */
void free_expression(Expression *expr) {
    if (expr == NULL) {
        return;
    }
    
    switch (expr->type) {
        case EXPR_BINARY:
            free_expression(expr->value.binary.left);
            free_expression(expr->value.binary.right);
            break;
        case EXPR_UNARY:
            free_expression(expr->value.unary.operand);
            break;
        case EXPR_VARIABLE:
            free(expr->value.variable_name);
            break;
        case EXPR_CALL:
            free(expr->value.call.function_name);
            for (int i = 0; i < expr->value.call.argument_count; i++) {
                free_expression(expr->value.call.arguments[i]);
            }
            free(expr->value.call.arguments);
            break;
        case EXPR_ASSIGN:
            free(expr->value.assign.variable_name);
            free_expression(expr->value.assign.value);
            break;
        case EXPR_SUBSCRIPT:
            free_expression(expr->value.subscript.array);
            free_expression(expr->value.subscript.index);
            break;
        default:
            // No additional cleanup needed for literals
            break;
    }
    
    free_type(expr->data_type);
    free(expr);
}

/**
 * Free resources used by a statement
 */
void free_statement(Statement *stmt) {
    if (stmt == NULL) {
        return;
    }
    
    switch (stmt->type) {
        case STMT_EXPRESSION:
            free_expression(stmt->value.expression);
            break;
        case STMT_RETURN:
            free_expression(stmt->value.expression);
            break;
        case STMT_IF:
            free_expression(stmt->value.if_statement.condition);
            free_statement(stmt->value.if_statement.then_branch);
            free_statement(stmt->value.if_statement.else_branch);
            break;
        case STMT_WHILE:
            free_expression(stmt->value.while_statement.condition);
            free_statement(stmt->value.while_statement.body);
            break;
        case STMT_FOR:
            free_expression(stmt->value.for_statement.initializer);
            free_expression(stmt->value.for_statement.condition);
            free_expression(stmt->value.for_statement.increment);
            free_statement(stmt->value.for_statement.body);
            break;
        case STMT_BLOCK:
            for (int i = 0; i < stmt->value.block.statement_count; i++) {
                free_statement(stmt->value.block.statements[i]);
            }
            free(stmt->value.block.statements);
            break;
        case STMT_DECLARATION:
            free_type(stmt->value.declaration.type);
            free(stmt->value.declaration.name);
            free_expression(stmt->value.declaration.initializer);
            break;
    }
    
    free(stmt);
}

/**
 * Free resources used by a function
 */
void free_function(Function *function) {
    if (function == NULL) {
        return;
    }
    
    free_type(function->return_type);
    free(function->name);
    
    for (int i = 0; i < function->parameter_count; i++) {
        free_type(function->parameter_types[i]);
        free(function->parameter_names[i]);
    }
    
    free(function->parameter_types);
    free(function->parameter_names);
    
    free_statement(function->body);
    free(function);
}

/**
 * Free resources used by a program
 */
void free_program(Program *program) {
    if (program == NULL) {
        return;
    }
    
    for (int i = 0; i < program->function_count; i++) {
        free_function(program->functions[i]);
    }
    
    free(program->functions);
    free(program);
}