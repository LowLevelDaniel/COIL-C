/**
 * @file ast.c
 * @brief Abstract Syntax Tree implementation for the COIL C Compiler
 * @details Provides data structures and functions for representing C code structure
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ast.h"
#include "token.h"
#include "type.h"

/**
 * @brief Create a binary expression node
 * @param left Left operand
 * @param right Right operand
 * @param operator Operator token type
 * @param result_type Result data type
 * @return Pointer to the newly created expression node
 */
Expression *create_binary_expr(Expression *left, Expression *right, TokenType operator, Type *result_type) {
  Expression *expr = NULL;
  
  /* Validate input parameters */
  if (left == NULL || right == NULL || result_type == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to create_binary_expr\n");
    return NULL;
  }
  
  expr = (Expression *)malloc(sizeof(Expression));
  if (expr == NULL) {
    fprintf(stderr, "Memory allocation failed for binary expression\n");
    exit(1);
  }
  
  expr->type = EXPR_BINARY;
  expr->data_type = result_type;
  expr->value.binary.left = left;
  expr->value.binary.right = right;
  expr->value.binary.operator = operator;
  
  return expr;
}

/**
 * @brief Create an integer literal expression node
 * @param value Integer value to create a node for
 * @return Pointer to the newly created expression node
 */
Expression *create_literal_int_expr(int value) {
  Expression *expr = NULL;
  
  expr = (Expression *)malloc(sizeof(Expression));
  if (expr == NULL) {
    fprintf(stderr, "Memory allocation failed for integer literal expression\n");
    exit(1);
  }
  
  expr->type = EXPR_LITERAL_INT;
  expr->data_type = create_type(TYPE_INT);
  if (expr->data_type == NULL) {
    fprintf(stderr, "Failed to create type for integer literal\n");
    free(expr);
    return NULL;
  }
  
  expr->value.int_value = value;
  
  return expr;
}

/**
 * @brief Create a floating-point literal expression node
 * @param value Float value to create a node for
 * @return Pointer to the newly created expression node
 */
Expression *create_literal_float_expr(float value) {
  Expression *expr = NULL;
  
  expr = (Expression *)malloc(sizeof(Expression));
  if (expr == NULL) {
    fprintf(stderr, "Memory allocation failed for float literal expression\n");
    exit(1);
  }
  
  expr->type = EXPR_LITERAL_FLOAT;
  expr->data_type = create_type(TYPE_FLOAT);
  if (expr->data_type == NULL) {
    fprintf(stderr, "Failed to create type for float literal\n");
    free(expr);
    return NULL;
  }
  
  expr->value.float_value = value;
  
  return expr;
}

/**
 * @brief Create a variable reference expression node
 * @param name Variable name
 * @param type Variable type
 * @return Pointer to the newly created expression node
 */
Expression *create_variable_expr(const char *name, Type *type) {
  Expression *expr = NULL;
  
  /* Validate input parameters */
  if (name == NULL || type == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to create_variable_expr\n");
    return NULL;
  }
  
  expr = (Expression *)malloc(sizeof(Expression));
  if (expr == NULL) {
    fprintf(stderr, "Memory allocation failed for variable expression\n");
    exit(1);
  }
  
  expr->type = EXPR_VARIABLE;
  expr->data_type = type;
  expr->value.variable_name = strdup(name);
  
  if (expr->value.variable_name == NULL) {
    fprintf(stderr, "Memory allocation failed for variable name\n");
    free(expr);
    exit(1);
  }
  
  return expr;
}

/**
 * @brief Create a function call expression node
 * @param function_name Name of the function to call
 * @param arguments Array of argument expressions
 * @param argument_count Number of arguments
 * @return Pointer to the newly created expression node
 */
Expression *create_call_expr(const char *function_name, Expression **arguments, int argument_count) {
  Expression *expr = NULL;
  
  /* Validate input parameters */
  if (function_name == NULL || (arguments == NULL && argument_count > 0)) {
    fprintf(stderr, "Error: Invalid parameters passed to create_call_expr\n");
    return NULL;
  }
  
  expr = (Expression *)malloc(sizeof(Expression));
  if (expr == NULL) {
    fprintf(stderr, "Memory allocation failed for function call expression\n");
    exit(1);
  }
  
  expr->type = EXPR_CALL;
  expr->data_type = create_type(TYPE_INT); /* Default to int, will be updated by semantic analyzer */
  if (expr->data_type == NULL) {
    fprintf(stderr, "Failed to create default return type for function call\n");
    free(expr);
    return NULL;
  }
  
  expr->value.call.function_name = strdup(function_name);
  if (expr->value.call.function_name == NULL) {
    fprintf(stderr, "Memory allocation failed for function name\n");
    free_type(expr->data_type);
    free(expr);
    exit(1);
  }
  
  expr->value.call.arguments = arguments;
  expr->value.call.argument_count = argument_count;
  
  return expr;
}

