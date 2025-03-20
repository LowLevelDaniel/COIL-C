/**
 * @file parser.c
 * @brief Parser Implementation for the COIL C Compiler
 * @details Builds an Abstract Syntax Tree (AST) from C code tokens
 * with robust error handling and type tracking
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "parser.h"
#include "token.h"
#include "ast.h"
#include "type.h"
#include "symbol.h"

/**
 * @brief Initialize a parser with a lexer
 * 
 * @param parser Pointer to the parser to initialize
 * @param lexer Lexer to use for getting tokens
 */
void parser_init(Parser *parser, Lexer *lexer) {
  /* Validate input parameters */
  if (parser == NULL || lexer == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to parser_init\n");
    exit(1);
  }
  
  parser->lexer = lexer;
  parser->current_token = lexer_next_token(lexer);
  
  /* Initialize previous_token to prevent freeing uninitialized memory */
  parser->previous_token.type = TOKEN_EOF;
  parser->previous_token.lexeme = NULL;
  parser->previous_token.line = 0;
  parser->previous_token.column = 0;
  
  parser->had_error = false;
  parser->error_message = NULL;
}

/**
 * @brief Report a parsing error
 * 
 * @param parser The parser reporting the error
 * @param format Format string (printf style)
 * @param ... Additional arguments for format string
 */
void parser_error(Parser *parser, const char *format, ...) {
  va_list args;
  size_t message_size = 256;  /* Initial size for error message */
  
  /* Validate input parameters */
  if (parser == NULL || format == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to parser_error\n");
    return;
  }
  
  /* Free any previous error message */
  if (parser->error_message != NULL) {
    free(parser->error_message);
  }
  
  /* Allocate space for the new error message */
  parser->error_message = (char *)malloc(message_size);
  if (parser->error_message == NULL) {
    fprintf(stderr, "Memory allocation failed for error message\n");
    return;
  }
  
  /* Format the error message with line and column information */
  snprintf(parser->error_message, message_size, "Error at line %d, column %d: ", 
           parser->current_token.line, parser->current_token.column);
  
  /* Append the custom error message */
  size_t prefix_len = strlen(parser->error_message);
  va_start(args, format);
  vsnprintf(parser->error_message + prefix_len, message_size - prefix_len, format, args);
  va_end(args);
  
  parser->had_error = true;
  
  /* Also print the error to stderr */
  fprintf(stderr, "%s\n", parser->error_message);
}

/**
 * @brief Check if the parser encountered an error
 * 
 * @param parser The parser to check
 * @return true if an error occurred, false otherwise
 */
bool parser_had_error(Parser *parser) {
  if (parser == NULL) {
    return true;  /* If parser is NULL, we consider it an error */
  }
  return parser->had_error;
}

/**
 * @brief Get the parser's error message
 * 
 * @param parser The parser
 * @return The error message or NULL if no error occurred
 */
const char *parser_get_error(Parser *parser) {
  if (parser == NULL) {
    return "NULL parser";
  }
  return parser->error_message;
}

/**
 * @brief Consume the current token and move to the next one
 * 
 * @param parser Parser object
 */
static void parser_consume(Parser *parser) {
  /* Validate input parameter */
  if (parser == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to parser_consume\n");
    return;
  }
  
  token_free(parser->previous_token);
  parser->previous_token = parser->current_token;
  parser->current_token = lexer_next_token(parser->lexer);
}

/**
 * @brief Check if the current token has the expected type and consume it if so
 * 
 * @param parser Parser object
 * @param type Expected token type
 * @return true if token matched, false otherwise
 */
static bool parser_match(Parser *parser, TokenType type) {
  /* Validate input parameter */
  if (parser == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to parser_match\n");
    return false;
  }
  
  if (parser->current_token.type == type) {
    parser_consume(parser);
    return true;
  }
  return false;
}

/**
 * @brief Expect the current token to have the specified type, error if not
 * Consumes the token if it matches
 * 
 * @param parser Parser object
 * @param type Expected token type
 * @return true if token matched, false otherwise
 */
static bool parser_expect(Parser *parser, TokenType type) {
  /* Validate input parameter */
  if (parser == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to parser_expect\n");
    return false;
  }
  
  if (parser->current_token.type == type) {
    parser_consume(parser);
    return true;
  }
  
  parser_error(parser, "Expected token type %s, got %s", 
               token_type_to_string(type), 
               token_type_to_string(parser->current_token.type));
  return false;
}

/**
 * @brief Parse a type specifier (like int, char, float, double)
 * 
 * @param parser The parser
 * @return The parsed type structure
 */
Type *parse_type(Parser *parser) {
  Type *type = NULL;
  
  /* Validate input parameter */
  if (parser == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to parse_type\n");
    return NULL;
  }
  
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
      parser_error(parser, "Expected type specifier, got %s", 
                   token_type_to_string(parser->current_token.type));
      return NULL;
  }
  
  if (type == NULL) {
    parser_error(parser, "Failed to create type");
    return NULL;
  }
  
  /* Handle pointers (e.g., int* x) */
  while (parser_match(parser, TOKEN_MULTIPLY)) {
    Type *ptr_type = create_pointer_type(type);
    if (ptr_type == NULL) {
      free_type(type);
      parser_error(parser, "Failed to create pointer type");
      return NULL;
    }
    type = ptr_type;
  }
  
  return type;
}

/* Forward declarations for recursive parsing */
static Expression *parse_expression(Parser *parser);
static Statement *parse_statement(Parser *parser);
static Statement *parse_block(Parser *parser);

/**
 * @brief Parse a primary expression (literal, variable, or parenthesized)
 * 
 * @param parser Parser object
 * @return Parsed expression or NULL on error
 */
