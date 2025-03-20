/**
 * @file ast.h
 * @brief Abstract Syntax Tree definitions for C language
 */

#ifndef AST_H
#define AST_H

#include <stdbool.h>

// Forward declarations
typedef struct Expr Expr;
typedef struct Stmt Stmt;
typedef struct Type Type;
typedef struct Decl Decl;

/**
 * @brief Source location for error reporting
 */
typedef struct {
  const char* file;
  int line;
  int column;
} SourceLocation;

/**
 * @brief Token types for C language
 */
typedef enum {
  TOKEN_EOF,
  TOKEN_IDENTIFIER,
  TOKEN_INTEGER_LITERAL,
  TOKEN_FLOAT_LITERAL,
  TOKEN_STRING_LITERAL,
  TOKEN_CHAR_LITERAL,
  
  // Operators
  TOKEN_PLUS,        // +
  TOKEN_MINUS,       // -
  TOKEN_STAR,        // *
  TOKEN_SLASH,       // /
  TOKEN_PERCENT,     // %
  TOKEN_AMPERSAND,   // &
  TOKEN_PIPE,        // |
  TOKEN_CARET,       // ^
  TOKEN_TILDE,       // ~
  TOKEN_EXCLAIM,     // !
  TOKEN_EQUAL,       // =
  TOKEN_LESS,        // <
  TOKEN_GREATER,     // >
  
  // Compound operators
  TOKEN_PLUS_EQUAL,  // +=
  TOKEN_MINUS_EQUAL, // -=
  TOKEN_STAR_EQUAL,  // *=
  TOKEN_SLASH_EQUAL, // /=
  TOKEN_PERCENT_EQUAL, // %=
  TOKEN_AMPERSAND_EQUAL, // &=
  TOKEN_PIPE_EQUAL,  // |=
  TOKEN_CARET_EQUAL, // ^=
  TOKEN_EQUAL_EQUAL, // ==
  TOKEN_EXCLAIM_EQUAL, // !=
  TOKEN_LESS_EQUAL,  // <=
  TOKEN_GREATER_EQUAL, // >=
  TOKEN_LESS_LESS,   // <<
  TOKEN_GREATER_GREATER, // >>
  TOKEN_PLUS_PLUS,   // ++
  TOKEN_MINUS_MINUS, // --
  TOKEN_AMPERSAND_AMPERSAND, // &&
  TOKEN_PIPE_PIPE,   // ||
  
  // Punctuation
  TOKEN_LEFT_PAREN,  // (
  TOKEN_RIGHT_PAREN, // )
  TOKEN_LEFT_BRACE,  // {
  TOKEN_RIGHT_BRACE, // }
  TOKEN_LEFT_BRACKET, // [
  TOKEN_RIGHT_BRACKET, // ]
  TOKEN_SEMICOLON,   // ;
  TOKEN_COMMA,       // ,
  TOKEN_DOT,         // .
  TOKEN_ARROW,       // ->
  TOKEN_QUESTION,    // ?
  TOKEN_COLON,       // :
  
  // Keywords
  TOKEN_AUTO,
  TOKEN_BREAK,
  TOKEN_CASE,
  TOKEN_CHAR,
  TOKEN_CONST,
  TOKEN_CONTINUE,
  TOKEN_DEFAULT,
  TOKEN_DO,
  TOKEN_DOUBLE,
  TOKEN_ELSE,
  TOKEN_ENUM,
  TOKEN_EXTERN,
  TOKEN_FLOAT,
  TOKEN_FOR,
  TOKEN_GOTO,
  TOKEN_IF,
  TOKEN_INT,
  TOKEN_LONG,
  TOKEN_REGISTER,
  TOKEN_RETURN,
  TOKEN_SHORT,
  TOKEN_SIGNED,
  TOKEN_SIZEOF,
  TOKEN_STATIC,
  TOKEN_STRUCT,
  TOKEN_SWITCH,
  TOKEN_TYPEDEF,
  TOKEN_UNION,
  TOKEN_UNSIGNED,
  TOKEN_VOID,
  TOKEN_VOLATILE,
  TOKEN_WHILE
} TokenType;

