/**
 * @file parser.c
 * @brief Parser implementation for C source code
 */

#include "../include/parser.h"
#include "../include/arena.h"
#include <string.h>
#include <stdlib.h>

// Forward declarations for recursive parsing
static Expr* parse_expression(Parser* parser);
static Stmt* parse_statement(Parser* parser);
static Decl* parse_declaration(Parser* parser);
static Type* parse_type(Parser* parser);

// Helper functions for parser error handling
static void parser_error_at(Parser* parser, Token token, const char* message) {
  if (parser->has_error) return; // Only report the first error
  
  parser->has_error = true;
  
  // Format error message with location
  char buffer[256];
  snprintf(buffer, sizeof(buffer), "Error at %s:%d:%d: %s",
           token.location.file, token.location.line, token.location.column, message);
  
  parser->error_message = arena_strdup(parser->arena, buffer);
}

static void parser_error_current(Parser* parser, const char* message) {
  parser_error_at(parser, lexer_peek_token(parser->lexer), message);
}

static void parser_error_previous(Parser* parser, const char* message) {
  Token previous = lexer_peek_token(parser->lexer); // This isn't actually the previous token, but will do for now
  parser_error_at(parser, previous, message);
}

// Helper functions for parsing
static bool match(Parser* parser, TokenType type) {
  if (lexer_check(parser->lexer, type)) {
    lexer_next_token(parser->lexer);
    return true;
  }
  return false;
}

static bool check(Parser* parser, TokenType type) {
  return lexer_check(parser->lexer, type);
}

static Token advance(Parser* parser) {
  Token current = lexer_peek_token(parser->lexer);
  lexer_next_token(parser->lexer);
  return current;
}

static Token consume(Parser* parser, TokenType type, const char* message) {
  if (check(parser, type)) {
    return advance(parser);
  }
  
  parser_error_current(parser, message);
  return lexer_peek_token(parser->lexer);
}

// Symbol table for typedefs
typedef struct TypedefEntry {
  char* name;
  struct TypedefEntry* next;
} TypedefEntry;

typedef struct {
  TypedefEntry** entries;
  int size;
  int capacity;
  struct Arena* arena;
} TypedefTable;

static TypedefTable* typedef_table_create(struct Arena* arena) {
  TypedefTable* table = arena_alloc(arena, sizeof(TypedefTable));
  table->capacity = 16;
  table->size = 0;
  table->entries = arena_calloc(arena, sizeof(TypedefEntry*) * table->capacity);
  table->arena = arena;
  return table;
}

static void typedef_table_add(TypedefTable* table, const char* name) {
  unsigned hash = 0;
  for (const char* p = name; *p; p++) {
    hash = hash * 31 + *p;
  }
  
  unsigned index = hash % table->capacity;
  
  TypedefEntry* entry = arena_alloc(table->arena, sizeof(TypedefEntry));
  entry->name = arena_strdup(table->arena, name);
  entry->next = table->entries[index];
  table->entries[index] = entry;
  table->size++;
  
  // Resize if load factor is too high
  if (table->size > table->capacity * 0.75) {
    int old_capacity = table->capacity;
    TypedefEntry** old_entries = table->entries;
    
    table->capacity *= 2;
    table->entries = arena_calloc(table->arena, sizeof(TypedefEntry*) * table->capacity);
    table->size = 0;
    
    for (int i = 0; i < old_capacity; i++) {
      TypedefEntry* entry = old_entries[i];
      while (entry) {
        TypedefEntry* next = entry->next;
        
        // Rehash and insert
        unsigned new_hash = 0;
        for (const char* p = entry->name; *p; p++) {
          new_hash = new_hash * 31 + *p;
        }
        
        unsigned new_index = new_hash % table->capacity;
        entry->next = table->entries[new_index];
        table->entries[new_index] = entry;
        table->size++;
        
        entry = next;
      }
    }
  }
}

static bool typedef_table_contains(TypedefTable* table, const char* name) {
  unsigned hash = 0;
  for (const char* p = name; *p; p++) {
    hash = hash * 31 + *p;
  }
  
  unsigned index = hash % table->capacity;
  
  TypedefEntry* entry = table->entries[index];
  while (entry) {
    if (strcmp(entry->name, name) == 0) {
      return true;
    }
    entry = entry->next;
  }
  
  return false;
}