static Expression *parse_primary_expression(Parser *parser) {
  Expression *expr = NULL;
  
  /* Validate input parameter */
  if (parser == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to parse_primary_expression\n");
    return NULL;
  }
  
  switch (parser->current_token.type) {
    case TOKEN_NUMBER_INT: {
      int value = parser->current_token.value.int_value;
      parser_consume(parser);
      expr = create_literal_int_expr(value);
      break;
    }
      
    case TOKEN_NUMBER_FLOAT: {
      float value = parser->current_token.value.float_value;
      parser_consume(parser);
      expr = create_literal_float_expr(value);
      break;
    }
      
    case TOKEN_CHAR: {
      expr = (Expression *)malloc(sizeof(Expression));
      if (expr == NULL) {
        parser_error(parser, "Memory allocation failed for character literal");
        return NULL;
      }
      
      expr->type = EXPR_LITERAL_CHAR;
      expr->data_type = create_type(TYPE_CHAR);
      if (expr->data_type == NULL) {
        free(expr);
        parser_error(parser, "Failed to create char type");
        return NULL;
      }
      
      expr->value.char_value = parser->current_token.value.char_value;
      parser_consume(parser);
      break;
    }
      
    case TOKEN_IDENTIFIER: {
      const char *identifier = parser->current_token.lexeme;
      parser_consume(parser);
      
      /* Check if it's a function call */
      if (parser_match(parser, TOKEN_LPAREN)) {
        Expression **arguments = NULL;
        int argument_count = 0;
        int argument_capacity = 10;  /* Initial capacity */
        
        /* Allocate space for arguments */
        arguments = (Expression **)malloc(argument_capacity * sizeof(Expression*));
        if (arguments == NULL) {
          parser_error(parser, "Memory allocation failed for function arguments");
          return NULL;
        }
        
        /* Parse arguments if present */
        if (parser->current_token.type != TOKEN_RPAREN) {
          do {
            /* Check if we need to expand the arguments array */
            if (argument_count >= argument_capacity) {
              argument_capacity *= 2;
              Expression **new_args = (Expression **)realloc(arguments, 
                                     argument_capacity * sizeof(Expression*));
              if (new_args == NULL) {
                parser_error(parser, "Memory reallocation failed for function arguments");
                /* Clean up already parsed arguments */
                for (int i = 0; i < argument_count; i++) {
                  free_expression(arguments[i]);
                }
                free(arguments);
                return NULL;
              }
              arguments = new_args;
            }
            
            /* Parse the argument expression */
            Expression *arg = parse_expression(parser);
            if (arg == NULL) {
              /* Clean up already parsed arguments */
              for (int i = 0; i < argument_count; i++) {
                free_expression(arguments[i]);
              }
              free(arguments);
              return NULL;
            }
            
            arguments[argument_count++] = arg;
          } while (parser_match(parser, TOKEN_COMMA));
        }
        
        /* Expect closing parenthesis */
        if (!parser_expect(parser, TOKEN_RPAREN)) {
          /* Clean up arguments on error */
          for (int i = 0; i < argument_count; i++) {
            free_expression(arguments[i]);
          }
          free(arguments);
          return NULL;
        }
        
        /* Create the function call expression */
        expr = create_call_expr(identifier, arguments, argument_count);
      } else {
        /* Variable reference */
        expr = (Expression *)malloc(sizeof(Expression));
        if (expr == NULL) {
          parser_error(parser, "Memory allocation failed for variable expression");
          return NULL;
        }
        
        expr->type = EXPR_VARIABLE;
        expr->data_type = create_type(TYPE_INT);  /* Type will be resolved during semantic analysis */
        if (expr->data_type == NULL) {
          free(expr);
          parser_error(parser, "Failed to create variable type");
          return NULL;
        }
        
        expr->value.variable_name = strdup(identifier);
        if (expr->value.variable_name == NULL) {
          free_type(expr->data_type);
          free(expr);
          parser_error(parser, "Memory allocation failed for variable name");
          return NULL;
        }
      }
      break;
    }
      
    case TOKEN_LPAREN:
      parser_consume(parser);
      expr = parse_expression(parser);
      if (expr == NULL) {
        return NULL;
      }
      if (!parser_expect(parser, TOKEN_RPAREN)) {
        free_expression(expr);
        return NULL;
      }
      break;
      
    default:
      parser_error(parser, "Unexpected token in expression: %s", 
                   parser->current_token.lexeme);
      return NULL;
  }
  
  /* Check for allocation failure */
  if (expr == NULL) {
    parser_error(parser, "Expression creation failed");
    return NULL;
  }
  
  /* Handle array subscript (e.g., a[i]) */
  if (parser_match(parser, TOKEN_LBRACKET)) {
    Expression *array = expr;
    Expression *index = NULL;
    
    /* Parse the index expression */
    index = parse_expression(parser);
    if (index == NULL) {
      free_expression(array);
      return NULL;
    }
    
    /* Expect closing bracket */
    if (!parser_expect(parser, TOKEN_RBRACKET)) {
      free_expression(array);
      free_expression(index);
      return NULL;
    }
    
    /* Type will be the element type of the array */
    Type *element_type = NULL;
    if (array->data_type->base_type == TYPE_ARRAY) {
      element_type = array->data_type->pointer_to;
    } else if (array->data_type->base_type == TYPE_POINTER) {
      element_type = array->data_type->pointer_to;
    } else {
      parser_error(parser, "Cannot index non-array type");
      free_expression(array);
      free_expression(index);
      return NULL;
    }
    
    /* Create the subscript expression */
    expr = create_subscript_expr(array, index, element_type);
  }
  
  return expr;
}

/**
 * @brief Parse a unary expression (e.g., -x, !x, ~x)
 * 
 * @param parser Parser object
 * @return Parsed expression or NULL on error
 */
