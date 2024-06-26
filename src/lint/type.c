#include <log.h>
#include <stdbool.h>
#include <stdio.h>

#include "ast.h"
#include "named_args.h"
#include "types.h"

// Forward declarations of local functions
static type_variable_t next_type_variable_id(void);

type_t* type_new_basic_type(type_basic_type_t basic_type) {
    type_t* type = malloc(sizeof(type_t));
    type->tag = TYPE_TAG_BASIC_TYPE;
    type->basic_type = basic_type;
    return type;
}

type_t* type_new_constructor_type(char* name, types_t* types) {
    type_t* type = malloc(sizeof(type_t));
    type->tag = TYPE_TAG_CONSTRUCTOR_TYPE;
    type->constructor_type.name = name;
    type->constructor_type.types = types;
    return type;
}

type_t* type_new_function_type(type_function_kind_t kind,
			       types_t* generic_types,
			       types_t* arg_types, type_t* return_type) {
    type_t* type = malloc(sizeof(type_t));
    type->tag = TYPE_TAG_FUNCTION_TYPE;
    type->function_type.kind = kind;
    type->function_type.generic_types = generic_types;
    type->function_type.arg_types = arg_types;
    type->function_type.return_type = return_type;
    return type;
}

type_t* type_new_list_type(type_t* list_type) {
    type_t* type = malloc(sizeof(type_t));
    type->tag = TYPE_TAG_LIST_TYPE;
    type->list_type = list_type;
    return type;
}

type_t* type_new_empty_list_type(void) {
    type_t* type = malloc(sizeof(type_t));
    type->tag = TYPE_TAG_EMPTY_LIST_TYPE;
    return type;
}

type_t* type_new_map_type(type_t* key_type, type_t* value_type) {
    type_t* type = malloc(sizeof(type_t));
    type->tag = TYPE_TAG_MAP_TYPE;
    type->map_type.key_type = key_type;
    type->map_type.value_type = value_type;
    return type;
}

type_t* type_new_empty_map_type(void) {
    type_t* type = malloc(sizeof(type_t));
    type->tag = TYPE_TAG_EMPTY_MAP_TYPE;
    return type;
}

type_t* type_new_member_method_type(char* name,
				    member_method_modifier_t modifier,
				    types_t* generic_types, types_t* arg_types,
				    type_t* return_type) {
    type_t* type = malloc(sizeof(type_t));
    type->tag = TYPE_TAG_MEMBER_METHOD_TYPE;
    type->member_method_type.name = name;
    type->member_method_type.modifier = modifier;
    type->member_method_type.generic_types = generic_types;
    type->member_method_type.arg_types = arg_types;
    type->member_method_type.return_type = return_type;
    return type;
}

type_t* type_new_member_property_type(char* name,
				      member_property_modifier_t modifier,
				      type_t* type) {
    type_t* member_property_type = malloc(sizeof(type_t));
    member_property_type->tag = TYPE_TAG_MEMBER_PROPERTY_TYPE;
    member_property_type->member_property_type.name = name;
    member_property_type->member_property_type.modifier = modifier;
    member_property_type->member_property_type.type = type;
    return member_property_type;
}

type_t* type_new_record_def_type(char*name, types_t* types_variables,
				 types_t* member_types) {
    type_t* type = malloc(sizeof(type_t));
    type->tag = TYPE_TAG_RECORD_DEF_TYPE;
    type->record_def_type.name = name;
    type->record_def_type.type_variables = types_variables;
    type->record_def_type.member_types = member_types;
    return type;
}

type_t* type_new_record_instance_type(char*name, type_t* record_def_type,
				      types_t* generic_types,
				      named_args_t* named_args) {
    type_t* type = malloc(sizeof(type_t));
    type->tag = TYPE_TAG_RECORD_INSTANCE_TYPE;
    type->record_instance_type.name = name;
    type->record_instance_type.record_def_type = record_def_type;
    type->record_instance_type.generic_types = generic_types;
    type->record_instance_type.named_args = named_args;
    return type;
}

type_t* type_new_record_dot_type(type_t* postfix_expr_type, char* member_name) {
    type_t* type = malloc(sizeof(type_t));
    type->tag = TYPE_TAG_RECORD_DOT_TYPE;
    type->record_dot_type.postfix_expr_type = postfix_expr_type;
    type->record_dot_type.member_name = member_name;
    return type;
}

type_t* type_new_tuple_type(types_t* tuple_types) {
    type_t* type = malloc(sizeof(type_t));
    type->tag = TYPE_TAG_TUPLE_TYPE;
    type->tuple_types = tuple_types;
    return type;
}

type_t* type_new_empty_tuple_type(void) {
    type_t* type = malloc(sizeof(type_t));
    type->tag = TYPE_TAG_EMPTY_TUPLE_TYPE;
    return type;
}

type_t* type_new_type_variable(char* name) {
    type_t* type = malloc(sizeof(type_t));
    type->tag = TYPE_TAG_TYPE_VARIABLE;
    type->type_variable.name = name;
    type->type_variable.id = next_type_variable_id();
    return type;
}