/**
 * @brief Token structure representing a lexical unit
 */
typedef struct {
  TokenType type;
  const char* lexeme;
  int length;
  SourceLocation location;
  
  // Value for literals
  union {
    long long int_value;
    double float_value;
    char* string_value;
    char char_value;
  } value;
} Token;

/**
 * @brief Type kinds for C type system
 */
typedef enum {
  TYPE_VOID,
  TYPE_BOOL,
  TYPE_CHAR,
  TYPE_SHORT,
  TYPE_INT,
  TYPE_LONG,
  TYPE_FLOAT,
  TYPE_DOUBLE,
  TYPE_POINTER,
  TYPE_ARRAY,
  TYPE_STRUCT,
  TYPE_UNION,
  TYPE_FUNCTION,
  TYPE_ENUM
} TypeKind;

/**
 * @brief Type representation for C types
 */
struct Type {
  TypeKind kind;
  bool is_const;
  bool is_volatile;
  bool is_signed;
  SourceLocation location;
  
  union {
    // For pointer types
    struct {
      Type* element_type;
    } pointer;
    
    // For array types
    struct {
      Type* element_type;
      int size; // -1 for unknown size
    } array;
    
    // For struct/union types
    struct {
      char* name;
      struct {
        char* name;
        Type* type;
        int offset; // Byte offset within the struct
      }* fields;
      int field_count;
      int size; // Total size in bytes
      bool is_complete; // True if fields are known
    } structure;
    
    // For function types
    struct {
      Type* return_type;
      Type** param_types;
      char** param_names;
      int param_count;
      bool is_variadic;
    } function;
    
    // For enum types
    struct {
      char* name;
      struct {
        char* name;
        int value;
      }* enumerators;
      int enumerator_count;
    } enumeration;
  } as;
};

/**
 * @brief Expression type enum for AST nodes
 */
typedef enum {
  EXPR_BINARY,
  EXPR_UNARY,
  EXPR_INTEGER_LITERAL,
  EXPR_FLOAT_LITERAL,
  EXPR_STRING_LITERAL,
  EXPR_CHAR_LITERAL,
  EXPR_IDENTIFIER,
  EXPR_CALL,
  EXPR_INDEX,
  EXPR_FIELD,
  EXPR_ASSIGN,
  EXPR_CONDITIONAL,
  EXPR_SIZEOF,
  EXPR_CAST
} ExprType;

/**
 * @brief Expression AST node
 */
struct Expr {
  ExprType type;
  Type* result_type;  // Type after semantic analysis
  SourceLocation location;
  
  union {
    struct {
      Expr* left;
      TokenType operator;
      Expr* right;
    } binary;
    
    struct {
      TokenType operator;
      Expr* operand;
      bool is_postfix;
    } unary;
    
    struct {
      long long value;
    } int_literal;
    
    struct {
      double value;
    } float_literal;
    
    struct {
      char* value;
    } string_literal;
    
    struct {
      char value;
    } char_literal;
    
    struct {
      char* name;
    } identifier;
    
    struct {
      Expr* function;
      Expr** arguments;
      int arg_count;
    } call;
    
    struct {
      Expr* array;
      Expr* index;
    } index;
    
    struct {
      Expr* object;
      char* field;
      bool is_arrow; // true for -> operator, false for . operator
    } field;
    
    struct {
      Expr* target;
      TokenType operator; // =, +=, -=, etc.
      Expr* value;
    } assign;
    
    struct {
      Expr* condition;
      Expr* true_expr;
      Expr* false_expr;
    } conditional;
    
    struct {
      Type* type;
      Expr* expr;
    } cast;
    
    struct {
      Type* type;
    } size_of;
  } as;
};

/**
 * @brief Statement type enum for AST nodes
 */