// Parsing functions
static Expr* parse_primary_expression(Parser* parser) {
  Token token = lexer_peek_token(parser->lexer);
  
  // Integer literal
  if (check(parser, TOKEN_INTEGER_LITERAL)) {
    advance(parser);
    
    Expr* expr = ast_create_expr(EXPR_INTEGER_LITERAL, token.location, parser->arena);
    expr->as.int_literal.value = token.value.int_value;
    return expr;
  }
  
  // Float literal
  if (check(parser, TOKEN_FLOAT_LITERAL)) {
    advance(parser);
    
    Expr* expr = ast_create_expr(EXPR_FLOAT_LITERAL, token.location, parser->arena);
    expr->as.float_literal.value = token.value.float_value;
    return expr;
  }
  
  // String literal
  if (check(parser, TOKEN_STRING_LITERAL)) {
    advance(parser);
    
    Expr* expr = ast_create_expr(EXPR_STRING_LITERAL, token.location, parser->arena);
    expr->as.string_literal.value = token.value.string_value;
    return expr;
  }
  
  // Character literal
  if (check(parser, TOKEN_CHAR_LITERAL)) {
    advance(parser);
    
    Expr* expr = ast_create_expr(EXPR_CHAR_LITERAL, token.location, parser->arena);
    expr->as.char_literal.value = token.value.char_value;
    return expr;
  }
  
  // Identifier
  if (check(parser, TOKEN_IDENTIFIER)) {
    advance(parser);
    
    Expr* expr = ast_create_expr(EXPR_IDENTIFIER, token.location, parser->arena);
    expr->as.identifier.name = arena_strdup(parser->arena, token.lexeme); // Careful, this might need length
    return expr;
  }
  
  // Parenthesized expression
  if (match(parser, TOKEN_LEFT_PAREN)) {
    Expr* expr = parse_expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after expression");
    return expr;
  }
  
  parser_error_current(parser, "Expect expression");
  return NULL;
}

