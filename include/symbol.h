/**
 * @file symbol.h
 * @brief Symbol table management for the COIL C Compiler
 * 
 * Defines the symbol table structure and functions for managing variables,
 * functions, and scopes during compilation.
 */

#ifndef SYMBOL_H
#define SYMBOL_H

#include <stdbool.h>
#include "type.h"

/**
 * Symbol kinds
 */
typedef enum {
    SYMBOL_VARIABLE,   /**< Variable symbol */
    SYMBOL_FUNCTION,   /**< Function symbol */
    SYMBOL_PARAMETER,  /**< Function parameter symbol */
    SYMBOL_TYPE        /**< Type definition symbol */
} SymbolKind;

/**
 * Symbol table entry
 */
typedef struct {
    char *name;      /**< Symbol name */
    Type *type;      /**< Symbol type */
    int offset;      /**< Stack offset or register number */
    SymbolKind kind; /**< Kind of symbol */
} Symbol;

/**
 * Symbol table structure
 */
typedef struct SymbolTable {
    Symbol **symbols;           /**< Array of symbol pointers */
    int symbol_count;           /**< Number of symbols in the table */
    int capacity;               /**< Capacity of the symbol array */
    struct SymbolTable *parent; /**< Parent scope or NULL for global scope */
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
 * @param kind Kind of symbol
 * @return The added symbol
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
 * Dump the contents of a symbol table (for debugging)
 * @param table The symbol table
 * @param indent Indentation level
 */
void symbol_table_dump(SymbolTable *table, int indent);

/**
 * Free the symbol table and all symbols it contains
 * @param table The symbol table to free
 */
void symbol_table_free(SymbolTable *table);

/**
 * Check if a name is already defined in the current scope
 * @param table The symbol table
 * @param name Symbol name
 * @return true if the name is already defined, false otherwise
 */
bool symbol_table_exists(SymbolTable *table, const char *name);

/**
 * Get the number of variables in a scope
 * @param table The symbol table
 * @return Number of variables (not including functions or types)
 */
int symbol_table_variable_count(SymbolTable *table);

/**
 * @brief Check if a name is already defined in the current scope
 * 
 * @param table The symbol table
 * @param name Symbol name
 * @return true if the name is already defined, false otherwise
 */
 bool symbol_table_exists(SymbolTable *table, const char *name);

#endif /* SYMBOL_H */