static Expression *parse_unary_expression(Parser *parser) {
  TokenType op;
  Expression *operand = NULL;
  Expression *expr = NULL;
  Type *result_type = NULL;
  
  /* Validate input parameter */
  if (parser == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to parse_unary_expression\n");
    return NULL;
  }
  
  /* Check if it's a unary expression */
  if (parser->current_token.type == TOKEN_PLUS ||
      parser->current_token.type == TOKEN_MINUS ||
      parser->current_token.type == TOKEN_NOT ||
      parser->current_token.type == TOKEN_BIT_NOT) {
    
    op = parser->current_token.type;
    parser_consume(parser);
    
    operand = parse_unary_expression(parser);
    if (operand == NULL) {
      return NULL;
    }
    
    /* Determine result type based on operand type */
    switch (op) {
      case TOKEN_PLUS:
      case TOKEN_MINUS:
        result_type = type_copy(operand->data_type);
        break;
      case TOKEN_NOT:
        result_type = create_type(TYPE_INT);  /* Boolean result */
        break;
      case TOKEN_BIT_NOT:
        result_type = type_copy(operand->data_type);
        break;
      default:
        parser_error(parser, "Unexpected unary operator");
        free_expression(operand);
        return NULL;
    }
    
    if (result_type == NULL) {
      parser_error(parser, "Failed to create result type for unary expression");
      free_expression(operand);
      return NULL;
    }
    
    expr = create_unary_expr(operand, op, result_type);
    if (expr == NULL) {
      free_type(result_type);
      free_expression(operand);
      return NULL;
    }
    
    return expr;
  }
  
  /* If it's not a unary expression, it's a primary expression */
  return parse_primary_expression(parser);
}

/**
 * @brief Parse a multiplicative expression (*, /, %)
 * 
 * @param parser Parser object
 * @return Parsed expression or NULL on error
 */
static Expression *parse_multiplicative_expression(Parser *parser) {
  Expression *expr = NULL;
  Expression *right = NULL;
  Expression *binary = NULL;
  TokenType op;
  Type *result_type = NULL;
  
  /* Validate input parameter */
  if (parser == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to parse_multiplicative_expression\n");
    return NULL;
  }
  
  /* Parse the first operand */
  expr = parse_unary_expression(parser);
  if (expr == NULL) {
    return NULL;
  }
  
  /* Parse operators and right operands as long as they match */
  while (parser->current_token.type == TOKEN_MULTIPLY ||
         parser->current_token.type == TOKEN_DIVIDE ||
         parser->current_token.type == TOKEN_MODULO) {
    
    op = parser->current_token.type;
    parser_consume(parser);
    
    /* Parse the right operand */
    right = parse_unary_expression(parser);
    if (right == NULL) {
      free_expression(expr);
      return NULL;
    }
    
    /* Determine result type */
    result_type = get_common_type(expr->data_type, right->data_type);
    if (result_type == NULL) {
      parser_error(parser, "Failed to determine common type for binary operation");
      free_expression(expr);
      free_expression(right);
      return NULL;
    }
    
    /* Create the binary expression */
    binary = create_binary_expr(expr, right, op, result_type);
    if (binary == NULL) {
      free_type(result_type);
      free_expression(expr);
      free_expression(right);
      return NULL;
    }
    
    expr = binary;
  }
  
  return expr;
}

/**
 * @brief Parse an additive expression (+, -)
 * 
 * @param parser Parser object
 * @return Parsed expression or NULL on error
 */
static Expression *parse_additive_expression(Parser *parser) {
  Expression *expr = NULL;
  Expression *right = NULL;
  Expression *binary = NULL;
  TokenType op;
  Type *result_type = NULL;
  
  /* Validate input parameter */
  if (parser == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to parse_additive_expression\n");
    return NULL;
  }
  
  /* Parse the first operand */
  expr = parse_multiplicative_expression(parser);
  if (expr == NULL) {
    return NULL;
  }
  
  /* Parse operators and right operands as long as they match */
  while (parser->current_token.type == TOKEN_PLUS ||
         parser->current_token.type == TOKEN_MINUS) {
    
    op = parser->current_token.type;
    parser_consume(parser);
    
    /* Parse the right operand */
    right = parse_multiplicative_expression(parser);
    if (right == NULL) {
      free_expression(expr);
      return NULL;
    }
    
    /* Determine result type */
    result_type = get_common_type(expr->data_type, right->data_type);
    if (result_type == NULL) {
      parser_error(parser, "Failed to determine common type for binary operation");
      free_expression(expr);
      free_expression(right);
      return NULL;
    }
    
    /* Create the binary expression */
    binary = create_binary_expr(expr, right, op, result_type);
    if (binary == NULL) {
      free_type(result_type);
      free_expression(expr);
      free_expression(right);
      return NULL;
    }
    
    expr = binary;
  }
  
  return expr;
}

/**
 * @brief Parse a relational expression (<, <=, >, >=)
 * 
 * @param parser Parser object
 * @return Parsed expression or NULL on error
 */
static Expression *parse_relational_expression(Parser *parser) {
  Expression *expr = NULL;
  Expression *right = NULL;
  Expression *binary = NULL;
  TokenType op;
  Type *result_type = NULL;
  
  /* Validate input parameter */
  if (parser == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to parse_relational_expression\n");
    return NULL;
  }
  
  /* Parse the first operand */
  expr = parse_additive_expression(parser);
  if (expr == NULL) {
    return NULL;
  }
  
  /* Parse operators and right operands as long as they match */
  while (parser->current_token.type == TOKEN_LT ||
         parser->current_token.type == TOKEN_LE ||
         parser->current_token.type == TOKEN_GT ||
         parser->current_token.type == TOKEN_GE) {
    
    op = parser->current_token.type;
    parser_consume(parser);
    
    /* Parse the right operand */
    right = parse_additive_expression(parser);
    if (right == NULL) {
      free_expression(expr);
      return NULL;
    }
    
    /* Comparison always results in boolean (represented as int) */
    result_type = create_type(TYPE_INT);
    if (result_type == NULL) {
      parser_error(parser, "Failed to create boolean result type");
      free_expression(expr);
      free_expression(right);
      return NULL;
    }
    
    /* Create the binary expression */
    binary = create_binary_expr(expr, right, op, result_type);
    if (binary == NULL) {
      free_type(result_type);
      free_expression(expr);
      free_expression(right);
      return NULL;
    }
    
    expr = binary;
  }
  
  return expr;
}

