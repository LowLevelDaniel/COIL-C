/**
* Type system implementation for the COIL C Compiler
*/

#include <stdlib.h>
#include "type.h"
#include "coil_constants.h"

Type *create_type(DataType base_type) {
  Type *type = malloc(sizeof(Type));
  type->base_type = base_type;
  type->pointer_to = NULL;
  type->array_size = 0;
  
  switch (base_type) {
      case TYPE_INT:
          type->size = 4;
          break;
      case TYPE_CHAR:
          type->size = 1;
          break;
      case TYPE_FLOAT:
          type->size = 4;
          break;
      case TYPE_DOUBLE:
          type->size = 8;
          break;
      case TYPE_VOID:
          type->size = 0;
          break;
      case TYPE_POINTER:
          type->size = 8; // Assuming 64-bit pointers
          break;
      case TYPE_ARRAY:
          // Size will be set when array dimensions are known
          type->size = 0;
          break;
  }
  
  return type;
}

Type *create_pointer_type(Type *base_type) {
  Type *ptr_type = create_type(TYPE_POINTER);
  ptr_type->pointer_to = base_type;
  return ptr_type;
}

Type *create_array_type(Type *element_type, int size) {
  Type *array_type = create_type(TYPE_ARRAY);
  array_type->pointer_to = element_type;
  array_type->array_size = size;
  array_type->size = size * element_type->size;
  return array_type;
}

unsigned char get_coil_type(Type *type) {
  switch (type->base_type) {
      case TYPE_INT:
          return COIL_TYPE_INT;
      case TYPE_CHAR:
          return COIL_TYPE_INT; // COIL uses INT for char type with 1-byte width
      case TYPE_FLOAT:
          return COIL_TYPE_FLOAT;
      case TYPE_DOUBLE:
          return COIL_TYPE_FLOAT; // COIL uses FLOAT for double type with 8-byte width
      case TYPE_VOID:
          return COIL_TYPE_VOID;
      case TYPE_POINTER:
          return COIL_TYPE_PTR;
      case TYPE_ARRAY:
          return COIL_TYPE_PTR; // Arrays are represented as pointers in COIL
      default:
          return COIL_TYPE_VOID; // Default fallback
  }
}

unsigned char get_coil_size(Type *type) {
  switch (type->base_type) {
      case TYPE_INT:
          return 0x04; // 4 bytes
      case TYPE_CHAR:
          return 0x01; // 1 byte
      case TYPE_FLOAT:
          return 0x04; // 4 bytes
      case TYPE_DOUBLE:
          return 0x08; // 8 bytes
      case TYPE_POINTER:
      case TYPE_ARRAY:
          return 0x08; // 8 bytes (64-bit pointers)
      case TYPE_VOID:
      default:
          return 0x00; // No size
  }
}

void free_type(Type *type) {
  if (type->pointer_to) {
      free_type(type->pointer_to);
  }
  free(type);
}