typedef enum {
  STMT_EXPR,
  STMT_BLOCK,
  STMT_IF,
  STMT_SWITCH,
  STMT_WHILE,
  STMT_DO_WHILE,
  STMT_FOR,
  STMT_GOTO,
  STMT_CONTINUE,
  STMT_BREAK,
  STMT_RETURN,
  STMT_LABEL,
  STMT_DECL
} StmtType;

/**
 * @brief Statement AST node
 */
struct Stmt {
  StmtType type;
  SourceLocation location;
  
  union {
    struct {
      Expr* expr;
    } expr;
    
    struct {
      Stmt** statements;
      int count;
    } block;
    
    struct {
      Expr* condition;
      Stmt* then_branch;
      Stmt* else_branch;
    } if_stmt;
    
    struct {
      Expr* condition;
      Stmt** cases;
      int case_count;
      Stmt* default_case; // NULL if no default
    } switch_stmt;
    
    struct {
      Expr* condition;
      Stmt* body;
    } while_stmt;
    
    struct {
      Stmt* body;
      Expr* condition;
    } do_while_stmt;
    
    struct {
      Stmt* init; // Can be NULL
      Expr* condition; // Can be NULL
      Expr* update; // Can be NULL
      Stmt* body;
    } for_stmt;
    
    struct {
      char* label;
    } goto_stmt;
    
    struct {
      char* name;
      Stmt* statement;
    } label_stmt;
    
    struct {
      Expr* value; // Can be NULL for void return
    } return_stmt;
    
    struct {
      Decl* decl;
    } decl_stmt;
  } as;
};

/**
 * @brief Declaration type enum
 */
typedef enum {
  DECL_VAR,
  DECL_FUNC,
  DECL_STRUCT,
  DECL_UNION,
  DECL_ENUM,
  DECL_TYPEDEF
} DeclType;

/**
 * @brief Declaration AST node
 */
struct Decl {
  DeclType type;
  char* name;
  SourceLocation location;
  bool is_extern;
  bool is_static;
  Type* declared_type;
  
  union {
    struct {
      Expr* initializer; // Can be NULL
    } var;
    
    struct {
      char** param_names;
      Stmt* body; // NULL for declarations without body
    } func;
    
    struct {
      // Fields are in the Type structure
    } struct_decl;
    
    struct {
      // Fields are in the Type structure
    } union_decl;
    
    struct {
      // Enumerators are in the Type structure
    } enum_decl;
    
    struct {
      // Original type is in declared_type
    } typedef_decl;
  } as;
};

/**
 * @brief Complete AST for a C program
 */
typedef struct {
  Decl** declarations;
  int count;
} Program;

/**
 * @brief Create a new program AST node
 * @param arena Memory arena for allocation
 * @return New program node
 */
Program* ast_create_program(struct Arena* arena);

/**
 * @brief Add a declaration to a program
 * @param program The program to add to
 * @param decl The declaration to add
 * @param arena Memory arena for allocation
 */
void ast_add_declaration(Program* program, Decl* decl, struct Arena* arena);

/**
 * @brief Create a new expression node
 * @param type Expression type
 * @param location Source location
 * @param arena Memory arena for allocation
 * @return New expression node
 */
Expr* ast_create_expr(ExprType type, SourceLocation location, struct Arena* arena);

/**
 * @brief Create a new statement node
 * @param type Statement type
 * @param location Source location
 * @param arena Memory arena for allocation
 * @return New statement node
 */
Stmt* ast_create_stmt(StmtType type, SourceLocation location, struct Arena* arena);

/**
 * @brief Create a new type node
 * @param kind Type kind
 * @param location Source location
 * @param arena Memory arena for allocation
 * @return New type node
 */
Type* ast_create_type(TypeKind kind, SourceLocation location, struct Arena* arena);

/**
 * @brief Create a new declaration node
 * @param type Declaration type
 * @param name Identifier name
 * @param location Source location
 * @param arena Memory arena for allocation
 * @return New declaration node
 */
Decl* ast_create_decl(DeclType type, const char* name, SourceLocation location, struct Arena* arena);

#endif /* AST_H */