/**
 * @brief Parse an equality expression (==, !=)
 * 
 * @param parser Parser object
 * @return Parsed expression or NULL on error
 */
static Expression *parse_equality_expression(Parser *parser) {
  Expression *expr = NULL;
  Expression *right = NULL;
  Expression *binary = NULL;
  TokenType op;
  Type *result_type = NULL;
  
  /* Validate input parameter */
  if (parser == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to parse_equality_expression\n");
    return NULL;
  }
  
  /* Parse the first operand */
  expr = parse_relational_expression(parser);
  if (expr == NULL) {
    return NULL;
  }
  
  /* Parse operators and right operands as long as they match */
  while (parser->current_token.type == TOKEN_EQ ||
         parser->current_token.type == TOKEN_NEQ) {
    
    op = parser->current_token.type;
    parser_consume(parser);
    
    /* Parse the right operand */
    right = parse_relational_expression(parser);
    if (right == NULL) {
      free_expression(expr);
      return NULL;
    }
    
    /* Comparison always results in boolean (represented as int) */
    result_type = create_type(TYPE_INT);
    if (result_type == NULL) {
      parser_error(parser, "Failed to create boolean result type");
      free_expression(expr);
      free_expression(right);
      return NULL;
    }
    
    /* Create the binary expression */
    binary = create_binary_expr(expr, right, op, result_type);
    if (binary == NULL) {
      free_type(result_type);
      free_expression(expr);
      free_expression(right);
      return NULL;
    }
    
    expr = binary;
  }
  
  return expr;
}

/**
 * @brief Parse a logical AND expression (&&)
 * 
 * @param parser Parser object
 * @return Parsed expression or NULL on error
 */
static Expression *parse_logical_and_expression(Parser *parser) {
  Expression *expr = NULL;
  Expression *right = NULL;
  Expression *binary = NULL;
  Type *result_type = NULL;
  
  /* Validate input parameter */
  if (parser == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to parse_logical_and_expression\n");
    return NULL;
  }
  
  /* Parse the first operand */
  expr = parse_equality_expression(parser);
  if (expr == NULL) {
    return NULL;
  }
  
  /* Parse operators and right operands as long as they match */
  while (parser->current_token.type == TOKEN_AND) {
    parser_consume(parser);
    
    /* Parse the right operand */
    right = parse_equality_expression(parser);
    if (right == NULL) {
      free_expression(expr);
      return NULL;
    }
    
    /* Logical operations result in boolean (represented as int) */
    result_type = create_type(TYPE_INT);
    if (result_type == NULL) {
      parser_error(parser, "Failed to create boolean result type");
      free_expression(expr);
      free_expression(right);
      return NULL;
    }
    
    /* Create the binary expression */
    binary = create_binary_expr(expr, right, TOKEN_AND, result_type);
    if (binary == NULL) {
      free_type(result_type);
      free_expression(expr);
      free_expression(right);
      return NULL;
    }
    
    expr = binary;
  }
  
  return expr;
}

/**
 * @brief Parse a logical OR expression (||)
 * 
 * @param parser Parser object
 * @return Parsed expression or NULL on error
 */
static Expression *parse_logical_or_expression(Parser *parser) {
  Expression *expr = NULL;
  Expression *right = NULL;
  Expression *binary = NULL;
  Type *result_type = NULL;
  
  /* Validate input parameter */
  if (parser == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to parse_logical_or_expression\n");
    return NULL;
  }
  
  /* Parse the first operand */
  expr = parse_logical_and_expression(parser);
  if (expr == NULL) {
    return NULL;
  }
  
  /* Parse operators and right operands as long as they match */
  while (parser->current_token.type == TOKEN_OR) {
    parser_consume(parser);
    
    /* Parse the right operand */
    right = parse_logical_and_expression(parser);
    if (right == NULL) {
      free_expression(expr);
      return NULL;
    }
    
    /* Logical operations result in boolean (represented as int) */
    result_type = create_type(TYPE_INT);
    if (result_type == NULL) {
      parser_error(parser, "Failed to create boolean result type");
      free_expression(expr);
      free_expression(right);
      return NULL;
    }
    
    /* Create the binary expression */
    binary = create_binary_expr(expr, right, TOKEN_OR, result_type);
    if (binary == NULL) {
      free_type(result_type);
      free_expression(expr);
      free_expression(right);
      return NULL;
    }
    
    expr = binary;
  }
  
  return expr;
}

/**
 * @brief Parse an assignment expression (=)
 * 
 * @param parser Parser object
 * @return Parsed expression or NULL on error
 */