void type_free(type_t* type) {
    free(type);
}

char* type_basic_type_to_string(type_basic_type_t basic_type) {
    switch (basic_type) {
	case TYPE_BASIC_TYPE_BOOL:
	    return "Bool";
	case TYPE_BASIC_TYPE_INT:
	    return "Int";
	case TYPE_BASIC_TYPE_FLOAT:
	    return "Float";
	case TYPE_BASIC_TYPE_CHAR:
	    return "Char";
	case TYPE_BASIC_TYPE_STRING:
	    return "String";
	case TYPE_BASIC_TYPE_TASK:
	    return "Task";
	default:
	    LOG_ABORT("Unknown basic type: %d\n", basic_type);
	    return NULL;
    }
}

type_basic_type_t type_string_to_basic_type(const char* string) {
    if (strcmp(string, "Bool") == 0) {
	return TYPE_BASIC_TYPE_BOOL;
    } else if (strcmp(string, "Int") == 0) {
	return TYPE_BASIC_TYPE_INT;
    } else if (strcmp(string, "Float") == 0) {
	return TYPE_BASIC_TYPE_FLOAT;
    } else if (strcmp(string, "Char") == 0) {
	return TYPE_BASIC_TYPE_CHAR;
    } else if (strcmp(string, "String") == 0) {
	return TYPE_BASIC_TYPE_STRING;
    } else if (strcmp(string, "Task") == 0) {
	return TYPE_BASIC_TYPE_TASK;
    } else {
	LOG_ABORT("Unknown basic type: %s\n", string);
	return 0;
    }
}

