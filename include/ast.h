/**
 * @file ast.h
 * @brief Abstract Syntax Tree definitions for the COIL C Compiler
 *
 * Defines the data structures used to represent C code as an Abstract Syntax
 * Tree (AST) that can be processed for code generation.
 */

#ifndef AST_H
#define AST_H

#include "token.h"
#include "type.h"

/* Forward declarations */
struct Expression;
struct Statement;

/**
 * Expression types in the AST
 */
typedef enum {
    EXPR_BINARY,       /**< Binary operation (e.g., a + b) */
    EXPR_UNARY,        /**< Unary operation (e.g., -a, !a) */
    EXPR_LITERAL_INT,  /**< Integer literal */
    EXPR_LITERAL_FLOAT, /**< Floating-point literal */
    EXPR_LITERAL_CHAR, /**< Character literal */
    EXPR_VARIABLE,     /**< Variable reference */
    EXPR_CALL,         /**< Function call */
    EXPR_ASSIGN,       /**< Assignment expression */
    EXPR_SUBSCRIPT,    /**< Array subscript (e.g., a[i]) */
    EXPR_CAST          /**< Type cast */
} ExpressionType;

/**
 * Expression node in the AST
 */
typedef struct Expression {
    ExpressionType type;  /**< The type of expression */
    Type *data_type;      /**< The data type of the expression result */
    union {
        struct {
            struct Expression *left;
            struct Expression *right;
            TokenType operator;
        } binary;         /**< Binary expression data */
        struct {
            struct Expression *operand;
            TokenType operator;
        } unary;          /**< Unary expression data */
        int int_value;    /**< Integer literal value */
        float float_value; /**< Floating-point literal value */
        char char_value;  /**< Character literal value */
        char *variable_name; /**< Variable name for variable references */
        struct {
            char *function_name;
            struct Expression **arguments;
            int argument_count;
        } call;           /**< Function call data */
        struct {
            char *variable_name;
            struct Expression *value;
        } assign;         /**< Assignment expression data */
        struct {
            struct Expression *array;
            struct Expression *index;
        } subscript;      /**< Array subscript data */
        struct {
            Type *cast_type;
            struct Expression *expression;
        } cast;           /**< Type cast data */
    } value;              /**< Union of all possible expression values */
} Expression;

/**
 * Statement types in the AST
 */
typedef enum {
    STMT_EXPRESSION,   /**< Expression statement (e.g., a = b;) */
    STMT_RETURN,       /**< Return statement */
    STMT_IF,           /**< If statement */
    STMT_WHILE,        /**< While loop */
    STMT_FOR,          /**< For loop */
    STMT_BLOCK,        /**< Block of statements */
    STMT_DECLARATION   /**< Variable declaration */
} StatementType;

/**
 * Statement node in the AST
 */
typedef struct Statement {
    StatementType type; /**< The type of statement */
    union {
        Expression *expression; /**< For STMT_EXPRESSION and STMT_RETURN */
        struct {
            Expression *condition;
            struct Statement *then_branch;
            struct Statement *else_branch;
        } if_statement;  /**< If statement data */
        struct {
            Expression *condition;
            struct Statement *body;
        } while_statement; /**< While loop data */
        struct {
            Expression *initializer;
            Expression *condition;
            Expression *increment;
            struct Statement *body;
        } for_statement; /**< For loop data */
        struct {
            struct Statement **statements;
            int statement_count;
        } block;        /**< Block statement data */
        struct {
            Type *type;
            char *name;
            Expression *initializer;
        } declaration;  /**< Declaration statement data */
    } value;           /**< Union of all possible statement values */
} Statement;

/**
 * Function definition in the AST
 */
typedef struct {
    char *name;                /**< Function name */
    Type *return_type;         /**< Return type */
    Type **parameter_types;    /**< Array of parameter types */
    char **parameter_names;    /**< Array of parameter names */
    int parameter_count;       /**< Number of parameters */
    Statement *body;           /**< Function body */
} Function;

/**
 * Complete program in the AST
 */
typedef struct {
    Function **functions;      /**< Array of functions */
    int function_count;        /**< Number of functions */
    int function_capacity;     /**< Capacity of functions array */
} Program;

/**
 * Create a binary expression node
 * @param left Left operand
 * @param right Right operand
 * @param operator Operator token type
 * @param result_type Result data type
 * @return New binary expression node
 */
