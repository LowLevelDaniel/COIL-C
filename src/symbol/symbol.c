/**
 * Enhanced Symbol Table Implementation for the COIL C Compiler
 * 
 * This symbol table implementation manages variable and function 
 * information including scopes, types, and stack offsets.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "symbol.h"
#include "type.h"

/**
 * Create a new symbol table
 * @param parent Parent scope or NULL for global scope
 * @return A new symbol table
 */
SymbolTable *symbol_table_create(SymbolTable *parent) {
    SymbolTable *table = malloc(sizeof(SymbolTable));
    if (table == NULL) {
        fprintf(stderr, "Memory allocation failed for symbol table\n");
        exit(1);
    }
    
    table->symbols = malloc(100 * sizeof(Symbol*)); // Initial capacity
    if (table->symbols == NULL) {
        fprintf(stderr, "Memory allocation failed for symbol table entries\n");
        free(table);
        exit(1);
    }
    
    table->symbol_count = 0;
    table->capacity = 100;
    table->parent = parent;
    
    return table;
}

/**
 * Add a symbol to the symbol table
 * @param table The symbol table
 * @param name Symbol name
 * @param type Symbol type
 * @param offset Stack offset or register number
 */
void symbol_table_add(SymbolTable *table, const char *name, Type *type, int offset) {
    // Check if symbol already exists in the current scope
    Symbol *existing = symbol_table_lookup_current(table, name);
    if (existing != NULL) {
        fprintf(stderr, "Error: Symbol '%s' already defined in the current scope\n", name);
        exit(1);
    }
    
    // Check if we need to resize the symbol table
    if (table->symbol_count >= table->capacity) {
        table->capacity *= 2;
        Symbol **new_symbols = realloc(table->symbols, table->capacity * sizeof(Symbol*));
        if (new_symbols == NULL) {
            fprintf(stderr, "Memory reallocation failed for symbol table\n");
            exit(1);
        }
        table->symbols = new_symbols;
    }
    
    // Create the new symbol
    Symbol *symbol = malloc(sizeof(Symbol));
    if (symbol == NULL) {
        fprintf(stderr, "Memory allocation failed for symbol\n");
        exit(1);
    }
    
    symbol->name = strdup(name);
    if (symbol->name == NULL) {
        fprintf(stderr, "Memory allocation failed for symbol name\n");
        free(symbol);
        exit(1);
    }
    
    symbol->type = type;
    symbol->offset = offset;
    
    // Add the symbol to the table
    table->symbols[table->symbol_count++] = symbol;
}

/**
 * Look up a symbol in the current scope only (no parent lookup)
 * @param table The symbol table
 * @param name Symbol name
 * @return The symbol or NULL if not found
 */
Symbol *symbol_table_lookup_current(SymbolTable *table, const char *name) {
    if (table == NULL) {
        return NULL;
    }
    
    for (int i = 0; i < table->symbol_count; i++) {
        if (strcmp(table->symbols[i]->name, name) == 0) {
            return table->symbols[i];
        }
    }
    
    return NULL;
}

/**
 * Look up a symbol in the symbol table and parent scopes
 * @param table The symbol table
 * @param name Symbol name
 * @return The symbol or NULL if not found
 */
Symbol *symbol_table_lookup(SymbolTable *table, const char *name) {
    if (table == NULL) {
        return NULL;
    }
    
    // First check the current scope
    Symbol *symbol = symbol_table_lookup_current(table, name);
    if (symbol != NULL) {
        return symbol;
    }
    
    // Then check parent scopes
    return symbol_table_lookup(table->parent, name);
}

/**
 * Dump the contents of a symbol table (for debugging)
 * @param table The symbol table
 * @param indent Indentation level
 */
void symbol_table_dump(SymbolTable *table, int indent) {
    if (table == NULL) {
        return;
    }
    
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
    printf("Symbol Table (%d symbols):\n", table->symbol_count);
    
    for (int i = 0; i < table->symbol_count; i++) {
        Symbol *symbol = table->symbols[i];
        for (int j = 0; j < indent + 1; j++) {
            printf("  ");
        }
        
        // Get type name
        const char *type_name = "unknown";
        switch (symbol->type->base_type) {
            case TYPE_VOID: type_name = "void"; break;
            case TYPE_INT: type_name = "int"; break;
            case TYPE_CHAR: type_name = "char"; break;
            case TYPE_FLOAT: type_name = "float"; break;
            case TYPE_DOUBLE: type_name = "double"; break;
            case TYPE_ARRAY: type_name = "array"; break;
            case TYPE_POINTER: type_name = "pointer"; break;
            case TYPE_UNION: type_name = "union"; break;
            case TYPE_STRUCT: type_name = "struct"; break;
        }
        
        printf("%s: type=%s, offset=%d\n", symbol->name, type_name, symbol->offset);
    }
    
    // Recursively dump parent scopes
    if (table->parent != NULL) {
        for (int i = 0; i < indent; i++) {
            printf("  ");
        }
        printf("Parent scope:\n");
        symbol_table_dump(table->parent, indent + 1);
    }
}

/**
 * Free the symbol table and all symbols it contains
 * @param table The symbol table to free
 */
void symbol_table_free(SymbolTable *table) {
    if (table == NULL) {
        return;
    }
    
    for (int i = 0; i < table->symbol_count; i++) {
        free(table->symbols[i]->name);
        // Note: We don't free the type here because it might be shared
        free(table->symbols[i]);
    }
    
    free(table->symbols);
    free(table);
}