static Expression *parse_assignment_expression(Parser *parser) {
  Expression *expr = NULL;
  Expression *value = NULL;
  Expression *assign = NULL;
  
  /* Validate input parameter */
  if (parser == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to parse_assignment_expression\n");
    return NULL;
  }
  
  /* Parse the left side of the assignment */
  expr = parse_logical_or_expression(parser);
  if (expr == NULL) {
    return NULL;
  }
  
  /* Check if it's an assignment */
  if (parser->current_token.type == TOKEN_ASSIGN) {
    parser_consume(parser);
    
    /* Parse the right side of the assignment */
    value = parse_assignment_expression(parser);
    if (value == NULL) {
      free_expression(expr);
      return NULL;
    }
    
    /* Only variables and array elements can be assigned to */
    if (expr->type == EXPR_VARIABLE) {
      assign = create_assign_expr(expr->value.variable_name, value, value->data_type);
      if (assign == NULL) {
        free_expression(expr);
        free_expression(value);
        return NULL;
      }
      
      /* Clean up the original variable expression */
      free(expr->value.variable_name);
      free_type(expr->data_type);
      free(expr);
      
      return assign;
    } else if (expr->type == EXPR_SUBSCRIPT) {
      /* This is simplified; a real compiler would handle array assignment differently */
      parser_error(parser, "Array element assignment not yet supported");
      free_expression(expr);
      free_expression(value);
      return NULL;
    } else {
      parser_error(parser, "Invalid assignment target");
      free_expression(expr);
      free_expression(value);
      return NULL;
    }
  }
  
  return expr;
}

/**
 * @brief Parse an expression
 * 
 * @param parser Parser object
 * @return Parsed expression or NULL on error
 */
static Expression *parse_expression(Parser *parser) {
  /* Validate input parameter */
  if (parser == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to parse_expression\n");
    return NULL;
  }
  
  return parse_assignment_expression(parser);
}

/**
 * @brief Parse an expression statement (expression followed by semicolon)
 * 
 * @param parser Parser object
 * @return Parsed statement or NULL on error
 */
static Statement *parse_expression_statement(Parser *parser) {
  Expression *expr = NULL;
  Statement *stmt = NULL;
  
  /* Validate input parameter */
  if (parser == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to parse_expression_statement\n");
    return NULL;
  }
  
  /* Parse the expression */
  expr = parse_expression(parser);
  if (expr == NULL) {
    return NULL;
  }
  
  /* Expect semicolon */
  if (!parser_expect(parser, TOKEN_SEMICOLON)) {
    free_expression(expr);
    return NULL;
  }
  
  /* Create the statement */
  stmt = create_expression_stmt(expr);
  if (stmt == NULL) {
    free_expression(expr);
    return NULL;
  }
  
  return stmt;
}

/**
 * @brief Parse a return statement
 * 
 * @param parser Parser object
 * @return Parsed statement or NULL on error
 */
static Statement *parse_return_statement(Parser *parser) {
  Expression *expr = NULL;
  Statement *stmt = NULL;
  
  /* Validate input parameter */
  if (parser == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to parse_return_statement\n");
    return NULL;
  }
  
  /* Consume 'return' */
  parser_consume(parser);
  
  /* Parse return value if present */
  if (parser->current_token.type != TOKEN_SEMICOLON) {
    expr = parse_expression(parser);
    if (expr == NULL) {
      return NULL;
    }
  } else {
    /* Return void */
    expr = (Expression *)malloc(sizeof(Expression));
    if (expr == NULL) {
      parser_error(parser, "Memory allocation failed for expression");
      return NULL;
    }
    
    expr->type = EXPR_LITERAL_INT;
    expr->data_type = create_type(TYPE_VOID);
    if (expr->data_type == NULL) {
      free(expr);
      parser_error(parser, "Failed to create void type");
      return NULL;
    }
    
    expr->value.int_value = 0;
  }
  
  /* Expect semicolon */
  if (!parser_expect(parser, TOKEN_SEMICOLON)) {
    free_expression(expr);
    return NULL;
  }
  
  /* Create the statement */
  stmt = create_return_stmt(expr);
  if (stmt == NULL) {
    free_expression(expr);
    return NULL;
  }
  
  return stmt;
}

/**
 * @brief Parse an if statement
 * 
 * @param parser Parser object
 * @return Parsed statement or NULL on error
 */
static Statement *parse_if_statement(Parser *parser) {
  Expression *condition = NULL;
  Statement *then_branch = NULL;
  Statement *else_branch = NULL;
  Statement *stmt = NULL;
  
  /* Validate input parameter */
  if (parser == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to parse_if_statement\n");
    return NULL;
  }
  
  /* Consume 'if' */
  parser_consume(parser);
  
  /* Expect opening parenthesis */
  if (!parser_expect(parser, TOKEN_LPAREN)) {
    return NULL;
  }
  
  /* Parse condition */
  condition = parse_expression(parser);
  if (condition == NULL) {
    return NULL;
  }
  
  /* Expect closing parenthesis */
  if (!parser_expect(parser, TOKEN_RPAREN)) {
    free_expression(condition);
    return NULL;
  }
  
  /* Parse then branch */
  then_branch = parse_statement(parser);
  if (then_branch == NULL) {
    free_expression(condition);
    return NULL;
  }
  
  /* Parse else branch if present */
  if (parser_match(parser, TOKEN_ELSE)) {
    else_branch = parse_statement(parser);
    if (else_branch == NULL) {
      free_expression(condition);
      free_statement(then_branch);
      return NULL;
    }
  }
  
  /* Create the statement */
  stmt = create_if_stmt(condition, then_branch, else_branch);
  if (stmt == NULL) {
    free_expression(condition);
    free_statement(then_branch);
    free_statement(else_branch);
    return NULL;
  }
  
  return stmt;
}

/**
 * @brief Parse a while statement
 * 
 * @param parser Parser object
 * @return Parsed statement or NULL on error
 */
static Statement *parse_while_statement(Parser *parser) {
  Expression *condition = NULL;
  Statement *body = NULL;
  Statement *stmt = NULL;
  
  /* Validate input parameter */
  if (parser == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to parse_while_statement\n");
    return NULL;
  }
  
  /* Consume 'while' */
  parser_consume(parser);
  
  /* Expect opening parenthesis */
  if (!parser_expect(parser, TOKEN_LPAREN)) {
    return NULL;
  }
  
  /* Parse condition */
  condition = parse_expression(parser);
  if (condition == NULL) {
    return NULL;
  }
  
  /* Expect closing parenthesis */
  if (!parser_expect(parser, TOKEN_RPAREN)) {
    free_expression(condition);
    return NULL;
  }
  
  /* Parse body */
  body = parse_statement(parser);
  if (body == NULL) {
    free_expression(condition);
    return NULL;
  }
  
  /* Create the statement */
  stmt = create_while_stmt(condition, body);
  if (stmt == NULL) {
    free_expression(condition);
    free_statement(body);
    return NULL;
  }
  
  return stmt;
}