static Expr* parse_unary_expression(Parser* parser) {
  // Check for unary operators
  if (match(parser, TOKEN_MINUS) || match(parser, TOKEN_PLUS) ||
      match(parser, TOKEN_TILDE) || match(parser, TOKEN_EXCLAIM) ||
      match(parser, TOKEN_AMPERSAND) || match(parser, TOKEN_STAR) ||
      match(parser, TOKEN_PLUS_PLUS) || match(parser, TOKEN_MINUS_MINUS)) {
    
    Token operator = lexer_peek_token(parser->lexer);
    TokenType op_type = operator.type;
    
    Expr* expr = ast_create_expr(EXPR_UNARY, operator.location, parser->arena);
    expr->as.unary.operator = op_type;
    expr->as.unary.operand = parse_unary_expression(parser);
    expr->as.unary.is_postfix = false;
    
    return expr;
  }
  
  // Parse primary expression
  Expr* expr = parse_primary_expression(parser);
  
  // Check for postfix operators
  while (match(parser, TOKEN_LEFT_BRACKET) || match(parser, TOKEN_LEFT_PAREN) ||
         match(parser, TOKEN_DOT) || match(parser, TOKEN_ARROW) ||
         match(parser, TOKEN_PLUS_PLUS) || match(parser, TOKEN_MINUS_MINUS)) {
    
    Token operator = lexer_peek_token(parser->lexer);
    
    if (operator.type == TOKEN_LEFT_BRACKET) {
      // Array indexing
      Expr* index = parse_expression(parser);
      consume(parser, TOKEN_RIGHT_BRACKET, "Expect ']' after array index");
      
      Expr* new_expr = ast_create_expr(EXPR_INDEX, operator.location, parser->arena);
      new_expr->as.index.array = expr;
      new_expr->as.index.index = index;
      expr = new_expr;
    }
    else if (operator.type == TOKEN_LEFT_PAREN) {
      // Function call
      Expr** arguments = NULL;
      int arg_count = 0;
      
      if (!check(parser, TOKEN_RIGHT_PAREN)) {
        do {
          // Allocate space for arguments
          if (arguments == NULL) {
            arguments = arena_alloc(parser->arena, sizeof(Expr*) * 4);
          } else if (arg_count % 4 == 0) {
            Expr** new_args = arena_alloc(parser->arena, sizeof(Expr*) * (arg_count + 4));
            memcpy(new_args, arguments, sizeof(Expr*) * arg_count);
            arguments = new_args;
          }
          
          // Parse argument
          arguments[arg_count++] = parse_expression(parser);
        } while (match(parser, TOKEN_COMMA));
      }
      
      consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after arguments");
      
      Expr* new_expr = ast_create_expr(EXPR_CALL, operator.location, parser->arena);
      new_expr->as.call.function = expr;
      new_expr->as.call.arguments = arguments;
      new_expr->as.call.arg_count = arg_count;
      expr = new_expr;
    }
    else if (operator.type == TOKEN_DOT || operator.type == TOKEN_ARROW) {
      // Struct field access
      advance(parser); // Consume . or ->
      
      Token field = consume(parser, TOKEN_IDENTIFIER, "Expect field name after '.' or '->'");
      
      Expr* new_expr = ast_create_expr(EXPR_FIELD, operator.location, parser->arena);
      new_expr->as.field.object = expr;
      new_expr->as.field.field = arena_strdup(parser->arena, field.lexeme); // Careful with length
      new_expr->as.field.is_arrow = (operator.type == TOKEN_ARROW);
      expr = new_expr;
    }
    else if (operator.type == TOKEN_PLUS_PLUS || operator.type == TOKEN_MINUS_MINUS) {
      // Postfix increment/decrement
      advance(parser); // Consume ++ or --
      
      Expr* new_expr = ast_create_expr(EXPR_UNARY, operator.location, parser->arena);
      new_expr->as.unary.operator = operator.type;
      new_expr->as.unary.operand = expr;
      new_expr->as.unary.is_postfix = true;
      expr = new_expr;
    }
  }
  
  return expr;
}

static Expr* parse_multiplicative_expression(Parser* parser) {
  Expr* expr = parse_unary_expression(parser);
  
  while (match(parser, TOKEN_STAR) || match(parser, TOKEN_SLASH) || match(parser, TOKEN_PERCENT)) {
    Token operator = lexer_peek_token(parser->lexer);
    TokenType op_type = operator.type;
    
    Expr* right = parse_unary_expression(parser);
    
    Expr* new_expr = ast_create_expr(EXPR_BINARY, operator.location, parser->arena);
    new_expr->as.binary.left = expr;
    new_expr->as.binary.operator = op_type;
    new_expr->as.binary.right = right;
    expr = new_expr;
  }
  
  return expr;
}

static Expr* parse_additive_expression(Parser* parser) {
  Expr* expr = parse_multiplicative_expression(parser);
  
  while (match(parser, TOKEN_PLUS) || match(parser, TOKEN_MINUS)) {
    Token operator = lexer_peek_token(parser->lexer);
    TokenType op_type = operator.type;
    
    Expr* right = parse_multiplicative_expression(parser);
    
    Expr* new_expr = ast_create_expr(EXPR_BINARY, operator.location, parser->arena);
    new_expr->as.binary.left = expr;
    new_expr->as.binary.operator = op_type;
    new_expr->as.binary.right = right;
    expr = new_expr;
  }
  
  return expr;
}

static Expr* parse_shift_expression(Parser* parser) {
  Expr* expr = parse_additive_expression(parser);
  
  while (match(parser, TOKEN_LESS_LESS) || match(parser, TOKEN_GREATER_GREATER)) {
    Token operator = lexer_peek_token(parser->lexer);
    TokenType op_type = operator.type;
    
    Expr* right = parse_additive_expression(parser);
    
    Expr* new_expr = ast_create_expr(EXPR_BINARY, operator.location, parser->arena);
    new_expr->as.binary.left = expr;
    new_expr->as.binary.operator = op_type;
    new_expr->as.binary.right = right;
    expr = new_expr;
  }
  
  return expr;
}

