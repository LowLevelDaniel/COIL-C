/**
 * Enhanced Type System Implementation for the COIL C Compiler
 * Maps C types to COIL types following the COIL specification
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "coil_constants.h"

/**
 * Create a type with the specified base type
 */
Type *create_type(DataType base_type) {
    Type *type = malloc(sizeof(Type));
    if (type == NULL) {
        fprintf(stderr, "Memory allocation failed for type\n");
        exit(1);
    }
    
    type->base_type = base_type;
    type->pointer_to = NULL;
    type->array_size = 0;
    
    // Set size based on data type
    switch (base_type) {
        case TYPE_VOID:
            type->size = 0;
            break;
        case TYPE_INT:
            type->size = 4; // 32-bit int
            break;
        case TYPE_CHAR:
            type->size = 1; // 8-bit char
            break;
        case TYPE_FLOAT:
            type->size = 4; // 32-bit float
            break;
        case TYPE_DOUBLE:
            type->size = 8; // 64-bit double
            break;
        case TYPE_POINTER:
            type->size = 8; // 64-bit pointer (for 64-bit architectures)
            break;
        case TYPE_ARRAY:
            type->size = 0; // Size will be set when array dimensions are known
            break;
        default:
            fprintf(stderr, "Unknown data type\n");
            free(type);
            exit(1);
    }
    
    return type;
}

/**
 * Create a pointer type to the specified type
 */
Type *create_pointer_type(Type *base_type) {
    Type *ptr_type = create_type(TYPE_POINTER);
    ptr_type->pointer_to = base_type;
    return ptr_type;
}

/**
 * Create an array type with the given element type and size
 */
Type *create_array_type(Type *element_type, int size) {
    Type *array_type = create_type(TYPE_ARRAY);
    array_type->pointer_to = element_type;
    array_type->array_size = size;
    array_type->size = size * element_type->size;
    return array_type;
}

/**
 * Check if two types are equal
 */
bool types_equal(Type *a, Type *b) {
    if (a == NULL || b == NULL) {
        return a == b;
    }
    
    if (a->base_type != b->base_type) {
        return false;
    }
    
    if (a->base_type == TYPE_POINTER || a->base_type == TYPE_ARRAY) {
        return types_equal(a->pointer_to, b->pointer_to);
    }
    
    if (a->base_type == TYPE_ARRAY && a->array_size != b->array_size) {
        return false;
    }
    
    return true;
}

/**
 * Create a copy of a type
 */
Type *type_copy(Type *type) {
    if (type == NULL) {
        return NULL;
    }
    
    Type *copy = create_type(type->base_type);
    copy->size = type->size;
    copy->array_size = type->array_size;
    
    if (type->pointer_to != NULL) {
        copy->pointer_to = type_copy(type->pointer_to);
    }
    
    return copy;
}

/**
 * Get a string representation of a type
 * The returned string must be freed by the caller
 */
char *type_to_string(Type *type) {
    if (type == NULL) {
        return strdup("NULL");
    }
    
    char buffer[256]; // Should be enough for most types
    
    switch (type->base_type) {
        case TYPE_VOID:
            strcpy(buffer, "void");
            break;
        case TYPE_INT:
            strcpy(buffer, "int");
            break;
        case TYPE_CHAR:
            strcpy(buffer, "char");
            break;
        case TYPE_FLOAT:
            strcpy(buffer, "float");
            break;
        case TYPE_DOUBLE:
            strcpy(buffer, "double");
            break;
        case TYPE_POINTER: {
            char *base_type_str = type_to_string(type->pointer_to);
            snprintf(buffer, sizeof(buffer), "%s*", base_type_str);
            free(base_type_str);
            break;
        }
        case TYPE_ARRAY: {
            char *element_type_str = type_to_string(type->pointer_to);
            if (type->array_size > 0) {
                snprintf(buffer, sizeof(buffer), "%s[%d]", element_type_str, type->array_size);
            } else {
                snprintf(buffer, sizeof(buffer), "%s[]", element_type_str);
            }
            free(element_type_str);
            break;
        }
        default:
            strcpy(buffer, "unknown");
            break;
    }
    
    return strdup(buffer);
}