/**
 * @brief Create an assignment expression node
 * @param variable_name Variable name to assign to
 * @param value Value expression to assign
 * @param type Result type
 * @return Pointer to the newly created expression node
 */
Expression *create_assign_expr(const char *variable_name, Expression *value, Type *type) {
  Expression *expr = NULL;
  
  /* Validate input parameters */
  if (variable_name == NULL || value == NULL || type == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to create_assign_expr\n");
    return NULL;
  }
  
  expr = (Expression *)malloc(sizeof(Expression));
  if (expr == NULL) {
    fprintf(stderr, "Memory allocation failed for assignment expression\n");
    exit(1);
  }
  
  expr->type = EXPR_ASSIGN;
  expr->data_type = type;
  expr->value.assign.variable_name = strdup(variable_name);
  
  if (expr->value.assign.variable_name == NULL) {
    fprintf(stderr, "Memory allocation failed for variable name\n");
    free(expr);
    exit(1);
  }
  
  expr->value.assign.value = value;
  
  return expr;
}

/**
 * @brief Create a unary expression node
 * @param operand Operand expression
 * @param operator Operator token type
 * @param result_type Result data type
 * @return Pointer to the newly created expression node
 */
Expression *create_unary_expr(Expression *operand, TokenType operator, Type *result_type) {
  Expression *expr = NULL;
  
  /* Validate input parameters */
  if (operand == NULL || result_type == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to create_unary_expr\n");
    return NULL;
  }
  
  expr = (Expression *)malloc(sizeof(Expression));
  if (expr == NULL) {
    fprintf(stderr, "Memory allocation failed for unary expression\n");
    exit(1);
  }
  
  expr->type = EXPR_UNARY;
  expr->data_type = result_type;
  expr->value.unary.operand = operand;
  expr->value.unary.operator = operator;
  
  return expr;
}

/**
 * @brief Create a subscript expression node (array access)
 * @param array Array expression
 * @param index Index expression
 * @param element_type Element type of the array
 * @return Pointer to the newly created expression node
 */
Expression *create_subscript_expr(Expression *array, Expression *index, Type *element_type) {
  Expression *expr = NULL;
  
  /* Validate input parameters */
  if (array == NULL || index == NULL || element_type == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to create_subscript_expr\n");
    return NULL;
  }
  
  expr = (Expression *)malloc(sizeof(Expression));
  if (expr == NULL) {
    fprintf(stderr, "Memory allocation failed for subscript expression\n");
    exit(1);
  }
  
  expr->type = EXPR_SUBSCRIPT;
  expr->data_type = element_type;
  expr->value.subscript.array = array;
  expr->value.subscript.index = index;
  
  return expr;
}

/**
 * @brief Create a cast expression node
 * @param cast_type Target type
 * @param expression Expression to cast
 * @return Pointer to the newly created expression node
 */
Expression *create_cast_expr(Type *cast_type, Expression *expression) {
  Expression *expr = NULL;
  
  /* Validate input parameters */
  if (cast_type == NULL || expression == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to create_cast_expr\n");
    return NULL;
  }
  
  expr = (Expression *)malloc(sizeof(Expression));
  if (expr == NULL) {
    fprintf(stderr, "Memory allocation failed for cast expression\n");
    exit(1);
  }
  
  expr->type = EXPR_CAST;
  expr->data_type = cast_type;
  expr->value.cast.cast_type = cast_type;
  expr->value.cast.expression = expression;
  
  return expr;
}

/**
 * @brief Create an expression statement node
 * @param expr Expression to create a statement for
 * @return Pointer to the newly created statement node
 */
Statement *create_expression_stmt(Expression *expr) {
  Statement *stmt = NULL;
  
  /* Validate input parameters */
  if (expr == NULL) {
    fprintf(stderr, "Error: NULL expression passed to create_expression_stmt\n");
    return NULL;
  }
  
  stmt = (Statement *)malloc(sizeof(Statement));
  if (stmt == NULL) {
    fprintf(stderr, "Memory allocation failed for expression statement\n");
    exit(1);
  }
  
  stmt->type = STMT_EXPRESSION;
  stmt->value.expression = expr;
  
  return stmt;
}