static Expr* parse_relational_expression(Parser* parser) {
  Expr* expr = parse_shift_expression(parser);
  
  while (match(parser, TOKEN_LESS) || match(parser, TOKEN_LESS_EQUAL) ||
         match(parser, TOKEN_GREATER) || match(parser, TOKEN_GREATER_EQUAL)) {
    Token operator = lexer_peek_token(parser->lexer);
    TokenType op_type = operator.type;
    
    Expr* right = parse_shift_expression(parser);
    
    Expr* new_expr = ast_create_expr(EXPR_BINARY, operator.location, parser->arena);
    new_expr->as.binary.left = expr;
    new_expr->as.binary.operator = op_type;
    new_expr->as.binary.right = right;
    expr = new_expr;
  }
  
  return expr;
}

static Expr* parse_equality_expression(Parser* parser) {
  Expr* expr = parse_relational_expression(parser);
  
  while (match(parser, TOKEN_EQUAL_EQUAL) || match(parser, TOKEN_EXCLAIM_EQUAL)) {
    Token operator = lexer_peek_token(parser->lexer);
    TokenType op_type = operator.type;
    
    Expr* right = parse_relational_expression(parser);
    
    Expr* new_expr = ast_create_expr(EXPR_BINARY, operator.location, parser->arena);
    new_expr->as.binary.left = expr;
    new_expr->as.binary.operator = op_type;
    new_expr->as.binary.right = right;
    expr = new_expr;
  }
  
  return expr;
}

static Expr* parse_bitwise_and_expression(Parser* parser) {
  Expr* expr = parse_equality_expression(parser);
  
  while (match(parser, TOKEN_AMPERSAND)) {
    Token operator = lexer_peek_token(parser->lexer);
    
    Expr* right = parse_equality_expression(parser);
    
    Expr* new_expr = ast_create_expr(EXPR_BINARY, operator.location, parser->arena);
    new_expr->as.binary.left = expr;
    new_expr->as.binary.operator = TOKEN_AMPERSAND;
    new_expr->as.binary.right = right;
    expr = new_expr;
  }
  
  return expr;
}

static Expr* parse_bitwise_xor_expression(Parser* parser) {
  Expr* expr = parse_bitwise_and_expression(parser);
  
  while (match(parser, TOKEN_CARET)) {
    Token operator = lexer_peek_token(parser->lexer);
    
    Expr* right = parse_bitwise_and_expression(parser);
    
    Expr* new_expr = ast_create_expr(EXPR_BINARY, operator.location, parser->arena);
    new_expr->as.binary.left = expr;
    new_expr->as.binary.operator = TOKEN_CARET;
    new_expr->as.binary.right = right;
    expr = new_expr;
  }
  
  return expr;
}

static Expr* parse_bitwise_or_expression(Parser* parser) {
  Expr* expr = parse_bitwise_xor_expression(parser);
  
  while (match(parser, TOKEN_PIPE)) {
    Token operator = lexer_peek_token(parser->lexer);
    
    Expr* right = parse_bitwise_xor_expression(parser);
    
    Expr* new_expr = ast_create_expr(EXPR_BINARY, operator.location, parser->arena);
    new_expr->as.binary.left = expr;
    new_expr->as.binary.operator = TOKEN_PIPE;
    new_expr->as.binary.right = right;
    expr = new_expr;
  }
  
  return expr;
}

static Expr* parse_logical_and_expression(Parser* parser) {
  Expr* expr = parse_bitwise_or_expression(parser);
  
  while (match(parser, TOKEN_AMPERSAND_AMPERSAND)) {
    Token operator = lexer_peek_token(parser->lexer);
    
    Expr* right = parse_bitwise_or_expression(parser);
    
    Expr* new_expr = ast_create_expr(EXPR_BINARY, operator.location, parser->arena);
    new_expr->as.binary.left = expr;
    new_expr->as.binary.operator = TOKEN_AMPERSAND_AMPERSAND;
    new_expr->as.binary.right = right;
    expr = new_expr;
  }
  
  return expr;
}

