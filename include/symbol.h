/**
* Symbol table management for the COIL C Compiler
*/

#ifndef SYMBOL_H
#define SYMBOL_H

#include "type.h"

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

/**
* Create a new symbol table
* @param parent Parent scope or NULL for global scope
* @return A new symbol table
*/
SymbolTable *symbol_table_create(SymbolTable *parent);

/**
* Add a symbol to the symbol table
* @param table The symbol table
* @param name Symbol name
* @param type Symbol type
* @param offset Stack offset or register number
*/
void symbol_table_add(SymbolTable *table, const char *name, Type *type, int offset);

/**
* Look up a symbol in the symbol table and parent scopes
* @param table The symbol table
* @param name Symbol name
* @return The symbol or NULL if not found
*/
Symbol *symbol_table_lookup(SymbolTable *table, const char *name);

/**
* Look up a symbol in the current scope only (no parent lookup)
* @param table The symbol table
* @param name Symbol name
* @return The symbol or NULL if not found
*/
Symbol *symbol_table_lookup_current(SymbolTable *table, const char *name);

/**
* Free the symbol table and all symbols it contains
* @param table The symbol table to free
*/
void symbol_table_free(SymbolTable *table);

#endif /* SYMBOL_H */