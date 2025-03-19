/**
* Parser implementation for the COIL C Compiler
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "token.h"
#include "ast.h"
#include "type.h"

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
      fprintf(stderr, "Line %d: Expected token type %d, got %d (%s)\n", 
              parser->current_token.line, type, parser->current_token.type,
              parser->current_token.lexeme);
      exit(1);
  }
  parser_consume(parser);
}

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
          fprintf(stderr, "Line %d: Unexpected token in expression: %s\n", 
                  parser->current_token.line, parser->current_token.lexeme);
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

Expression *parse_expression(Parser *parser) {
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