static Expr* parse_logical_or_expression(Parser* parser) {
  Expr* expr = parse_logical_and_expression(parser);
  
  while (match(parser, TOKEN_PIPE_PIPE)) {
    Token operator = lexer_peek_token(parser->lexer);
    
    Expr* right = parse_logical_and_expression(parser);
    
    Expr* new_expr = ast_create_expr(EXPR_BINARY, operator.location, parser->arena);
    new_expr->as.binary.left = expr;
    new_expr->as.binary.operator = TOKEN_PIPE_PIPE;
    new_expr->as.binary.right = right;
    expr = new_expr;
  }
  
  return expr;
}

static Expr* parse_conditional_expression(Parser* parser) {
  Expr* expr = parse_logical_or_expression(parser);
  
  if (match(parser, TOKEN_QUESTION)) {
    Expr* true_expr = parse_expression(parser);
    consume(parser, TOKEN_COLON, "Expect ':' in conditional expression");
    Expr* false_expr = parse_conditional_expression(parser);
    
    Expr* new_expr = ast_create_expr(EXPR_CONDITIONAL, expr->location, parser->arena);
    new_expr->as.conditional.condition = expr;
    new_expr->as.conditional.true_expr = true_expr;
    new_expr->as.conditional.false_expr = false_expr;
    expr = new_expr;
  }
  
  return expr;
}

static Expr* parse_assignment_expression(Parser* parser) {
  Expr* expr = parse_conditional_expression(parser);
  
  if (match(parser, TOKEN_EQUAL) || match(parser, TOKEN_PLUS_EQUAL) ||
      match(parser, TOKEN_MINUS_EQUAL) || match(parser, TOKEN_STAR_EQUAL) ||
      match(parser, TOKEN_SLASH_EQUAL) || match(parser, TOKEN_PERCENT_EQUAL) ||
      match(parser, TOKEN_AMPERSAND_EQUAL) || match(parser, TOKEN_PIPE_EQUAL) ||
      match(parser, TOKEN_CARET_EQUAL) || match(parser, TOKEN_LESS_LESS_EQUAL) ||
      match(parser, TOKEN_GREATER_GREATER_EQUAL)) {
    
    Token operator = lexer_peek_token(parser->lexer);
    TokenType op_type = operator.type;
    
    Expr* right = parse_assignment_expression(parser);
    
    Expr* new_expr = ast_create_expr(EXPR_ASSIGN, operator.location, parser->arena);
    new_expr->as.assign.target = expr;
    new_expr->as.assign.operator = op_type;
    new_expr->as.assign.value = right;
    expr = new_expr;
  }
  
  return expr;
}

static Expr* parse_expression(Parser* parser) {
  return parse_assignment_expression(parser);
}

// Parse a statement
static Stmt* parse_expression_statement(Parser* parser) {
  Expr* expr = parse_expression(parser);
  consume(parser, TOKEN_SEMICOLON, "Expect ';' after expression");
  
  Stmt* stmt = ast_create_stmt(STMT_EXPR, expr->location, parser->arena);
  stmt->as.expr.expr = expr;
  return stmt;
}

