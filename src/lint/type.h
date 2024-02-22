#ifndef __TYPE_H__
#define __TYPE_H__

#include "dynarr.h"

typedef dynarray_t types_t;

typedef enum {
    TYPE_TAG_BASIC_TYPE,
    TYPE_TAG_CONSTRUCTOR_TYPE,
    TYPE_TAG_FUNCTION_TYPE,
    TYPE_TAG_LIST_TYPE,
    TYPE_TAG_MAP_TYPE,
    TYPE_TAG_TUPLE_TYPE,
    TYPE_TAG_TYPE_VARIABLE
} type_tag_t;

typedef enum {
    TYPE_BASIC_TYPE_BOOL,
    TYPE_BASIC_TYPE_INT,
    TYPE_BASIC_TYPE_FLOAT,
    TYPE_BASIC_TYPE_STRING,
    TYPE_BASIC_TYPE_TASK
} type_basic_type_t;

typedef uint32_t type_variable_t;

typedef struct type {
    type_tag_t tag;
    union {
	type_basic_type_t basic_type;
	struct type* list_type;
	struct {
	    types_t* arg_types;
	    struct type* return_type;
	} function_type;
	types_t* tuple_types;
	struct {
	    struct type* key_type;
	    struct type* value_type;
	} map_type;
	struct {
	    char* name;
	    types_t* types;
	} constructor_type;
	type_variable_t type_variable;
    };
} type_t;

type_t* type_new_basic_type(type_basic_type_t basic_type);
type_t* type_new_list_type(type_t* list_type);
type_t* type_new_function_type(types_t* arg_types, type_t* return_type);
type_t* type_new_tuple_type(types_t* tuple_types);
type_t* type_new_map_type(type_t* key_type, type_t* value_type);
type_t* type_new_constructor_type(char* name, types_t* types);
type_t* type_new_type_variable(void);
char* type_basic_type_to_string(type_basic_type_t basic_type);
type_basic_type_t type_string_to_basic_type(const char* string);
void type_print_type(type_t* type);

#endif