/* Forward declaration for parse_declaration */
static Statement *parse_declaration(Parser *parser);

/**
 * @brief Parse a for statement
 * 
 * @param parser Parser object
 * @return Parsed statement or NULL on error
 */
static Statement *parse_for_statement(Parser *parser) {
  Expression *initializer = NULL;
  Statement *init_decl = NULL;
  Expression *condition = NULL;
  Expression *increment = NULL;
  Statement *body = NULL;
  Statement *stmt = NULL;
  bool has_declaration = false;
  
  /* Validate input parameter */
  if (parser == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to parse_for_statement\n");
    return NULL;
  }
  
  /* Consume 'for' */
  parser_consume(parser);
  
  /* Expect opening parenthesis */
  if (!parser_expect(parser, TOKEN_LPAREN)) {
    return NULL;
  }
  
  /* Check for a declaration (C99-style for loop) */
  if (parser->current_token.type == TOKEN_INT ||
      parser->current_token.type == TOKEN_CHAR_KW ||
      parser->current_token.type == TOKEN_FLOAT ||
      parser->current_token.type == TOKEN_DOUBLE ||
      parser->current_token.type == TOKEN_VOID) {
    
    has_declaration = true;
    init_decl = parse_declaration(parser);
    if (init_decl == NULL) {
      return NULL;
    }
  }
  /* Parse initializer if present (C89-style) */
  else if (parser->current_token.type != TOKEN_SEMICOLON) {
    initializer = parse_expression(parser);
    if (initializer == NULL) {
      return NULL;
    }
    
    if (!parser_expect(parser, TOKEN_SEMICOLON)) {
      free_expression(initializer);
      return NULL;
    }
  } else {
    /* Empty initializer, just consume semicolon */
    parser_consume(parser);
  }
  
  /* Parse condition if present */
  if (parser->current_token.type != TOKEN_SEMICOLON) {
    condition = parse_expression(parser);
    if (condition == NULL) {
      if (has_declaration) {
        free_statement(init_decl);
      } else {
        free_expression(initializer);
      }
      return NULL;
    }
  } else {
    /* Default to true condition */
    condition = create_literal_int_expr(1);
    if (condition == NULL) {
      if (has_declaration) {
        free_statement(init_decl);
      } else {
        free_expression(initializer);
      }
      return NULL;
    }
  }
  
  /* Expect semicolon */
  if (!parser_expect(parser, TOKEN_SEMICOLON)) {
    free_expression(condition);
    if (has_declaration) {
      free_statement(init_decl);
    } else {
      free_expression(initializer);
    }
    return NULL;
  }
  
  /* Parse increment if present */
  if (parser->current_token.type != TOKEN_RPAREN) {
    increment = parse_expression(parser);
    if (increment == NULL) {
      free_expression(condition);
      if (has_declaration) {
        free_statement(init_decl);
      } else {
        free_expression(initializer);
      }
      return NULL;
    }
  }
  
  /* Expect closing parenthesis */
  if (!parser_expect(parser, TOKEN_RPAREN)) {
    free_expression(condition);
    free_expression(increment);
    if (has_declaration) {
      free_statement(init_decl);
    } else {
      free_expression(initializer);
    }
    return NULL;
  }
  
  /* Parse body */
  body = parse_statement(parser);
  if (body == NULL) {
    free_expression(condition);
    free_expression(increment);
    if (has_declaration) {
      free_statement(init_decl);
    } else {
      free_expression(initializer);
    }
    return NULL;
  }
  
  /* If we have a declaration initialization, we need to create a block statement
   * that contains both the declaration and the original for loop */
  if (has_declaration) {
    /* Create the for statement with NULL as initializer (since it's handled separately) */
    stmt = create_for_stmt(NULL, condition, increment, body);
    if (stmt == NULL) {
      free_expression(condition);
      free_expression(increment);
      free_statement(init_decl);
      free_statement(body);
      return NULL;
    }
    
    /* Create a block that contains both the declaration and the for loop */
    Statement **statements = malloc(2 * sizeof(Statement*));
    if (statements == NULL) {
      free_expression(condition);
      free_expression(increment);
      free_statement(init_decl);
      free_statement(body);
      free_statement(stmt);
      parser_error(parser, "Memory allocation failed for for-loop block");
      return NULL;
    }
    
    statements[0] = init_decl;
    statements[1] = stmt;
    
    /* Create a block statement */
    Statement *block_stmt = create_block_stmt(statements, 2);
    if (block_stmt == NULL) {
      free_expression(condition);
      free_expression(increment);
      free_statement(init_decl);
      free_statement(body);
      free_statement(stmt);
      free(statements);
      return NULL;
    }
    
    return block_stmt;
  } else {
    /* Create the standard for statement */
    stmt = create_for_stmt(initializer, condition, increment, body);
    if (stmt == NULL) {
      free_expression(initializer);
      free_expression(condition);
      free_expression(increment);
      free_statement(body);
      return NULL;
    }
    
    return stmt;
  }
}

/**
 * @brief Parse a block statement (multiple statements in curly braces)
 * 
 * @param parser Parser object
 * @return Parsed statement or NULL on error
 */
