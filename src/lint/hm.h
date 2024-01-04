#ifndef __HM_H__
#define __HM_H__

#include <stdint.h>
#include <lhash_kv.h>

// Forward declaration of circular dependency
struct ast_node;
typedef lhash_kv_t symbol_table_t;

typedef enum {
    HM_TYPE_TAG_BASIC_TYPE,
    HM_TYPE_TAG_VARIABLE
} hm_type_tag_t;

typedef enum {
    HM_TYPE_INTEGRAL,
    HM_TYPE_BOOL
} hm_basic_type_t;

typedef uint32_t hm_type_variable_t;

typedef struct hm_type {
    hm_type_tag_t tag;
    union {
	hm_basic_type_t basic_type;
	hm_type_variable_t variable;
    };
} hm_type_t;

typedef struct {
    hm_type_t* argument_type;
    hm_type_t* return_type;
} constraint_t;

void hm_add_type_variables(struct ast_node* node, symbol_table_t* table);

#endif