void type_print_type(type_t* type) {
    switch (type->tag) {
	case TYPE_TAG_BASIC_TYPE:
	    switch (type->basic_type) {
		case TYPE_BASIC_TYPE_BOOL:
		    printf("bool");
		    break;
		case TYPE_BASIC_TYPE_INT:
		    printf("int");
		    break;
		case TYPE_BASIC_TYPE_FLOAT:
		    printf("float");
		    break;
		case TYPE_BASIC_TYPE_CHAR:
		    printf("char");
		    break;
		case TYPE_BASIC_TYPE_STRING:
		    printf("string");
		    break;
		case TYPE_BASIC_TYPE_TASK:
		    printf("task");
		    break;
		default:
		    LOG_ABORT("Unknown basic type: %d\n", type->basic_type);
	    }
	    break;
	case TYPE_TAG_CONSTRUCTOR_TYPE:
	    printf("{constructor, %s, [", type->constructor_type.name);
	    uint16_t n = types_size(type->constructor_type.types);
	    for (uint16_t i = 0; i < n; i++) {
		type_t* constructor_type =
		    types_get(type->constructor_type.types, i);
		type_print_type(constructor_type);
		if (i < n - 1) {
		    printf(", ");
		}
	    }
	    printf("]}");
	    break;
	case TYPE_TAG_FUNCTION_TYPE: {
	    printf("{function, ");
	    switch (type->function_type.kind) {
		case TYPE_FUNCTION_KIND_INSTANCE:
		    printf("instance, ");
		    break;
		case TYPE_FUNCTION_KIND_DEFINITION:
		    printf("definition, ");
		    break;
	    }
	    // Generic types
	    printf("[");
	    uint16_t n = types_size(type->function_type.generic_types);
	    for (uint16_t i = 0; i < n; i++) {
		type_t* generic_type =
		    types_get(type->function_type.generic_types, i);
		type_print_type(generic_type);
		if (i < n - 1) {
		    printf(", ");
		}
	    }
	    printf("], ");
	    // Argument types
	    printf("[");
	    n = types_size(type->function_type.arg_types);
	    for (uint16_t i = 0; i < n; i++) {
		type_t* arg_type =
		    types_get(type->function_type.arg_types, i);
		type_print_type(arg_type);
		if (i < n - 1) {
		    printf(", ");
		}
	    }
	    printf("], ");
	    type_print_type(type->function_type.return_type);
	    printf("}");
	    break;
	}
	case TYPE_TAG_LIST_TYPE:
	    printf("{list, ");
	    type_print_type(type->list_type);
	    printf("}");
	    break;
	case TYPE_TAG_EMPTY_LIST_TYPE:
	    printf("empty_list");
	    break;
	case TYPE_TAG_MAP_TYPE:
	    printf("{map, ");
	    type_print_type(type->map_type.key_type);
	    printf(", ");
	    type_print_type(type->map_type.value_type);
	    printf("}");
	    break;
	case TYPE_TAG_EMPTY_MAP_TYPE:
	    printf("empty_map");
	    break;
	case TYPE_TAG_MEMBER_METHOD_TYPE: {
	    printf("{method, \"%s\", ", type->member_method_type.name);
	    switch (type->member_method_type.modifier) {
		case MEMBER_METHOD_MODIFIER_PRIVATE:
		    printf("private, ");
		    break;
		case MEMBER_METHOD_MODIFIER_PUBLIC:
		    printf("public, ");
		    break;
		default:
		    LOG_ABORT("Unknown member method modifier: %d\n",
			      type->member_method_type.modifier);
	    }
	    // Generic types
	    printf("[");
	    uint16_t n = types_size(type->member_method_type.generic_types);
	    for (uint16_t i = 0; i < n; i++) {
		type_t* generic_type =
		    types_get(type->member_method_type.generic_types, i);
		type_print_type(generic_type);
		if (i < n - 1) {
		    printf(", ");
		}
	    }
	    printf("], ");
	    // Argument types
	    printf("[");
	    n = types_size(type->member_method_type.arg_types);
	    for (uint16_t i = 0; i < n; i++) {
		type_t* arg_type =
		    types_get(type->member_method_type.arg_types, i);
		type_print_type(arg_type);
		if (i < n - 1) {
		    printf(", ");
		}
	    }
	    printf("], ");
	    type_print_type(type->member_method_type.return_type);
	    printf("}");
	    break;
	}
	case TYPE_TAG_MEMBER_PROPERTY_TYPE:
	    printf("{property, \"%s\", ", type->member_property_type.name);
	    switch (type->member_property_type.modifier) {
		case MEMBER_PROPERTY_MODIFIER_PRIVATE:
		    printf("private, ");
		    break;
		case MEMBER_PROPERTY_MODIFIER_PRIVATE_CONST:
		    printf("private_const, ");
		    break;
		case MEMBER_PROPERTY_MODIFIER_PUBLIC:
		    printf("public, ");
		    break;
		case MEMBER_PROPERTY_MODIFIER_PUBLIC_CONST:
		    printf("public_const, ");
		    break;
		case MEMBER_PROPERTY_MODIFIER_READONLY:
		    printf("readonly, ");
		    break;
		default:
		    LOG_ABORT("Unknown member property modifier: %d\n",
			      type->member_property_type.modifier);
	    }
	    type_print_type(type->member_property_type.type);
	    printf("}");
	    break;
	case TYPE_TAG_RECORD_INSTANCE_TYPE: {
	    printf("{record_instance, \"%s\", ",
		   type->record_instance_type.name);
	    type_print_type(type->record_instance_type.record_def_type);
	    printf(", [");
	    uint16_t n = types_size(type->record_instance_type.generic_types);
	    for (uint16_t i = 0; i < n; i++) {
		type_t* generic_type =
		    types_get(type->record_instance_type.generic_types, i);
		type_print_type(generic_type);
		if (i < n - 1) {
		    printf(", ");
		}
	    }
	    printf("], [");
	    n = named_args_size(type->record_instance_type.named_args);
	    for (uint16_t i = 0; i < n; i++) {
		named_arg_t* named_arg =
		    named_args_get(type->record_instance_type.named_args, i);
		printf("{named_arg, \"%s\", ", named_arg->name);
		type_print_type(named_arg->type);
		printf("}");
		if (i < n - 1) {
		    printf(", ");
		}
	    }
	    printf("]}");
	    break;
	}
	case TYPE_TAG_RECORD_DEF_TYPE: {
	    printf("{record_def, \"%s\", [", type->record_def_type.name);
	    uint16_t n = types_size(type->record_def_type.type_variables);
	    for (uint16_t i = 0; i < n; i++) {
		type_t* type_variable =
		    types_get(type->record_def_type.type_variables, i);
		type_print_type(type_variable);
		if (i < n - 1) {
		    printf(", ");
		}
	    }
	    printf("], [");
	    n = types_size(type->record_def_type.member_types);
	    for (uint16_t i = 0; i < n; i++) {
		type_t* member_type =
		    types_get(type->record_def_type.member_types, i);
		type_print_type(member_type);
		if (i < n - 1) {
		    printf(", ");
		}
	    }
	    printf("]}");
	    break;
	}
	case TYPE_TAG_RECORD_DOT_TYPE:
	    printf("{record_dot, ");
	    type_print_type(type->record_dot_type.postfix_expr_type);
	    printf(", \"%s\"}", type->record_dot_type.member_name);
	    break;
	case TYPE_TAG_TUPLE_TYPE: {
	    printf("{tuple, [");
	    uint16_t n = types_size(type->tuple_types);
	    for (uint16_t i = 0; i < n; i++) {
		type_t* tuple_type =
		    types_get(type->tuple_types, i);
		type_print_type(tuple_type);
		if (i < n - 1) {
		    printf(", ");
		}
	    }
	    printf("]}");
	    break;
	}
	case TYPE_TAG_EMPTY_TUPLE_TYPE:
	    printf("empty_tuple");
	    break;
	case TYPE_TAG_TYPE_VARIABLE:
	    printf("%d", type->type_variable.id);
	    break;
	default:
	    LOG_ABORT("Unknown type tag: %d\n", type->tag);
    }
}

//
// Local functions
//

static type_variable_t next_type_variable_id(void) {
    static uint32_t next_type_variable_id = 0;
    return next_type_variable_id++;
}