static Statement *parse_block(Parser *parser) {
  Statement **statements = NULL;
  int statement_count = 0;
  int statement_capacity = 10;  /* Initial capacity */
  Statement *stmt = NULL;
  int i;
  
  /* Validate input parameter */
  if (parser == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to parse_block\n");
    return NULL;
  }
  
  /* Expect opening brace */
  if (!parser_expect(parser, TOKEN_LBRACE)) {
    return NULL;
  }
  
  /* Allocate initial array for statements */
  statements = (Statement **)malloc(statement_capacity * sizeof(Statement*));
  if (statements == NULL) {
    parser_error(parser, "Memory allocation failed for block statements");
    return NULL;
  }
  
  /* Parse statements until closing brace */
  while (parser->current_token.type != TOKEN_RBRACE) {
    /* Check if we need to expand the statements array */
    if (statement_count >= statement_capacity) {
      Statement **new_stmts;
      int new_capacity = statement_capacity * 2;
      
      new_stmts = (Statement **)realloc(statements, new_capacity * sizeof(Statement*));
      if (new_stmts == NULL) {
        parser_error(parser, "Memory reallocation failed for block statements");
        /* Clean up already parsed statements */
        for (i = 0; i < statement_count; i++) {
          free_statement(statements[i]);
        }
        free(statements);
        return NULL;
      }
      
      statements = new_stmts;
      statement_capacity = new_capacity;
    }
    
    /* Parse a statement */
    Statement *statement = parse_statement(parser);
    if (statement == NULL) {
      /* Clean up already parsed statements */
      for (i = 0; i < statement_count; i++) {
        free_statement(statements[i]);
      }
      free(statements);
      return NULL;
    }
    
    statements[statement_count++] = statement;
    
    /* Check for end of file in the middle of a block */
    if (parser->current_token.type == TOKEN_EOF) {
      parser_error(parser, "Unexpected end of file in block");
      /* Clean up already parsed statements */
      for (i = 0; i < statement_count; i++) {
        free_statement(statements[i]);
      }
      free(statements);
      return NULL;
    }
  }
  
  /* Expect closing brace */
  if (!parser_expect(parser, TOKEN_RBRACE)) {
    /* Clean up already parsed statements */
    for (i = 0; i < statement_count; i++) {
      free_statement(statements[i]);
    }
    free(statements);
    return NULL;
  }
  
  /* Create the statement */
  stmt = create_block_stmt(statements, statement_count);
  if (stmt == NULL) {
    /* Clean up already parsed statements */
    for (i = 0; i < statement_count; i++) {
      free_statement(statements[i]);
    }
    free(statements);
    return NULL;
  }
  
  return stmt;
}

/**
 * @brief Parse a variable declaration
 * 
 * @param parser Parser object
 * @return Parsed statement or NULL on error
 */
static Statement *parse_declaration(Parser *parser) {
  Type *type = NULL;
  const char *name = NULL;
  Expression *initializer = NULL;
  Statement *stmt = NULL;
  
  /* Validate input parameter */
  if (parser == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to parse_declaration\n");
    return NULL;
  }
  
  /* Parse type */
  type = parse_type(parser);
  if (type == NULL) {
    return NULL;
  }
  
  /* Expect identifier */
  if (!parser_expect(parser, TOKEN_IDENTIFIER)) {
    free_type(type);
    return NULL;
  }
  
  /* Get variable name */
  name = parser->previous_token.lexeme;
  
  /* Check for array declaration */
  if (parser_match(parser, TOKEN_LBRACKET)) {
    if (parser->current_token.type == TOKEN_NUMBER_INT) {
      int size = parser->current_token.value.int_value;
      parser_consume(parser);
      
      Type *array_type = create_array_type(type, size);
      if (array_type == NULL) {
        free_type(type);
        parser_error(parser, "Failed to create array type");
        return NULL;
      }
      
      type = array_type;
    } else {
      free_type(type);
      parser_error(parser, "Expected array size");
      return NULL;
    }
    
    if (!parser_expect(parser, TOKEN_RBRACKET)) {
      free_type(type);
      return NULL;
    }
  }
  
  /* Check for initializer */
  if (parser_match(parser, TOKEN_ASSIGN)) {
    initializer = parse_expression(parser);
    if (initializer == NULL) {
      free_type(type);
      return NULL;
    }
  }
  
  /* Expect semicolon */
  if (!parser_expect(parser, TOKEN_SEMICOLON)) {
    free_type(type);
    free_expression(initializer);
    return NULL;
  }
  
  /* Create the statement */
  stmt = create_declaration_stmt(type, name, initializer);
  if (stmt == NULL) {
    free_type(type);
    free_expression(initializer);
    return NULL;
  }
  
  /* Variables are added to symbol tables during code generation */
  
  return stmt;
}

/**
 * @brief Parse a statement (any kind)
 * 
 * @param parser Parser object
 * @return Parsed statement or NULL on error
 */