/**
 * @brief Create a return statement node
 * @param expr Return value expression (can be NULL for void return)
 * @return Pointer to the newly created statement node
 */
Statement *create_return_stmt(Expression *expr) {
  Statement *stmt = NULL;
  
  stmt = (Statement *)malloc(sizeof(Statement));
  if (stmt == NULL) {
    fprintf(stderr, "Memory allocation failed for return statement\n");
    exit(1);
  }
  
  stmt->type = STMT_RETURN;
  stmt->value.expression = expr;
  
  return stmt;
}

/**
 * @brief Create an if statement node
 * @param condition Condition expression
 * @param then_branch Statement for "then" branch
 * @param else_branch Statement for "else" branch (can be NULL)
 * @return Pointer to the newly created statement node
 */
Statement *create_if_stmt(Expression *condition, Statement *then_branch, Statement *else_branch) {
  Statement *stmt = NULL;
  
  /* Validate input parameters */
  if (condition == NULL || then_branch == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to create_if_stmt\n");
    return NULL;
  }
  
  stmt = (Statement *)malloc(sizeof(Statement));
  if (stmt == NULL) {
    fprintf(stderr, "Memory allocation failed for if statement\n");
    exit(1);
  }
  
  stmt->type = STMT_IF;
  stmt->value.if_statement.condition = condition;
  stmt->value.if_statement.then_branch = then_branch;
  stmt->value.if_statement.else_branch = else_branch;
  
  return stmt;
}

/**
 * @brief Create a while statement node
 * @param condition Loop condition expression
 * @param body Loop body statement
 * @return Pointer to the newly created statement node
 */
Statement *create_while_stmt(Expression *condition, Statement *body) {
  Statement *stmt = NULL;
  
  /* Validate input parameters */
  if (condition == NULL || body == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to create_while_stmt\n");
    return NULL;
  }
  
  stmt = (Statement *)malloc(sizeof(Statement));
  if (stmt == NULL) {
    fprintf(stderr, "Memory allocation failed for while statement\n");
    exit(1);
  }
  
  stmt->type = STMT_WHILE;
  stmt->value.while_statement.condition = condition;
  stmt->value.while_statement.body = body;
  
  return stmt;
}

/**
 * @brief Create a for statement node
 * @param initializer Initialization expression (can be NULL)
 * @param condition Loop condition expression (can be NULL)
 * @param increment Increment expression (can be NULL)
 * @param body Loop body statement
 * @return Pointer to the newly created statement node
 */
Statement *create_for_stmt(Expression *initializer, Expression *condition, 
                         Expression *increment, Statement *body) {
  Statement *stmt = NULL;
  
  /* Validate input parameters */
  if (body == NULL) {
    fprintf(stderr, "Error: NULL body parameter passed to create_for_stmt\n");
    return NULL;
  }
  
  stmt = (Statement *)malloc(sizeof(Statement));
  if (stmt == NULL) {
    fprintf(stderr, "Memory allocation failed for for statement\n");
    exit(1);
  }
  
  stmt->type = STMT_FOR;
  stmt->value.for_statement.initializer = initializer;
  stmt->value.for_statement.condition = condition;
  stmt->value.for_statement.increment = increment;
  stmt->value.for_statement.body = body;
  
  return stmt;
}

/**
 * @brief Create a block statement node
 * @param statements Array of statements
 * @param statement_count Number of statements
 * @return Pointer to the newly created statement node
 */
Statement *create_block_stmt(Statement **statements, int statement_count) {
  Statement *stmt = NULL;
  
  /* Validate input parameters */
  if ((statements == NULL && statement_count > 0) || statement_count < 0) {
    fprintf(stderr, "Error: Invalid parameters passed to create_block_stmt\n");
    return NULL;
  }
  
  stmt = (Statement *)malloc(sizeof(Statement));
  if (stmt == NULL) {
    fprintf(stderr, "Memory allocation failed for block statement\n");
    exit(1);
  }
  
  stmt->type = STMT_BLOCK;
  stmt->value.block.statements = statements;
  stmt->value.block.statement_count = statement_count;
  
  return stmt;
}

