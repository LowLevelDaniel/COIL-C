/**
 * @file type.h
 * @brief Type system definitions for the COIL C Compiler
 *
 * Maps C types to COIL types following the COIL specification.
 */

#ifndef TYPE_H
#define TYPE_H

#include <stdbool.h>

/**
 * Data type enumerations, following COIL specification in type.md
 */
typedef enum {
    TYPE_VOID,     /**< Void type (no value) */
    TYPE_INT,      /**< Integer type */
    TYPE_CHAR,     /**< Character type */
    TYPE_FLOAT,    /**< Single-precision floating-point type */
    TYPE_DOUBLE,   /**< Double-precision floating-point type */
    TYPE_ARRAY,    /**< Array type */
    TYPE_POINTER,  /**< Pointer type */
    TYPE_STRUCT,   /**< Structure type (planned for future) */
    TYPE_UNION     /**< Union type (planned for future) */
} DataType;

/**
 * Type structure that represents both C types and their COIL equivalents
 */
typedef struct Type {
    DataType base_type;      /**< The base data type */
    struct Type *pointer_to; /**< For pointer and array types */
    int array_size;          /**< For array types */
    int size;                /**< Size in bytes */
    bool is_signed;          /**< For integer types */
    bool is_const;           /**< Whether the type is const-qualified */
    bool is_volatile;        /**< Whether the type is volatile-qualified */
} Type;

/**
 * Create a type with the specified base type
 * @param base_type The base data type
 * @return A new type structure
 */
Type *create_type(DataType base_type);

/**
 * Create a pointer type to the specified type
 * @param base_type The type to create a pointer to
 * @return A new pointer type
 */
Type *create_pointer_type(Type *base_type);

/**
 * Create an array type with the given element type and size
 * @param element_type The element type
 * @param size The number of elements
 * @return A new array type
 */
Type *create_array_type(Type *element_type, int size);

/**
 * Check if two types are equal
 * @param a First type
 * @param b Second type
 * @return true if the types are equal, false otherwise
 */
bool types_equal(Type *a, Type *b);

/**
 * Create a copy of a type
 * @param type The type to copy
 * @return A new copy of the type
 */
Type *type_copy(Type *type);

/**
 * Get a string representation of a type
 * @param type The type
 * @return A string representation (must be freed by caller)
 */
char *type_to_string(Type *type);

/**
 * Get the COIL type encoding for a type
 * @param type The type to encode
 * @return The COIL type encoding
 */
unsigned char get_coil_type(Type *type);

/**
 * Get the COIL size encoding for a type
 * @param type The type to encode
 * @return The COIL size encoding
 */
unsigned char get_coil_size(Type *type);

/**
 * Check if a type is numeric (can be used in arithmetic operations)
 * @param type The type to check
 * @return true if the type is numeric, false otherwise
 */
bool is_numeric_type(Type *type);

/**
 * Check if a type is integral (not floating-point)
 * @param type The type to check
 * @return true if the type is integral, false otherwise
 */
bool is_integral_type(Type *type);

/**
 * Check if a type is a floating-point type
 * @param type The type to check
 * @return true if the type is floating-point, false otherwise
 */
bool is_floating_type(Type *type);

/**
 * Get the common type for binary operations
 * @param a First operand type
 * @param b Second operand type
 * @return The common type (must be freed by caller)
 */
Type *get_common_type(Type *a, Type *b);

/**
 * Free a type structure and all contained types
 * @param type The type to free
 */
void free_type(Type *type);

#endif /* TYPE_H */