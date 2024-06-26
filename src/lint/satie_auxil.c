#define MUTE_LOG_DEBUG 1
#define MUTE_LOG_WARNING 1

#include <assert.h>
#include <log.h>

#include "satie_auxil.h"

// Forward declarations of local functions (alphabetical order)
static ast_node_t* create_node(satie_auxil_t* auxil, node_name_t name,
			       uint16_t n, va_list args);
static ast_node_t* new_node(satie_auxil_t* auxil, node_name_t name);

satie_auxil_t* satie_auxil_new() {
    LOG_DEBUG("satie_auxil_new");
    satie_auxil_t* auxil = malloc(sizeof(satie_auxil_t));
    auxil->row = 1;
    return auxil;
}

ast_node_t* satie_auxil_rename_node(ast_node_t* node, node_name_t name) {
    LOG_DEBUG("rename_node");
    if (node == NULL) {
        LOG_WARNING("An undefined node cannot be renamed to %s",
		    ast_node_name_to_string(name));
    } else {
        node->name = name;
    }
    return node;
}

ast_node_t* satie_auxil_create_terminal(satie_auxil_t* auxil, node_name_t name,
					const char* value) {
    LOG_DEBUG("create_terminal: %s (%s)", value, ast_node_name_to_string(name));
    ast_node_t* node = new_node(auxil, name);
    if (value != NULL) {
        node->value = strdup(value);
    } else {
        node->value = NULL;
    }
    return node;
}

ast_node_t* satie_auxil_create_node(satie_auxil_t* auxil, node_name_t name,
				    uint16_t n, ...) {
    LOG_DEBUG("create_node: %s", ast_node_name_to_string(name));
    va_list args;
    va_start(args, n);
    ast_node_t* node = create_node(auxil, name, n, args);
    va_end(args);
    return node;
}

ast_node_t* satie_auxil_conditional_create_node(satie_auxil_t* auxil,
						node_name_t name,
						uint16_t n, ...) {
    LOG_DEBUG("conditional_create_node: %s", ast_node_name_to_string(name));
    va_list args;
    va_start(args, n);
    bool any_children = false;
    for (uint16_t i = 0; i < n; i++) {
	ast_node_t* child_node = va_arg(args, ast_node_t*);
	if (child_node != NULL) {
	    any_children = true;
	    break;
	}
    }
    va_end(args);
    if (any_children) {
	va_list args;
	va_start(args, n);
	ast_node_t* node = create_node(auxil, name, n, args);
	va_end(args);
	return node;
    } else {
	return NULL;
    }
}

void satie_auxil_add_child(ast_node_t* parent_node, ast_node_t* node) {
    LOG_DEBUG("add_child");
    if (node == NULL) {
        LOG_WARNING("An undefined node cannot be appended to %s",
		    ast_node_name_to_string(parent_node->name));
    } else {
        if (node->name == POSTFIX &&
            dynarray_size(node->children) == 1) {
            ast_node_t* child_node = dynarray_element(node->children, 0);
            free(node);
            node = child_node;
        }
        if (parent_node->children == NULL) {
	    parent_node->children = malloc(sizeof(node_array_t));
	    dynarray_init(parent_node->children, NULL, 0, sizeof(ast_node_t));
	}
        dynarray_append(parent_node->children, node);
    }
}

//
// Local functions (alphabetical order)
//

static ast_node_t* create_node(satie_auxil_t* auxil, node_name_t name,
			       uint16_t n, va_list args) {
    LOG_DEBUG("create_node: %s", ast_node_name_to_string(name));
    ast_node_t* node = new_node(auxil, name);
    node->children = malloc(sizeof(node_array_t));
    dynarray_init(node->children, NULL, 0, sizeof(ast_node_t));
    for (uint16_t i = 0; i < n; i++) {
	ast_node_t* child_node = va_arg(args, ast_node_t*);
	if (child_node != NULL) {
	    if (child_node->name == POSTFIX &&
		dynarray_size(child_node->children) == 1) {
		ast_node_t* grand_child_node =
		    dynarray_element(child_node->children, 0);
		free(child_node);
		child_node = grand_child_node;
	    }
	    dynarray_append(node->children, child_node);
	} else {
	    LOG_WARNING("An undefined child node %d is ignored by %s\n",
			i, ast_node_name_to_string(name));
	}
    }
    return node;
}

static ast_node_t* new_node(satie_auxil_t* auxil, node_name_t name) {
    ast_node_t* node = malloc(sizeof(ast_node_t));
    node->name = name;
    node->type = NULL;
    node->value = NULL;
    node->row = auxil->row;
    node->column = 1;
    node->children = NULL;
    return node;
}