static Statement *parse_statement(Parser *parser) {
  /* Validate input parameter */
  if (parser == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to parse_statement\n");
    return NULL;
  }
  
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
 * @brief Parse a function definition
 * 
 * @param parser Parser object
 * @return Parsed function or NULL on error
 */
static Function *parse_function(Parser *parser) {
  Type *return_type = NULL;
  const char *name = NULL;
  Type **parameter_types = NULL;
  char **parameter_names = NULL;
  int parameter_count = 0;
  int parameter_capacity = 10;  /* Initial capacity */
  Statement *body = NULL;
  Function *function = NULL;
  int i;
  
  /* Validate input parameter */
  if (parser == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to parse_function\n");
    return NULL;
  }
  
  /* Parse return type */
  return_type = parse_type(parser);
  if (return_type == NULL) {
    return NULL;
  }
  
  /* Expect function name */
  if (!parser_expect(parser, TOKEN_IDENTIFIER)) {
    free_type(return_type);
    return NULL;
  }
  
  /* Get function name */
  name = parser->previous_token.lexeme;
  
  /* Expect opening parenthesis */
  if (!parser_expect(parser, TOKEN_LPAREN)) {
    free_type(return_type);
    return NULL;
  }
  
  /* Allocate space for parameters */
  parameter_types = (Type **)malloc(parameter_capacity * sizeof(Type*));
  parameter_names = (char **)malloc(parameter_capacity * sizeof(char*));
  if (parameter_types == NULL || parameter_names == NULL) {
    free_type(return_type);
    free(parameter_types);
    free(parameter_names);
    parser_error(parser, "Memory allocation failed for function parameters");
    return NULL;
  }
  
  /* Parse parameters if present */
  if (parser->current_token.type != TOKEN_RPAREN) {
    do {
      /* Check if we need to expand the parameters arrays */
      if (parameter_count >= parameter_capacity) {
        Type **new_types;
        char **new_names;
        int new_capacity = parameter_capacity * 2;
        
        /* Expand parameter_types */
        new_types = (Type **)realloc(parameter_types, new_capacity * sizeof(Type*));
        if (new_types == NULL) {
          free_type(return_type);
          /* Clean up already parsed parameters */
          for (i = 0; i < parameter_count; i++) {
            free_type(parameter_types[i]);
            free(parameter_names[i]);
          }
          free(parameter_types);
          free(parameter_names);
          parser_error(parser, "Memory reallocation failed for function parameters");
          return NULL;
        }
        parameter_types = new_types;

        /* Expand parameter_names */
        new_names = (char **)realloc(parameter_names, new_capacity * sizeof(char*));
        if (new_names == NULL) {
          free_type(return_type);
          /* Clean up already parsed parameters */
          for (i = 0; i < parameter_count; i++) {
            free_type(parameter_types[i]);
            free(parameter_names[i]);
          }
          free(parameter_types);
          free(parameter_names);
          parser_error(parser, "Memory reallocation failed for function parameters");
          return NULL;
        }
        parameter_names = new_names;
        parameter_capacity = new_capacity;
      }
      
      /* Parse parameter type */
      Type *param_type = parse_type(parser);
      if (param_type == NULL) {
        free_type(return_type);
        /* Clean up already parsed parameters */
        for (i = 0; i < parameter_count; i++) {
          free_type(parameter_types[i]);
          free(parameter_names[i]);
        }
        free(parameter_types);
        free(parameter_names);
        return NULL;
      }
      
      /* Expect parameter name */
      if (!parser_expect(parser, TOKEN_IDENTIFIER)) {
        free_type(return_type);
        free_type(param_type);
        /* Clean up already parsed parameters */
        for (i = 0; i < parameter_count; i++) {
          free_type(parameter_types[i]);
          free(parameter_names[i]);
        }
        free(parameter_types);
        free(parameter_names);
        return NULL;
      }
      
      /* Store parameter type and name */
      parameter_types[parameter_count] = param_type;
      parameter_names[parameter_count] = strdup(parser->previous_token.lexeme);
      if (parameter_names[parameter_count] == NULL) {
        free_type(return_type);
        free_type(param_type);
        /* Clean up already parsed parameters */
        for (i = 0; i < parameter_count; i++) {
          free_type(parameter_types[i]);
          free(parameter_names[i]);
        }
        free(parameter_types);
        free(parameter_names);
        parser_error(parser, "Memory allocation failed for parameter name");
        return NULL;
      }
      
      parameter_count++;
    } while (parser_match(parser, TOKEN_COMMA));
  }
  
  /* Expect closing parenthesis */
  if (!parser_expect(parser, TOKEN_RPAREN)) {
    free_type(return_type);
    /* Clean up parameters */
    for (i = 0; i < parameter_count; i++) {
      free_type(parameter_types[i]);
      free(parameter_names[i]);
    }
    free(parameter_types);
    free(parameter_names);
    return NULL;
  }
  
  /* Parse function body */
  body = parse_block(parser);
  if (body == NULL) {
    free_type(return_type);
    /* Clean up parameters */
    for (i = 0; i < parameter_count; i++) {
      free_type(parameter_types[i]);
      free(parameter_names[i]);
    }
    free(parameter_types);
    free(parameter_names);
    return NULL;
  }
  
  /* Create the function */
  function = create_function(return_type, name, parameter_types, parameter_names, 
                           parameter_count, body);
  if (function == NULL) {
    free_type(return_type);
    /* Clean up parameters */
    for (i = 0; i < parameter_count; i++) {
      free_type(parameter_types[i]);
      free(parameter_names[i]);
    }
    free(parameter_types);
    free(parameter_names);
    free_statement(body);
    return NULL;
  }
  
  return function;
}

/**
 * @brief Parse a complete C program
 * 
 * @param lexer The lexer to read tokens from
 * @return The parsed program AST, or NULL if parsing failed
 */
Program *parse_program(Lexer *lexer) {
  Parser parser;
  Program *program = NULL;
  
  /* Validate input parameter */
  if (lexer == NULL) {
    fprintf(stderr, "Error: NULL lexer passed to parse_program\n");
    return NULL;
  }
  
  /* Initialize parser */
  parser_init(&parser, lexer);
  
  /* Create program */
  program = create_program();
  if (program == NULL) {
    return NULL;
  }
  
  /* Parse functions until end of file */
  while (parser.current_token.type != TOKEN_EOF) {
    Function *function = parse_function(&parser);
    if (function == NULL) {
      free_program(program);
      return NULL;
    }
    
    add_function(program, function);
  }
  
  return program;
}

/**
 * @brief Free parser resources
 * 
 * @param parser The parser to free
 */
void parser_free(Parser *parser) {
  if (parser == NULL) {
    return;
  }
  
  token_free(parser->current_token);
  token_free(parser->previous_token);
  
  if (parser->error_message != NULL) {
    free(parser->error_message);
  }
}