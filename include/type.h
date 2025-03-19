/**
* Type system definitions for the COIL C Compiler
* Maps C types to COIL types following the COIL specification
*/

#ifndef TYPE_H
#define TYPE_H

/**
* Data type enumerations, following COIL specification in type.md
*/
typedef enum {
  TYPE_VOID,
  TYPE_INT,
  TYPE_CHAR,
  TYPE_FLOAT,
  TYPE_DOUBLE,
  TYPE_ARRAY,
  TYPE_POINTER
} DataType;

/**
* Type structure that represents both C types and their COIL equivalents
*/
typedef struct Type {
  DataType base_type;
  struct Type *pointer_to;  // For pointer types
  int array_size;          // For array types
  int size;                // Size in bytes
} Type;

/* COIL type encodings from type.md */
#define COIL_TYPE_INT     0x00  // Signed integer
#define COIL_TYPE_UINT    0x01  // Unsigned integer
#define COIL_TYPE_FLOAT   0x02  // Floating-point
#define COIL_TYPE_VOID    0xF0  // Void type
#define COIL_TYPE_BOOL    0xF1  // Boolean
#define COIL_TYPE_LINT    0xF2  // Largest native integer
#define COIL_TYPE_FINT    0xF3  // Fastest native integer
#define COIL_TYPE_PTR     0xF4  // Native pointer

/**
* Create a type with the specified base type
*/
Type *create_type(DataType base_type);

/**
* Create a pointer type to the specified type
*/
Type *create_pointer_type(Type *base_type);

/**
* Create an array type with the given element type and size
*/
Type *create_array_type(Type *element_type, int size);

/**
* Get the COIL type encoding for a type
*/
unsigned char get_coil_type(Type *type);

/**
* Get the COIL size encoding for a type
*/
unsigned char get_coil_size(Type *type);

/**
* Free a type structure
*/
void free_type(Type *type);

#endif /* TYPE_H */