/**
 * @brief Create a declaration statement node
 * @param type Variable type
 * @param name Variable name
 * @param initializer Initial value expression (can be NULL)
 * @return Pointer to the newly created statement node
 */
Statement *create_declaration_stmt(Type *type, const char *name, Expression *initializer) {
  Statement *stmt = NULL;
  
  /* Validate input parameters */
  if (type == NULL || name == NULL) {
    fprintf(stderr, "Error: NULL required parameter passed to create_declaration_stmt\n");
    return NULL;
  }
  
  stmt = (Statement *)malloc(sizeof(Statement));
  if (stmt == NULL) {
    fprintf(stderr, "Memory allocation failed for declaration statement\n");
    exit(1);
  }
  
  stmt->type = STMT_DECLARATION;
  stmt->value.declaration.type = type;
  stmt->value.declaration.name = strdup(name);
  
  if (stmt->value.declaration.name == NULL) {
    fprintf(stderr, "Memory allocation failed for variable name\n");
    free(stmt);
    exit(1);
  }
  
  stmt->value.declaration.initializer = initializer;
  
  return stmt;
}

/**
 * @brief Create a function node
 * @param return_type Function return type
 * @param name Function name
 * @param parameter_types Array of parameter types
 * @param parameter_names Array of parameter names
 * @param parameter_count Number of parameters
 * @param body Function body statement
 * @return Pointer to the newly created function node
 */
Function *create_function(Type *return_type, const char *name,
                        Type **parameter_types, char **parameter_names,
                        int parameter_count, Statement *body) {
  Function *function = NULL;
  
  /* Validate input parameters */
  if (return_type == NULL || name == NULL || 
      (parameter_count > 0 && (parameter_types == NULL || parameter_names == NULL)) ||
      body == NULL) {
    fprintf(stderr, "Error: Invalid parameters passed to create_function\n");
    return NULL;
  }
  
  function = (Function *)malloc(sizeof(Function));
  if (function == NULL) {
    fprintf(stderr, "Memory allocation failed for function\n");
    exit(1);
  }
  
  function->return_type = return_type;
  function->name = strdup(name);
  
  if (function->name == NULL) {
    fprintf(stderr, "Memory allocation failed for function name\n");
    free(function);
    exit(1);
  }
  
  function->parameter_types = parameter_types;
  function->parameter_names = parameter_names;
  function->parameter_count = parameter_count;
  function->body = body;
  
  return function;
}

/**
 * @brief Create a program node
 * @return Pointer to the newly created program node
 */
Program *create_program(void) {
  Program *program = NULL;
  
  program = (Program *)malloc(sizeof(Program));
  if (program == NULL) {
    fprintf(stderr, "Memory allocation failed for program\n");
    exit(1);
  }
  
  program->functions = (Function **)malloc(10 * sizeof(Function*)); /* Initial capacity */
  if (program->functions == NULL) {
    fprintf(stderr, "Memory allocation failed for functions array\n");
    free(program);
    exit(1);
  }
  
  program->function_count = 0;
  program->function_capacity = 10;
  
  return program;
}

/**
 * @brief Add a function to a program
 * @param program Program to add to
 * @param function Function to add
 */
void add_function(Program *program, Function *function) {
  Function **new_functions = NULL;
  
  /* Validate input parameters */
  if (program == NULL || function == NULL) {
    fprintf(stderr, "Error: NULL parameter passed to add_function\n");
    return;
  }
  
  if (program->function_count >= program->function_capacity) {
    program->function_capacity *= 2;
    new_functions = (Function **)realloc(program->functions, 
                                   program->function_capacity * sizeof(Function*));
    if (new_functions == NULL) {
      fprintf(stderr, "Memory reallocation failed for functions array\n");
      exit(1);
    }
    program->functions = new_functions;
  }
  
  program->functions[program->function_count++] = function;
}

/**
 * @brief Free an expression and all its resources
 * @param expr Expression to free
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
    case EXPR_CAST:
      free_type(expr->value.cast.cast_type);
      free_expression(expr->value.cast.expression);
      break;
    default:
      /* No additional cleanup needed for literals */
      break;
  }
  
  free_type(expr->data_type);
  free(expr);
}

/**
 * @brief Free a statement and all its resources
 * @param stmt Statement to free
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
 * @brief Free a function and all its resources
 * @param function Function to free
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
 * @brief Free a program and all its resources
 * @param program Program to free
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