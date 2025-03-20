/**
 * @file ast.c
 * @brief Implementation of Abstract Syntax Tree functions
 */

#include "../include/ast.h"
#include "../include/arena.h"
#include <stdlib.h>
#include <string.h>

Program* ast_create_program(struct Arena* arena) {
  Program* program = arena_alloc(arena, sizeof(Program));
  program->declarations = NULL;
  program->count = 0;
  return program;
}

void ast_add_declaration(Program* program, Decl* decl, struct Arena* arena) {
  // Allocate or resize declarations array
  if (program->declarations == NULL) {
    program->declarations = arena_alloc(arena, sizeof(Decl*) * 8);
  } else if (program->count % 8 == 0) {
    Decl** new_decls = arena_alloc(arena, sizeof(Decl*) * (program->count + 8));
    memcpy(new_decls, program->declarations, sizeof(Decl*) * program->count);
    program->declarations = new_decls;
  }
  
  // Add declaration
  program->declarations[program->count++] = decl;
}

Expr* ast_create_expr(ExprType type, SourceLocation location, struct Arena* arena) {
  Expr* expr = arena_alloc(arena, sizeof(Expr));
  expr->type = type;
  expr->result_type = NULL;
  expr->location = location;
  return expr;
}

Stmt* ast_create_stmt(StmtType type, SourceLocation location, struct Arena* arena) {
  Stmt* stmt = arena_alloc(arena, sizeof(Stmt));
  stmt->type = type;
  stmt->location = location;
  return stmt;
}

Type* ast_create_type(TypeKind kind, SourceLocation location, struct Arena* arena) {
  Type* type = arena_alloc(arena, sizeof(Type));
  type->kind = kind;
  type->is_const = false;
  type->is_volatile = false;
  type->is_signed = false;
  type->location = location;
  
  // Initialize union fields to zero
  memset(&type->as, 0, sizeof(type->as));
  
  return type;
}

Decl* ast_create_decl(DeclType type, const char* name, SourceLocation location, struct Arena* arena) {
  Decl* decl = arena_alloc(arena, sizeof(Decl));
  decl->type = type;
  decl->name = arena_strdup(arena, name);
  decl->location = location;
  decl->is_extern = false;
  decl->is_static = false;
  decl->declared_type = NULL;
  
  // Initialize union fields to zero
  memset(&decl->as, 0, sizeof(decl->as));
  
  return decl;
}