Expression *create_binary_expr(Expression *left, Expression *right, TokenType operator, Type *result_type);

/**
 * Create an integer literal expression node
 * @param value Integer value
 * @return New integer literal expression node
 */
Expression *create_literal_int_expr(int value);

/**
 * Create a floating-point literal expression node
 * @param value Float value
 * @return New float literal expression node
 */
Expression *create_literal_float_expr(float value);

/**
 * Create a variable reference expression node
 * @param name Variable name
 * @param type Variable type
 * @return New variable reference expression node
 */
Expression *create_variable_expr(const char *name, Type *type);

/**
 * Create a function call expression node
 * @param function_name Function name
 * @param arguments Array of argument expressions
 * @param argument_count Number of arguments
 * @return New function call expression node
 */
Expression *create_call_expr(const char *function_name, Expression **arguments, int argument_count);

/**
 * Create an assignment expression node
 * @param variable_name Variable name
 * @param value Value expression to assign
 * @param type Result type
 * @return New assignment expression node
 */
Expression *create_assign_expr(const char *variable_name, Expression *value, Type *type);

/**
 * Create a unary expression node
 * @param operand Operand expression
 * @param operator Operator token type
 * @param result_type Result data type
 * @return New unary expression node
 */
Expression *create_unary_expr(Expression *operand, TokenType operator, Type *result_type);

/**
 * Create a subscript (array access) expression node
 * @param array Array expression
 * @param index Index expression
 * @param element_type Element type of the array
 * @return New subscript expression node
 */
Expression *create_subscript_expr(Expression *array, Expression *index, Type *element_type);

/**
 * Create a cast expression node
 * @param cast_type Target type
 * @param expression Expression to cast
 * @return New cast expression node
 */
Expression *create_cast_expr(Type *cast_type, Expression *expression);

/**
 * Create an expression statement node
 * @param expr Expression
 * @return New expression statement node
 */
Statement *create_expression_stmt(Expression *expr);

/**
 * Create a return statement node
 * @param expr Return value expression
 * @return New return statement node
 */
Statement *create_return_stmt(Expression *expr);

/**
 * Create an if statement node
 * @param condition Condition expression
 * @param then_branch Statement for "then" branch
 * @param else_branch Statement for "else" branch (can be NULL)
 * @return New if statement node
 */
Statement *create_if_stmt(Expression *condition, Statement *then_branch, Statement *else_branch);

/**
 * Create a while statement node
 * @param condition Loop condition expression
 * @param body Loop body statement
 * @return New while statement node
 */
Statement *create_while_stmt(Expression *condition, Statement *body);

/**
 * Create a for statement node
 * @param initializer Initialization expression
 * @param condition Loop condition expression
 * @param increment Increment expression
 * @param body Loop body statement
 * @return New for statement node
 */
Statement *create_for_stmt(Expression *initializer, Expression *condition,
                         Expression *increment, Statement *body);

/**
 * Create a block statement node
 * @param statements Array of statements
 * @param statement_count Number of statements
 * @return New block statement node
 */
Statement *create_block_stmt(Statement **statements, int statement_count);

/**
 * Create a variable declaration statement node
 * @param type Variable type
 * @param name Variable name
 * @param initializer Initial value expression (can be NULL)
 * @return New declaration statement node
 */
Statement *create_declaration_stmt(Type *type, const char *name, Expression *initializer);

/**
 * Create a function node
 * @param return_type Function return type
 * @param name Function name
 * @param parameter_types Array of parameter types
 * @param parameter_names Array of parameter names
 * @param parameter_count Number of parameters
 * @param body Function body statement
 * @return New function node
 */
Function *create_function(Type *return_type, const char *name,
                        Type **parameter_types, char **parameter_names,
                        int parameter_count, Statement *body);

/**
 * Create a program node
 * @return New empty program node
 */
Program *create_program();

/**
 * Add a function to a program
 * @param program Program to add to
 * @param function Function to add
 */
void add_function(Program *program, Function *function);

/**
 * Free an expression and all its resources
 * @param expr Expression to free
 */
void free_expression(Expression *expr);

/**
 * Free a statement and all its resources
 * @param stmt Statement to free
 */
void free_statement(Statement *stmt);

/**
 * Free a function and all its resources
 * @param function Function to free
 */
void free_function(Function *function);

/**
 * Free a program and all its resources
 * @param program Program to free
 */
void free_program(Program *program);

#endif /* AST_H */