/**
* Symbol table implementation for the COIL C Compiler
*/

#include <stdlib.h>
#include <string.h>
#include "symbol.h"

SymbolTable *symbol_table_create(SymbolTable *parent) {
  SymbolTable *table = malloc(sizeof(SymbolTable));
  table->symbols = malloc(100 * sizeof(Symbol*)); // Arbitrary initial capacity
  table->symbol_count = 0;
  table->capacity = 100;
  table->parent = parent;
  return table;
}

void symbol_table_add(SymbolTable *table, const char *name, Type *type, int offset) {
  // Check if we need to resize
  if (table->symbol_count >= table->capacity) {
      table->capacity *= 2;
      table->symbols = realloc(table->symbols, table->capacity * sizeof(Symbol*));
  }
  
  Symbol *symbol = malloc(sizeof(Symbol));
  symbol->name = strdup(name);
  symbol->type = type;
  symbol->offset = offset;
  
  table->symbols[table->symbol_count++] = symbol;
}

Symbol *symbol_table_lookup_current(SymbolTable *table, const char *name) {
  for (int i = 0; i < table->symbol_count; i++) {
      if (strcmp(table->symbols[i]->name, name) == 0) {
          return table->symbols[i];
      }
  }
  
  return NULL;
}

Symbol *symbol_table_lookup(SymbolTable *table, const char *name) {
  // First check the current scope
  Symbol *symbol = symbol_table_lookup_current(table, name);
  if (symbol != NULL) {
      return symbol;
  }
  
  // Then check parent scopes
  if (table->parent != NULL) {
      return symbol_table_lookup(table->parent, name);
  }
  
  return NULL;
}

void symbol_table_free(SymbolTable *table) {
  for (int i = 0; i < table->symbol_count; i++) {
      free(table->symbols[i]->name);
      // Note: We don't free the type here because it may be shared
      free(table->symbols[i]);
  }
  free(table->symbols);
  free(table);
}