/**
* Abstract Syntax Tree definitions for the COIL C Compiler
*/

#ifndef AST_H
#define AST_H

#include "token.h"
#include "type.h"

/* Forward declarations */
struct Expression;
struct Statement;

/* Expression types */
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

/* Expression node */
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

/* Statement types */
typedef enum {
  STMT_EXPRESSION,
  STMT_RETURN,
  STMT_IF,
  STMT_WHILE,
  STMT_FOR,
  STMT_BLOCK,
  STMT_DECLARATION
} StatementType;

/* Statement node */
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

/* Function definition */
typedef struct {
  char *name;
  Type *return_type;
  Type **parameter_types;
  char **parameter_names;
  int parameter_count;
  Statement *body;
} Function;

/* Complete program */
typedef struct {
  Function **functions;
  int function_count;
  int function_capacity;
} Program;

/* AST node management */
Expression *create_binary_expr(Expression *left, Expression *right, TokenType operator, Type *result_type);
Expression *create_literal_int_expr(int value);
Expression *create_literal_float_expr(float value);
Expression *create_variable_expr(const char *name, Type *type);
Expression *create_call_expr(const char *function_name, Expression **arguments, int argument_count);

Statement *create_expression_stmt(Expression *expr);
Statement *create_return_stmt(Expression *expr);
Statement *create_if_stmt(Expression *condition, Statement *then_branch, Statement *else_branch);
Statement *create_block_stmt(Statement **statements, int statement_count);
Statement *create_declaration_stmt(Type *type, const char *name, Expression *initializer);

Program *create_program();
void add_function(Program *program, Function *function);
void free_program(Program *program);

#endif /* AST_H */