static Stmt* parse_block_statement(Parser* parser) {
  Token start = consume(parser, TOKEN_LEFT_BRACE, "Expect '{' before block");
  
  Stmt** statements = NULL;
  int count = 0;
  
  while (!check(parser, TOKEN_RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
    // Allocate space for statements
    if (statements == NULL) {
      statements = arena_alloc(parser->arena, sizeof(Stmt*) * 8);
    } else if (count % 8 == 0) {
      Stmt** new_stmts = arena_alloc(parser->arena, sizeof(Stmt*) * (count + 8));
      memcpy(new_stmts, statements, sizeof(Stmt*) * count);
      statements = new_stmts;
    }
    
    // Parse statement
    statements[count++] = parse_statement(parser);
  }
  
  consume(parser, TOKEN_RIGHT_BRACE, "Expect '}' after block");
  
  Stmt* stmt = ast_create_stmt(STMT_BLOCK, start.location, parser->arena);
  stmt->as.block.statements = statements;
  stmt->as.block.count = count;
  return stmt;
}

static Stmt* parse_if_statement(Parser* parser) {
  Token start = consume(parser, TOKEN_IF, "Expect 'if'");
  consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after 'if'");
  Expr* condition = parse_expression(parser);
  consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after condition");
  
  Stmt* then_branch = parse_statement(parser);
  Stmt* else_branch = NULL;
  
  if (match(parser, TOKEN_ELSE)) {
    else_branch = parse_statement(parser);
  }
  
  Stmt* stmt = ast_create_stmt(STMT_IF, start.location, parser->arena);
  stmt->as.if_stmt.condition = condition;
  stmt->as.if_stmt.then_branch = then_branch;
  stmt->as.if_stmt.else_branch = else_branch;
  return stmt;
}

static Stmt* parse_while_statement(Parser* parser) {
  Token start = consume(parser, TOKEN_WHILE, "Expect 'while'");
  consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after 'while'");
  Expr* condition = parse_expression(parser);
  consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after condition");
  
  Stmt* body = parse_statement(parser);
  
  Stmt* stmt = ast_create_stmt(STMT_WHILE, start.location, parser->arena);
  stmt->as.while_stmt.condition = condition;
  stmt->as.while_stmt.body = body;
  return stmt;
}

static Stmt* parse_return_statement(Parser* parser) {
  Token start = consume(parser, TOKEN_RETURN, "Expect 'return'");
  
  Expr* value = NULL;
  if (!check(parser, TOKEN_SEMICOLON)) {
    value = parse_expression(parser);
  }
  
  consume(parser, TOKEN_SEMICOLON, "Expect ';' after return value");
  
  Stmt* stmt = ast_create_stmt(STMT_RETURN, start.location, parser->arena);
  stmt->as.return_stmt.value = value;
  return stmt;
}

static Stmt* parse_statement(Parser* parser) {
  if (match(parser, TOKEN_LEFT_BRACE)) {
    // Undo the consumption of '{'
    lexer_peek_token(parser->lexer); // Not ideal, we need a proper way to go back
    return parse_block_statement(parser);
  }
  
  if (match(parser, TOKEN_IF)) {
    // Undo the consumption of 'if'
    lexer_peek_token(parser->lexer); // Not ideal
    return parse_if_statement(parser);
  }
  
  if (match(parser, TOKEN_WHILE)) {
    // Undo the consumption of 'while'
    lexer_peek_token(parser->lexer); // Not ideal
    return parse_while_statement(parser);
  }
  
  if (match(parser, TOKEN_RETURN)) {
    // Undo the consumption of 'return'
    lexer_peek_token(parser->lexer); // Not ideal
    return parse_return_statement(parser);
  }
  
  // Check if it's a declaration
  if (match(parser, TOKEN_INT) || match(parser, TOKEN_CHAR) ||
      match(parser, TOKEN_FLOAT) || match(parser, TOKEN_DOUBLE) ||
      match(parser, TOKEN_VOID) || match(parser, TOKEN_STRUCT) ||
      match(parser, TOKEN_UNION) || match(parser, TOKEN_TYPEDEF) ||
      match(parser, TOKEN_STATIC) || match(parser, TOKEN_EXTERN)) {
    
    // Undo the consumption
    lexer_peek_token(parser->lexer); // Not ideal
    
    Decl* decl = parse_declaration(parser);
    
    Stmt* stmt = ast_create_stmt(STMT_DECL, decl->location, parser->arena);
    stmt->as.decl_stmt.decl = decl;
    return stmt;
  }
  
  // Default to expression statement
  return parse_expression_statement(parser);
}

// Parse a declaration
static Type* parse_basic_type(Parser* parser) {
  Token token = lexer_peek_token(parser->lexer);
  SourceLocation location = token.location;
  Type* type = NULL;
  
  if (match(parser, TOKEN_VOID)) {
    type = ast_create_type(TYPE_VOID, location, parser->arena);
  } else if (match(parser, TOKEN_CHAR)) {
    type = ast_create_type(TYPE_CHAR, location, parser->arena);
    type->is_signed = true; // Default to signed char
  } else if (match(parser, TOKEN_SHORT)) {
    type = ast_create_type(TYPE_SHORT, location, parser->arena);
    type->is_signed = true; // Default to signed short
  } else if (match(parser, TOKEN_INT)) {
    type = ast_create_type(TYPE_INT, location, parser->arena);
    type->is_signed = true; // Default to signed int
  } else if (match(parser, TOKEN_LONG)) {
    type = ast_create_type(TYPE_LONG, location, parser->arena);
    type->is_signed = true; // Default to signed long
  } else if (match(parser, TOKEN_FLOAT)) {
    type = ast_create_type(TYPE_FLOAT, location, parser->arena);
  } else if (match(parser, TOKEN_DOUBLE)) {
    type = ast_create_type(TYPE_DOUBLE, location, parser->arena);
  } else {
    parser_error_current(parser, "Expect type");
    return NULL;
  }
  
  // Handle modifiers if any
  if (match(parser, TOKEN_CONST)) {
    type->is_const = true;
  }
  if (match(parser, TOKEN_VOLATILE)) {
    type->is_volatile = true;
  }
  
  return type;
}

static Type* parse_type(Parser* parser) {
  // Parse basic type or typedef name
  Type* type = parse_basic_type(parser);
  
  // Parse derived types
  while (match(parser, TOKEN_STAR) || match(parser, TOKEN_LEFT_BRACKET)) {
    if (lexer_peek_token(parser->lexer).type == TOKEN_STAR) {
      // Pointer type
      Type* ptr_type = ast_create_type(TYPE_POINTER, type->location, parser->arena);
      ptr_type->as.pointer.element_type = type;
      type = ptr_type;
    } else {
      // Array type
      Type* array_type = ast_create_type(TYPE_ARRAY, type->location, parser->arena);
      array_type->as.array.element_type = type;
      
      if (match(parser, TOKEN_RIGHT_BRACKET)) {
        // Unsized array
        array_type->as.array.size = -1;
      } else {
        // Parse array size
        Expr* size_expr = parse_expression(parser);
        
        // In a full implementation, we would evaluate the constant expression
        // For now, we'll just assume it's a simple integer literal
        if (size_expr->type == EXPR_INTEGER_LITERAL) {
          array_type->as.array.size = (int)size_expr->as.int_literal.value;
        } else {
          array_type->as.array.size = -1; // Variable-length array
        }
        
        consume(parser, TOKEN_RIGHT_BRACKET, "Expect ']' after array size");
      }
      
      type = array_type;
    }
  }
  
  return type;
}

static Decl* parse_variable_declaration(Parser* parser, Type* type, const char* name, bool is_static, bool is_extern) {
  SourceLocation location = lexer_location(parser->lexer);
  
  Decl* decl = ast_create_decl(DECL_VAR, name, location, parser->arena);
  decl->declared_type = type;
  decl->is_static = is_static;
  decl->is_extern = is_extern;
  
  // Check for initializer
  if (match(parser, TOKEN_EQUAL)) {
    decl->as.var.initializer = parse_expression(parser);
  } else {
    decl->as.var.initializer = NULL;
  }
  
  consume(parser, TOKEN_SEMICOLON, "Expect ';' after variable declaration");
  
  return decl;
}

static Decl* parse_function_declaration(Parser* parser, Type* return_type, const char* name, bool is_static, bool is_extern) {
  SourceLocation location = lexer_location(parser->lexer);
  
  // Create function type
  Type* func_type = ast_create_type(TYPE_FUNCTION, location, parser->arena);
  func_type->as.function.return_type = return_type;
  func_type->as.function.param_types = NULL;
  func_type->as.function.param_names = NULL;
  func_type->as.function.param_count = 0;
  func_type->as.function.is_variadic = false;
  
  // Parse parameters
  consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after function name");
  
  if (!check(parser, TOKEN_RIGHT_PAREN)) {
    // At least one parameter
    do {
      // Parse parameter type
      Type* param_type = parse_type(parser);
      
      // Parse parameter name (optional)
      char* param_name = NULL;
      if (check(parser, TOKEN_IDENTIFIER)) {
        Token name_token = advance(parser);
        param_name = arena_strdup(parser->arena, name_token.lexeme); // Careful with length
      }
      
      // Add parameter to function type
      if (func_type->as.function.param_types == NULL) {
        func_type->as.function.param_types = arena_alloc(parser->arena, sizeof(Type*) * 4);
        func_type->as.function.param_names = arena_alloc(parser->arena, sizeof(char*) * 4);
      } else if (func_type->as.function.param_count % 4 == 0) {
        Type** new_types = arena_alloc(parser->arena, sizeof(Type*) * (func_type->as.function.param_count + 4));
        char** new_names = arena_alloc(parser->arena, sizeof(char*) * (func_type->as.function.param_count + 4));
        
        memcpy(new_types, func_type->as.function.param_types, sizeof(Type*) * func_type->as.function.param_count);
        memcpy(new_names, func_type->as.function.param_names, sizeof(char*) * func_type->as.function.param_count);
        
        func_type->as.function.param_types = new_types;
        func_type->as.function.param_names = new_names;
      }
      
      func_type->as.function.param_types[func_type->as.function.param_count] = param_type;
      func_type->as.function.param_names[func_type->as.function.param_count] = param_name;
      func_type->as.function.param_count++;
      
    } while (match(parser, TOKEN_COMMA));
  }
  
  consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after parameters");
  
  // Create function declaration
  Decl* decl = ast_create_decl(DECL_FUNC, name, location, parser->arena);
  decl->declared_type = func_type;
  decl->is_static = is_static;
  decl->is_extern = is_extern;
  decl->as.func.param_names = func_type->as.function.param_names;
  
  // Parse function body if present (not just a declaration)
  if (match(parser, TOKEN_LEFT_BRACE)) {
    // Undo the consumption of '{'
    lexer_peek_token(parser->lexer); // Not ideal
    
    decl->as.func.body = parse_block_statement(parser);
  } else {
    // Function prototype
    decl->as.func.body = NULL;
    consume(parser, TOKEN_SEMICOLON, "Expect ';' after function prototype");
  }
  
  return decl;
}

static Decl* parse_declaration(Parser* parser) {
  // Parse storage class if present
  bool is_static = false;
  bool is_extern = false;
  
  if (match(parser, TOKEN_STATIC)) {
    is_static = true;
  } else if (match(parser, TOKEN_EXTERN)) {
    is_extern = true;
  }
  
  // Parse type
  Type* type = parse_type(parser);
  
  // Parse declarator
  Token name_token = consume(parser, TOKEN_IDENTIFIER, "Expect identifier name");
  char* name = arena_strdup(parser->arena, name_token.lexeme); // Careful with length
  
  // Determine if it's a variable or function declaration
  if (check(parser, TOKEN_LEFT_PAREN)) {
    return parse_function_declaration(parser, type, name, is_static, is_extern);
  } else {
    return parse_variable_declaration(parser, type, name, is_static, is_extern);
  }
}

// Parse a complete program
static Program* parse_program(Parser* parser) {
  Program* program = ast_create_program(parser->arena);
  
  while (!check(parser, TOKEN_EOF)) {
    Decl* decl = parse_declaration(parser);
    ast_add_declaration(program, decl, parser->arena);
  }
  
  return program;
}

// Public interface
Parser* parser_create(Lexer* lexer, struct Arena* arena) {
  Parser* parser = arena_alloc(arena, sizeof(Parser));
  parser->lexer = lexer;
  parser->arena = arena;
  parser->program = NULL;
  parser->has_error = false;
  parser->error_message = NULL;
  parser->typedefs = NULL; // In a full implementation, initialize the typedef table
  
  return parser;
}

Program* parser_parse_program(Parser* parser) {
  parser->program = parse_program(parser);
  return parser->program;
}

Decl* parser_parse_declaration(Parser* parser) {
  return parse_declaration(parser);
}

Stmt* parser_parse_statement(Parser* parser) {
  return parse_statement(parser);
}

Expr* parser_parse_expression(Parser* parser) {
  return parse_expression(parser);
}

Type* parser_parse_type(Parser* parser) {
  return parse_type(parser);
}

const char* parser_error(Parser* parser) {
  if (parser->has_error) {
    return parser->error_message;
  }
  
  const char* lexer_err = lexer_error(parser->lexer);
  if (lexer_err) {
    return lexer_err;
  }
  
  return NULL;
}