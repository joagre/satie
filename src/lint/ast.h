#ifndef __AST_H__
#define __AST_H__

#include <stdio.h>
#include <dynarr.h>
#include <assert.h>
#include <stdbool.h>
#include "type.h"

typedef dynarray_t node_array_t;

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,
#define FOREACH_NODE_NAME(NODE_NAME) \
        NODE_NAME(ALIAS) \
        NODE_NAME(AND) \
        NODE_NAME(ARGS) \
        NODE_NAME(BIND) \
        NODE_NAME(BITWISE_AND) \
        NODE_NAME(BITWISE_COMPLEMENT) \
        NODE_NAME(BITWISE_OR) \
        NODE_NAME(BITWISE_XOR) \
        NODE_NAME(BLOCK_EXPR) \
        NODE_NAME(BLOCK_LEVEL_EXPR) \
        NODE_NAME(BOUND_NAME) \
        NODE_NAME(BSL) \
        NODE_NAME(BSR) \
        NODE_NAME(CASE) \
        NODE_NAME(CAST) \
        NODE_NAME(CHANNEL) \
        NODE_NAME(CHANNEL_NAME) \
        NODE_NAME(CHANNELS) \
        NODE_NAME(CHARACTER_LITERAL) \
        NODE_NAME(CLASS_DEF) \
        NODE_NAME(CLASS_MEMBERS) \
        NODE_NAME(CLASS_NAME) \
        NODE_NAME(CONCAT) \
        NODE_NAME(CONST) \
        NODE_NAME(CONSTRUCTOR) \
        NODE_NAME(DEFAULT) \
        NODE_NAME(DEFAULT_PARAM) \
        NODE_NAME(DEFAULT_PARAM_NAME) \
        NODE_NAME(DEFAULT_PARAMS) \
        NODE_NAME(DESTRUCTOR) \
        NODE_NAME(DIVIDE) \
        NODE_NAME(DOT_NAME) \
        NODE_NAME(DOTTED_NAME) \
        NODE_NAME(ELIF) \
        NODE_NAME(ELSE) \
        NODE_NAME(ENUM) \
        NODE_NAME(ENUM_DEF) \
        NODE_NAME(ENUM_DEF_NAME) \
        NODE_NAME(ENUM_LITERAL) \
        NODE_NAME(ENUM_NAME) \
        NODE_NAME(ENUMS) \
        NODE_NAME(ENUM_VALUE) \
        NODE_NAME(EQ) \
        NODE_NAME(ESCAPE_CHARACTER) \
        NODE_NAME(EXPONENTIATE) \
        NODE_NAME(EXPORT) \
        NODE_NAME(EXPR) \
        NODE_NAME(EXPRS) \
        NODE_NAME(FALSE) \
        NODE_NAME(FLOATING_POINT) \
        NODE_NAME(FUNCTION_CALL) \
        NODE_NAME(FUNCTION_DEF) \
        NODE_NAME(FUNCTION_LITERAL) \
        NODE_NAME(FUNCTION_NAME) \
        NODE_NAME(GT) \
        NODE_NAME(GTE) \
        NODE_NAME(IDENTIFIER) \
        NODE_NAME(IF) \
        NODE_NAME(IF_EXPR) \
        NODE_NAME(IMPORT) \
        NODE_NAME(IMPORTED_NAME) \
        NODE_NAME(IMPORTED_NAMES) \
        NODE_NAME(IMPORTS) \
        NODE_NAME(IN) \
        NODE_NAME(INDEX_VALUE) \
        NODE_NAME(INDEX_VALUES)        \
        NODE_NAME(INTEGRAL) \
        NODE_NAME(INTERFACE) \
        NODE_NAME(INTERFACE_DEF) \
        NODE_NAME(INTERFACE_MEMBER_METHOD) \
        NODE_NAME(INTERFACE_MEMBER_PROPERTY) \
        NODE_NAME(INTERFACE_MEMBERS) \
        NODE_NAME(INTERFACE_METHOD) \
        NODE_NAME(INTERFACE_NAME) \
        NODE_NAME(INTERFACES) \
        NODE_NAME(LIST_LITERAL) \
        NODE_NAME(LIST_LOOKUP) \
        NODE_NAME(LIST_SLICE)   \
        NODE_NAME(LIST_UPDATE) \
        NODE_NAME(LT) \
        NODE_NAME(LTE) \
        NODE_NAME(MAP_KEY_VALUE) \
        NODE_NAME(MAP_KEY_VALUES) \
        NODE_NAME(MAP_LITERAL) \
        NODE_NAME(MAP_UPDATE) \
        NODE_NAME(MATCH_EXPR) \
        NODE_NAME(MATCH_EXPRS) \
        NODE_NAME(MATCH_IS) \
        NODE_NAME(MEMBER_METHOD) \
        NODE_NAME(MEMBER_PROPERTY) \
        NODE_NAME(MINUS) \
        NODE_NAME(MODULE) \
        NODE_NAME(MODULE_COMPONENT) \
        NODE_NAME(MODULUS) \
        NODE_NAME(MULTIPLY) \
        NODE_NAME(NAME) \
        NODE_NAME(NAMED_ARG) \
        NODE_NAME(NAMED_ARGS) \
        NODE_NAME(NE) \
        NODE_NAME(NEW_EXPR) \
        NODE_NAME(NIL) \
        NODE_NAME(NON_DEFAULT_PARAM) \
        NODE_NAME(NON_DEFAULT_PARAMS) \
        NODE_NAME(NON_QUOTE_CHARACTER) \
        NODE_NAME(NOT) \
        NODE_NAME(NOT_SET) \
        NODE_NAME(OR) \
        NODE_NAME(PARAMS) \
        NODE_NAME(PLUS) \
        NODE_NAME(POSITIONAL_ARGS) \
        NODE_NAME(POSTFIX_EXPR) \
        NODE_NAME(PRIVATE) \
        NODE_NAME(PROGRAM) \
        NODE_NAME(PUBLIC) \
        NODE_NAME(RAW_STRING) \
        NODE_NAME(READONLY) \
        NODE_NAME(RECEIVE) \
        NODE_NAME(RECEIVE_EXPR) \
        NODE_NAME(REGULAR_STRING) \
        NODE_NAME(SELF) \
        NODE_NAME(SLICE_LENGTH) \
        NODE_NAME(SWITCH) \
        NODE_NAME(SWITCH_EXPR) \
        NODE_NAME(THIS) \
        NODE_NAME(TIMEOUT) \
        NODE_NAME(TOP_LEVEL_DEFS) \
        NODE_NAME(TRUE) \
        NODE_NAME(TUPLE_LITERAL) \
        NODE_NAME(UNARY_MINUS) \
        NODE_NAME(UNARY_PLUS) \
        NODE_NAME(UNBOUND_NAME) \
        NODE_NAME(WHEN)

typedef enum {
    FOREACH_NODE_NAME(GENERATE_ENUM)
} node_name_t;

typedef uint32_t type_variable_t;

typedef struct ast_node {
    node_name_t name;
    type_t* type;
    char *value;
    uint32_t row;
    uint32_t column;
    node_array_t* children;
} ast_node_t;

const char* ast_node_name_to_string(node_name_t name);
ast_node_t* ast_get_child(ast_node_t* node, uint32_t i);
size_t ast_number_of_children(ast_node_t* node);
ast_node_t* ast_last_child(ast_node_t* node);
void ast_print(ast_node_t* node, uint16_t level);

#endif