/**
 * Get the COIL type encoding for a type
 */
unsigned char get_coil_type(Type *type) {
    if (type == NULL) {
        return COIL_TYPE_VOID;
    }
    
    switch (type->base_type) {
        case TYPE_VOID:
            return COIL_TYPE_VOID;
        case TYPE_INT:
            return COIL_TYPE_INT;
        case TYPE_CHAR:
            return COIL_TYPE_INT; // COIL uses INT for char type with 1-byte width
        case TYPE_FLOAT:
            return COIL_TYPE_FLOAT;
        case TYPE_DOUBLE:
            return COIL_TYPE_FLOAT; // COIL uses FLOAT for double type with 8-byte width
        case TYPE_POINTER:
            return COIL_TYPE_PTR;
        case TYPE_ARRAY:
            return COIL_TYPE_PTR; // Arrays are represented as pointers in COIL
        default:
            fprintf(stderr, "Unknown data type for COIL encoding\n");
            return COIL_TYPE_VOID; // Default fallback
    }
}

/**
 * Get the COIL size encoding for a type
 */
unsigned char get_coil_size(Type *type) {
    if (type == NULL) {
        return 0x00;
    }
    
    switch (type->base_type) {
        case TYPE_VOID:
            return 0x00;
        case TYPE_INT:
            return 0x04; // 4 bytes (32-bit)
        case TYPE_CHAR:
            return 0x01; // 1 byte (8-bit)
        case TYPE_FLOAT:
            return 0x04; // 4 bytes (32-bit)
        case TYPE_DOUBLE:
            return 0x08; // 8 bytes (64-bit)
        case TYPE_POINTER:
            return 0x08; // 8 bytes (64-bit pointer)
        case TYPE_ARRAY:
            return 0x08; // 8 bytes (64-bit pointer)
        default:
            fprintf(stderr, "Unknown data type for COIL size encoding\n");
            return 0x00;
    }
}

/**
 * Check if a type is numeric (can be used in arithmetic operations)
 */
bool is_numeric_type(Type *type) {
    if (type == NULL) {
        return false;
    }
    
    return type->base_type == TYPE_INT || 
           type->base_type == TYPE_CHAR || 
           type->base_type == TYPE_FLOAT || 
           type->base_type == TYPE_DOUBLE;
}

/**
 * Check if a type is integral (not floating-point)
 */
bool is_integral_type(Type *type) {
    if (type == NULL) {
        return false;
    }
    
    return type->base_type == TYPE_INT || type->base_type == TYPE_CHAR;
}

/**
 * Check if a type is a floating-point type
 */
bool is_floating_type(Type *type) {
    if (type == NULL) {
        return false;
    }
    
    return type->base_type == TYPE_FLOAT || type->base_type == TYPE_DOUBLE;
}

/**
 * Get the common type for binary operations
 * Implements C's type promotion rules
 */
Type *get_common_type(Type *a, Type *b) {
    if (a == NULL || b == NULL) {
        return NULL;
    }
    
    // If either operand is double, the result is double
    if (a->base_type == TYPE_DOUBLE || b->base_type == TYPE_DOUBLE) {
        return create_type(TYPE_DOUBLE);
    }
    
    // If either operand is float, the result is float
    if (a->base_type == TYPE_FLOAT || b->base_type == TYPE_FLOAT) {
        return create_type(TYPE_FLOAT);
    }
    
    // For integer types, promote to the larger type
    if (is_integral_type(a) && is_integral_type(b)) {
        if (a->size >= b->size) {
            return type_copy(a);
        } else {
            return type_copy(b);
        }
    }
    
    // If we get here, something is wrong
    fprintf(stderr, "Cannot determine common type for binary operation\n");
    return create_type(TYPE_INT); // Default fallback
}

/**
 * Free a type structure and all contained types
 */
void free_type(Type *type) {
    if (type == NULL) {
        return;
    }
    
    if (type->pointer_to != NULL) {
        free_type(type->pointer_to);
    }
    
    free(type);
}