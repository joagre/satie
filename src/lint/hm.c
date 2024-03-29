//#define MUTE_LOG_DEBUG 1

#include <assert.h>
#include <log.h>
#include <bits.h>
#include "hm.h"
#include "types.h"
#include "equation.h"
#include "equations.h"
#include "symbol_table.h"
#include "symbol_tables.h"
#include "unbound_names_list.h"

// Forward declarations of local functions
static uint32_t unique_id(void);

static bool is_valid(ast_node_t* parent_node, ast_node_t* node, uint16_t flags,
		     satie_error_t* error);
static bool is_valid_match_exprs(ast_node_t* exprs_node, satie_error_t* error);
static void collect_unbound_names(ast_node_t* node, unbound_names_t* names);

static void forward_declare_function_definitions(ast_node_t* node,
						 symbol_tables_t* tables);

static bool create_type_variables(ast_node_t* node, symbol_tables_t* tables,
				  uint32_t block_expr_id, satie_error_t* error);

static bool create_type_equations(ast_node_t* node, symbol_tables_t* tables,
				  equations_t* equations, satie_error_t* error);
static bool create_type_variable_equations(
    equations_t* equations, ast_node_t* type_variables_node,
    uint16_t number_of_type_variables, ast_node_t* type_node,
    satie_error_t* error);
static type_t* extract_type(ast_node_t* type_node);
static operator_types_t get_operator_types(node_name_t name);

bool hm_infer_types(ast_node_t* node, satie_error_t* error) {
    // Check if tree is semantically valid
    if (!is_valid(NULL, node, 0, error)) {
	return false;
    }
    // Create the top level symbol table
    symbol_tables_t tables;
    symbol_tables_init(&tables);
    symbol_table_t* table = symbol_table_new();
    symbol_tables_insert_table(&tables, table, unique_id());
    // Create type variables
    forward_declare_function_definitions(node, &tables);
    if (!create_type_variables(node, &tables, unique_id(), error)) {
	return false;
    }
    // Print parse tree
    ast_print(node, 0);
    printf("\n");
    // Create type equations
    equations_t equations;
    equations_init(&equations);
    if (!create_type_equations(node, &tables, &equations, error)) {
	return false;
    }
    // Print equations
    equations_print(&equations);
    return true;
}

//
// Local functions
//

static uint32_t unique_id(void) {
    static uint32_t id = 0;
    return id++;
}

static bool is_valid(ast_node_t* parent_node, ast_node_t* node, uint16_t flags,
		     satie_error_t* error) {
    if (node->name == BIND &&
	parent_node != NULL && parent_node->name != BLOCK) {
	// Make sure that bind expressions aren't used deep within expressions
	SET_ERROR_MESSAGE(
	    error, COMPONENT_COMPILER,
	    "%d: The := operator is not allowed deep within expressions",
	    node->row);
	return false;
    } else if (node->name == BIND) {
	// Make sure that unbound names only are used to the left in
	// bind expressions. The actual check is done in the
	// UNBOUND_NAME check below.
	ast_node_t *left_node = ast_get_child(node, 0);
	if (is_valid(node, left_node, SET_BIT(flags, CONTEXT_BIND),
		     error)) {
	    uint16_t n = ast_number_of_children(node);
	    for (uint16_t i = 1; i < n; i++) {
		ast_node_t* child_node = ast_get_child(node, i);
		if (!is_valid(node, child_node, flags, error)) {
		    return false;
		}
	    }
	    CLEAR_ERROR(error);
	    return true;
	} else {
	    return false;
	}
    } else if (node->name == CASE) {
	// Make sure that unbound names only are used in match expressions
	ast_node_t* match_exprs_node = ast_get_child(node, 0);
	if (is_valid(node, match_exprs_node, SET_BIT(flags, CONTEXT_CASE),
		     error)) {
	    // Make sure that unbound names occur in all match
	    // expressions (if more than one)
	    if (ast_number_of_children(match_exprs_node) > 1 &&
		!is_valid_match_exprs(match_exprs_node, error)) {
		return false;
	    }
	    // Validate all other children
	    uint16_t n = ast_number_of_children(node);
	    for (uint16_t i = 1; i < n; i++) {
		ast_node_t* child_node = ast_get_child(node, i);
		if (!is_valid(node, child_node, flags, error)) {
		    return false;
		}
	    }
	    CLEAR_ERROR(error);
	    return true;
	} else {
	    return false;
	}
    } else if (node->name == LIST_LOOKUP) {
	// Make sure that $ expressions only are allowed in list lookup/slices
	return is_valid(node, ast_get_child(node, 0),
			SET_BIT(flags, CONTEXT_LIST_LOOKUP), error);
    } else if (node->name == LIST_SLICE) {
	// Make sure that $ expressions only are allowed in list lookup/slices
	if (is_valid(node, ast_get_child(node, 0),
		     SET_BIT(flags, CONTEXT_LIST_SLICE), error)) {
	    return is_valid(node, ast_get_child(node, 1),
			    SET_BIT(flags, CONTEXT_LIST_SLICE), error);
	} else {
	    return false;
	}
    } else if (node->name == SLICE_LENGTH) {
	// Make sure that $ expressions only are allowed in certain situations
	if (IS_BIT_SET(flags, CONTEXT_LIST_SLICE) != 0 ||
	    IS_BIT_SET(flags, CONTEXT_LIST_LOOKUP) != 0) {
	    CLEAR_ERROR(error);
	    return true;
	} else {
	    SET_ERROR_MESSAGE(
		error, COMPONENT_COMPILER,
		"%d: The $ operator is only allowed in list lookups and list "
		"slices",
		node->row);
	    return false;
	}
    } else if (node->name == AS) {
	return is_valid(node, ast_get_child(node, 0),
			SET_BIT(flags, CONTEXT_AS), error);
    } else if (node->name == UNBOUND_NAME) {
	// Make sure that unbound names are allowed only in certain locations
	if (IS_BIT_SET(flags, CONTEXT_AS) != 0) {
	    // Unbound names are allowed in 'as' constructs
	    CLEAR_ERROR(error);
	    return true;
	} else if (IS_BIT_SET(flags, CONTEXT_BIND) != 0 ||
		   IS_BIT_SET(flags, CONTEXT_CASE) != 0) {
	    // Unbound names are only allowed their own or in
	    // composite literals
	    if(parent_node->name == BIND ||
	       parent_node->name == TUPLE_LITERAL) {
		CLEAR_ERROR(error);
		return true;
	    } else {
		SET_ERROR_MESSAGE(
		    error, COMPONENT_COMPILER,
		    "%d: The unbound name '%s' is only allowed on its own or "
		    "in composite literals", node->row, node->value);
		return false;
	    }
	} else {
	    SET_ERROR_MESSAGE(
		error, COMPONENT_COMPILER,
		"%d: The unbound name '%s' is not allowed here", node->row,
		node->value);
	    return false;
	}
    }

    // Traverse children (if any)
    uint16_t n = ast_number_of_children(node);
    if (n > 0) {
        for (uint16_t i = 0; i < n; i++) {
	    if (!is_valid(node, ast_get_child(node, i), flags, error)) {
		return false;
	    }
	}
    }

    CLEAR_ERROR(error);
    return true;
}

static bool is_valid_match_exprs(ast_node_t* exprs_node, satie_error_t* error) {
    unbound_names_list_t list;
    unbound_names_list_init(&list);
    // Collect all unbound names in all expressions
    uint16_t n = ast_number_of_children(exprs_node);
    for (uint16_t i = 0; i < n; i++) {
	ast_node_t* expr_node = ast_get_child(exprs_node, i);
	unbound_names_t* names = unbound_names_new();
	collect_unbound_names(expr_node, names);
	unbound_names_list_append(&list, names);
    }
    // Verify that any unbound name is represented in all expressions
    for (uint16_t i = 0; i < n; i++) {
	unbound_names_t* names = unbound_names_list_get(&list, i);
	uint16_t k = unbound_names_size(names);
	for (uint16_t j = 0; j < k; j++) {
	    for (uint16_t l = 0; l < n; l++) {
		if (l == i) {
		    continue;
		}
		char* name = unbound_names_get(names, j);
		unbound_names_t* names_to_be_checked =
		    unbound_names_list_get(&list, l);
		if (!unbound_names_member(names_to_be_checked, name)) {
		    SET_ERROR_MESSAGE(
			error, COMPONENT_COMPILER,
			"%d: The unbound name ?%s must occur in all case "
			"expressions", exprs_node->row, name);
		    return false;
		}
	    }
	}
    }
    unbound_names_list_clear(&list);
    CLEAR_ERROR(error);
    return true;
}

static void collect_unbound_names(ast_node_t* node, unbound_names_t* names) {
    if (node->name == UNBOUND_NAME) {
	unbound_names_append(names, node->value);
    } else if (node->name == TUPLE_LITERAL ||
	       node->name == LIST_LITERAL) {
	uint16_t n = ast_number_of_children(node);
	for (uint16_t i = 0; i < n; i++) {
	    collect_unbound_names(ast_get_child(node, i), names);
	}
    } else if (node->name == MAP_LITERAL ||
	       node->name == CONSTRUCTOR_LITERAL) {
	LOG_PANIC("Not implemented yet");
    }
}

static void forward_declare_function_definitions(ast_node_t* node,
						 symbol_tables_t* tables) {
    ast_node_t* top_level_defs_node = ast_get_child(node, 0);
    uint16_t n = ast_number_of_children(top_level_defs_node);
    for (uint16_t i = 0; i < n; i++) {
	ast_node_t* child_node = ast_get_child(top_level_defs_node, i);
	if (child_node->name == FUNCTION_DEF) {
	    ast_node_t* function_name_node = ast_get_child(child_node, 0);
	    if (function_name_node->name == EXPORT) {
		function_name_node = ast_get_child(child_node, 1);
	    }
	    LOG_ASSERT(function_name_node->name == FUNCTION_NAME,
		       "Expected a FUNCTION_NAME node");
	    type_t* type = type_new_type_variable();
	    function_name_node->type = type;
	    symbol_tables_insert(tables, function_name_node->value, type);
	}
    }
}

static bool create_type_variables(ast_node_t* node, symbol_tables_t* tables,
				  uint32_t block_expr_id,
				  satie_error_t* error) {
    bool traverse_children = true;
    if (node->name == AS ||
	node->name == ARGS ||
	node->name == ARG_TYPES ||
	node->name == CASE ||
	node->name == DEFAULT ||
	node->name == ELIF ||
	node->name == ELSE ||
	node->name == EXPORT ||
	node->name == EQ_TYPE ||
	node->name == INDEX_VALUE ||
	node->name == IS ||
	node->name == PARAMS ||
	node->name == PROGRAM ||
	node->name == TOP_LEVEL_DEFS ||
	node->name == TYPE ||
	node->name == TYPE_VARIABLES ||
	node->name == TYPES) {
	// Do not assign a type variable
    } else if (node->name == ADD_INT ||
	       node->name == ADD_FLOAT ||
     	       node->name == AND ||
	       node->name == BITWISE_AND ||
	       node->name == BITWISE_OR ||
	       node->name == BOOL_TYPE ||
	       node->name == BSL ||
	       node->name == BSR ||
	       node->name == CHAR ||
	       node->name == CHAR_TYPE ||
	       node->name == CONS ||
	       node->name == CONCAT_LIST ||
	       node->name == CONCAT_MAP ||
	       node->name == CONCAT_STRING ||
	       node->name == DIV_INT ||
	       node->name == DIV_FLOAT ||
	       node->name == EMPTY_LIST_TYPE ||
	       node->name == EMPTY_MAP_TYPE ||
	       node->name == EMPTY_TUPLE_TYPE ||
	       node->name == EQ ||
	       node->name == ESCAPE_CHAR ||
	       node->name == EXP ||
	       node->name == EXPRS ||
	       node->name == FALSE ||
	       node->name == FLOAT ||
	       node->name == FLOAT_TYPE ||
	       node->name == FUNCTION_CALL ||
	       node->name == GTE_INT ||
	       node->name == GT_INT ||
	       node->name == GTE_FLOAT ||
	       node->name == GT_INT ||
	       node->name == GT_FLOAT ||
	       node->name == IF ||
	       node->name == INT ||
	       node->name == INT_TYPE ||
	       node->name == FUNCTION_TYPE ||
	       node->name == LIST_LITERAL ||
	       node->name == LIST_LOOKUP ||
	       node->name == LIST_SLICE ||
	       node->name == LIST_TYPE ||
	       node->name == LIST_UPDATE ||
	       node->name == LTE_INT ||
	       node->name == LTE_FLOAT ||
	       node->name == LT_INT ||
	       node->name == LT_FLOAT ||
	       node->name == MAP_KEY_VALUE ||
	       node->name == MAP_LITERAL ||
	       node->name == MAP_LOOKUP ||
	       node->name == MAP_TYPE ||
	       node->name == MAP_UPDATE ||
	       node->name == NE ||
	       node->name == NEG_INT ||
	       node->name == NEG_FLOAT ||
	       node->name == MUL_INT ||
	       node->name == MUL_FLOAT ||
	       node->name == NOT ||
	       node->name == MOD ||
	       node->name == NE ||
	       node->name == NON_QUOTE_CHAR ||
	       node->name == OR ||
	       node->name == POS_INT ||
	       node->name == POS_FLOAT ||
	       node->name == POSTFIX ||
	       node->name == RAW_STRING ||
	       node->name == REGULAR_STRING ||
	       node->name == SLICE_LENGTH ||
	       node->name == STRING_TYPE ||
	       node->name == SUB_INT ||
	       node->name == SUB_FLOAT ||
	       node->name == SWITCH ||
	       node->name == TASK_TYPE ||
	       node->name == TRUE ||
	       node->name == TUPLE_LITERAL ||
	       node->name == TYPE_VARIABLE ||
	       node->name == TUPLE_TYPE ||
	       node->name == WHEN) {
	// Assign a type variable
	type_t* type = type_new_type_variable();
	node->type = type;
    } else if (node->name == NAME) {
	// Names should already be in a symbol table or else it is an error
	node->type = symbol_tables_lookup(tables, node->value);
	if (node->type == NULL) {
	    SET_ERROR_MESSAGE(
		error, COMPONENT_COMPILER,
		"%d: '%s' has not been bound to a value",
		node->row, node->value);
	    return false;
	}
    } else if (node->name == PARAM_NAME ||
	       node->name == UNBOUND_NAME ||
	       node->name == FUNCTION_NAME) {
	if (node->type == NULL) {
	    // Assign a type variable
	    type_t* type = type_new_type_variable();
	    node->type = type;
	    // Add the type variable to the symbol table
	    symbol_tables_insert(tables, node->value, type);
	}
    } else if (node->name == FUNCTION_DEF) {
	uint32_t block_expr_id = unique_id();
	symbol_table_t* table = symbol_table_new();
	symbol_tables_insert_table(tables, table, block_expr_id);
	// Add type variables to the function definition children nodes
	uint16_t n = ast_number_of_children(node);
	for (uint16_t i = 0; i < n; i++) {
	    if (!create_type_variables(ast_get_child(node, i), tables,
				       block_expr_id, error)) {
		CLEAR_ERROR(error);
		return false;
	    }
	}
	// Remove the symbol table created above and all nested symbol
	// tables created by bind expressions
	symbol_tables_delete_by_id(tables, block_expr_id);
	traverse_children = false;
    } else if (node->name == FUNCTION_LITERAL) {
	// Assign a type variable
	type_t* type = type_new_type_variable();
	node->type = type;
	// Create a new symbol table
	uint32_t block_expr_id = unique_id();
	symbol_table_t* table = symbol_table_new();
	symbol_tables_insert_table(tables, table, block_expr_id);
	// Add type variables to the function definition children nodes
	uint16_t n = ast_number_of_children(node);
	for (uint16_t i = 0; i < n; i++) {
	    if (!create_type_variables(ast_get_child(node, i), tables,
				       block_expr_id, error)) {
		CLEAR_ERROR(error);
		return false;
	    }
	}
	// Hoist the function name type variable
	ast_node_t* function_name_node = ast_get_child(node, 0);
	if (function_name_node->name == FUNCTION_NAME) {
	    symbol_tables_hoist(tables, function_name_node->value);
	}
	// Remove the symbol table created above and all nested symbol
	// tables created by bind expressions
	symbol_tables_delete_by_id(tables, block_expr_id);
	traverse_children = false;
    } else if (node->name == BIND) {
	// Assign a type variable
	type_t* type = type_new_type_variable();
	node->type = type;
	// Extract all nodes constituting the bind expression
	uint16_t index = 0;
	ast_node_t* left_node = ast_get_child(node, index);
	ast_node_t* is_node = NULL;
	ast_node_t* as_node = NULL;
	ast_node_t* right_node;
	ast_node_t* child_node = ast_get_child(node, index + 1);
	// Note: 'is' and 'as' can come in any order (if at all)
	if (child_node->name == IS) {
	    is_node = child_node;
	    child_node = ast_get_child(node, index + 2);
	    if (child_node->name == AS) {
		as_node = child_node;
		right_node = ast_get_child(node, index + 3);
	    } else {
		right_node = child_node;
	    }
	} else if (child_node->name == AS) {
	    as_node = child_node;
	    right_node = ast_get_child(node, index + 2);
	} else {
	    right_node = child_node;
	}
	// Take care of the right node first
	if (!create_type_variables(right_node, tables, block_expr_id, error)) {
	    CLEAR_ERROR(error);
	    return false;
	}
        // Create a new symbol table
	symbol_table_t* table = symbol_table_new();
	block_expr_id = unique_id();
	symbol_tables_insert_table(tables, table, block_expr_id);
	// Take care of the left node
	if (!create_type_variables(left_node, tables, block_expr_id, error)) {
	    CLEAR_ERROR(error);
	    return false;
	}
	// Take care of the is node
	if (is_node != NULL) {
	    if (!create_type_variables(is_node, tables, block_expr_id, error)) {
		CLEAR_ERROR(error);
		return false;
	    }
	}
	// Take care of the as node
	if (as_node != NULL) {
	    if (!create_type_variables(as_node, tables, block_expr_id, error)) {
		CLEAR_ERROR(error);
		return false;
	    }
	}
	traverse_children = false;
    } else if (node->name == BLOCK) {
	// Assign a type variable
	type_t* type = type_new_type_variable();
	node->type = type;
	// Create a new symbol table
	symbol_table_t* table = symbol_table_new();
	block_expr_id = unique_id();
	symbol_tables_insert_table(tables, table, block_expr_id);
	// Add type variables to the block expression children nodes
	uint16_t n = ast_number_of_children(node);
	for (uint16_t i = 0; i < n; i++) {
	    if (!create_type_variables(ast_get_child(node, i), tables,
				       block_expr_id, error)) {
		CLEAR_ERROR(error);
		return false;
	    }
	}
	// Remove the symbol table created above and all sub symbol
	// tables created by bind expressions
	symbol_tables_delete_by_id(tables, block_expr_id);
	traverse_children = false;
    } else {
	LOG_ABORT("Not handled node: %s\n",
		  ast_node_name_to_string(node->name));
    }

    // Traverse children (if any)
    if (traverse_children) {
	uint16_t n = ast_number_of_children(node);
	for (uint16_t i = 0; i < n; i++) {
	    if (!create_type_variables(ast_get_child(node, i), tables,
				       block_expr_id, error)) {
		return false;
	    }
	}
    }

    CLEAR_ERROR(error);
    return true;
}

static bool create_type_equations(ast_node_t *node, symbol_tables_t* tables,
				  equations_t* equations,
				  satie_error_t* error) {
    // Traverse children first (if any)
    uint16_t n = ast_number_of_children(node);
    for (uint16_t i = 0; i < n; i++) {
	if (!create_type_equations(ast_get_child(node, i), tables, equations,
				   error)) {
	    return false;
	}
    }

    if (node->name == ARGS ||
	node->name == ARG_TYPES ||
	node->name == AS ||
	node->name == BOOL_TYPE ||
	node->name == CASE ||
	node->name == CHAR_TYPE ||
	node->name == DEFAULT ||
	node->name == ELIF ||
	node->name == ELSE ||
	node->name == ESCAPE_CHAR ||
	node->name == EQ_TYPE ||
	node->name == EXPORT ||
	node->name == EXPRS ||
	node->name == FLOAT_TYPE ||
	node->name == FUNCTION_CALL ||
	node->name == FUNCTION_NAME ||
	node->name == FUNCTION_TYPE ||
	node->name == INT_TYPE ||
	node->name == INDEX_VALUE ||
	node->name == IS ||
	node->name == LIST_LOOKUP ||
	node->name == LIST_SLICE ||
	node->name == LIST_UPDATE ||
	node->name == MAP_KEY_VALUE ||
	node->name == MAP_LOOKUP ||
	node->name == MAP_UPDATE ||
	node->name == NON_QUOTE_CHAR ||
	node->name == NAME ||
	node->name == PARAMS ||
	node->name == PARAM_NAME ||
	node->name == PROGRAM ||
	node->name == SLICE_LENGTH ||
	node->name == STRING_TYPE ||
	node->name == TASK_TYPE ||
	node->name == TOP_LEVEL_DEFS ||
	node->name == TUPLE_TYPE ||
	node->name == TYPE_VARIABLE ||
	node->name == TYPE_VARIABLES ||
	node->name == TYPES ||
	node->name == UNBOUND_NAME ||
	node->name == WHEN) {
	// Do not create an equation
    } else if (node->name == TYPE) {
	// Equation: Type
	ast_node_t* type_node = ast_get_child(node, 0);
	type_t* type = extract_type(type_node);
	equation_t equation =
	    equation_new(type_node->type, type, node, type_node, true);
	equations_append(equations, &equation);
    } else if (node->name == FALSE || node->name == TRUE) {
	// Equation: Bool constant
	equation_t equation =
	    equation_new(node->type, type_new_basic_type(TYPE_BASIC_TYPE_BOOL),
			 node, node, false);
	equations_append(equations, &equation);
    } else if (node->name == INT) {
	// Equation: Int constant
	equation_t equation =
	    equation_new(node->type, type_new_basic_type(TYPE_BASIC_TYPE_INT),
			 node, node, false);
	equations_append(equations, &equation);
    } else if (node->name == FLOAT) {
	// Equation: Float constant
	equation_t equation =
	    equation_new(node->type, type_new_basic_type(TYPE_BASIC_TYPE_FLOAT),
			 node, node, false);
	equations_append(equations, &equation);
    } else if (node->name == CHAR) {
	// Equation: Char constant
	equation_t equation =
	    equation_new(node->type, type_new_basic_type(TYPE_BASIC_TYPE_CHAR),
			 node, node, false);
	equations_append(equations, &equation);
    } else if (node->name == RAW_STRING) {
	// Equation: String constant
	equation_t equation =
	    equation_new(node->type,
			 type_new_basic_type(TYPE_BASIC_TYPE_STRING),
			 node, node, false);
	equations_append(equations, &equation);
    } else if (node->name == REGULAR_STRING) {
	// Equation: String constant
	equation_t equation =
	    equation_new(node->type,
			 type_new_basic_type(TYPE_BASIC_TYPE_STRING),
			 node, node, false);
	equations_append(equations, &equation);
    } else if (node->name == TASK) {
	// Equation: Task constant
	equation_t equation =
	    equation_new(node->type, type_new_basic_type(TYPE_BASIC_TYPE_TASK),
			 node, node, false);
	equations_append(equations, &equation);
    } else if (node->name == CONSTRUCTOR_TYPE) {
	LOG_ABORT("Not implemented yet\n");
    } else if (node->name == LIST_TYPE) {
	// Equation: List
	ast_node_t* list_type_node = ast_get_child(node, 0);
	equation_t equation =
	    equation_new(node->type, type_new_list_type(list_type_node->type),
			 node, node, false);
	equations_append(equations, &equation);
    } else if (node->name == EMPTY_LIST_TYPE) {
	// Equation: Empty list
	equation_t equation =
	    equation_new(node->type, type_new_empty_list_type(), node, node,
			 false);
	equations_append(equations, &equation);
    } else if (node->name == MAP_TYPE) {
	// Equation: Map
	ast_node_t* key_type_node = ast_get_child(node, 0);
	ast_node_t* value_type_node = ast_get_child(node, 1);
	equation_t equation =
	    equation_new(node->type, type_new_map_type(key_type_node->type,
						       value_type_node->type),
			 node, node, false);
	equations_append(equations, &equation);
    } else if (node->name == EMPTY_MAP_TYPE) {
	// Equation: Empty map
	equation_t equation =
	    equation_new(node->type, type_new_empty_map_type(), node, node,
			 false);
	equations_append(equations, &equation);
    } else if (node->name == TUPLE_TYPE) {
	// Equation: Tuple
	types_t* types = types_new();
	uint16_t n = ast_number_of_children(node);
	for (uint16_t i = 0; i < n; i++) {
	    ast_node_t* type_node = ast_get_child(node, i);
	    types_add(types, type_node->type);
	}
	type_t* type = type_new_tuple_type(types);
	equation_t equation = equation_new(node->type, type, node, node, false);
	equations_append(equations, &equation);
    } else if (node->name == EMPTY_TUPLE_TYPE) {
	// Equation: Empty tuple
	equation_t equation =
	    equation_new(node->type, type_new_empty_tuple_type(), node, node,
			 false);
	equations_append(equations, &equation);
    } else if (node->name == FUNCTION_LITERAL) {
        // Extract all nodes constituting the function literal
	uint16_t index = 0;
	ast_node_t* child_node = ast_get_child(node, index);
	if (child_node->name == FUNCTION_NAME) {
	    child_node = ast_get_child(node, ++index);
	}
	ast_node_t* type_variables_node = NULL;
	ast_node_t* params_node = NULL;
	ast_node_t* return_type_node = NULL;
	ast_node_t* block_expr_node;
	if (child_node->name == TYPE_VARIABLES) {
	    type_variables_node = child_node;
	    child_node = ast_get_child(node, ++index);
	}
	if (child_node->name == PARAMS) {
	    params_node = child_node;
	    child_node = ast_get_child(node, ++index);
	}
	if (child_node->name == TYPE) {
	    return_type_node = child_node;
	    child_node = ast_get_child(node, index + 1);
	}
	block_expr_node = child_node;
	// Extract generic types (if any)
	types_t* generic_types = types_new();
	if (type_variables_node != NULL) {
	    uint16_t n = ast_number_of_children(type_variables_node);
	    for (uint16_t i = 0; i < n; i++) {
		ast_node_t* type_variable_node =
		    ast_get_child(type_variables_node, i);
		LOG_ASSERT(type_variable_node->name == TYPE_VARIABLE,
			   "Expected a TYPE_VARIABLE node");
		types_add(generic_types, type_variable_node->type);
	    }
	}
	// Extract all parameter types (if any)
	types_t* param_types = types_new();
	if (params_node != NULL) {
	    uint16_t number_of_type_variables = 0 ;
	    if (type_variables_node != NULL) {
		number_of_type_variables =
		    ast_number_of_children(type_variables_node);
	    }
	    uint16_t n = ast_number_of_children(params_node);
	    for (uint16_t i = 0; i < n; i++) {
		ast_node_t* param_node = ast_get_child(params_node, i);
		LOG_ASSERT(param_node->name == PARAM_NAME,
			   "Expected a PARAM_NAME node");
		// Has a type been specified?
		if (ast_number_of_children(param_node) > 0) {
		    ast_node_t* param_type_node = ast_get_child(param_node, 0);
		    LOG_ASSERT(param_type_node->name == TYPE,
			       "Expected a TYPE node");
		    ast_node_t* type_node = ast_get_child(param_type_node, 0);
		    if (!create_type_variable_equations(
			    equations, type_variables_node,
			    number_of_type_variables, type_node,
			    error)) {
			return false;
		    }
                    // Equation: Parameter
		    equation_t param_type_equation =
			equation_new(param_node->type, type_node->type, node
				     , param_node, true);
		    equations_append(equations, &param_type_equation);
		}
		types_add(param_types, param_node->type);
	    }
	}
	// Equation: Function
	equation_t function_equation =
	    equation_new(node->type,
			 type_new_function_type(generic_types, param_types,
						block_expr_node->type),
			 node, node, false);
	equations_append(equations, &function_equation);
	// Has a return type been specified?
	if (return_type_node != NULL) {
	    // Equation: Return type
	    ast_node_t* type_node = ast_get_child(return_type_node, 0);
	    equation_t return_type_equation =
		equation_new(block_expr_node->type, type_node->type, node,
			     return_type_node, true);
	    equations_append(equations, &return_type_equation);
	}
    } else if (node->name == LIST_LITERAL) {
	uint16_t n = ast_number_of_children(node);
	if (n == 0) {
	    // Equation: Empty list literal
	    equation_t equation = equation_new(
		node->type, type_new_empty_list_type(), node, node, false);
	    equations_append(equations, &equation);
	} else {
	    // Extract first list element
	    ast_node_t* first_element_node = ast_get_child(node, 0);
	    // Equation: List literal
	    type_t* type = type_new_list_type(first_element_node->type);
	    equation_t equation = equation_new(node->type, type, node,
					       first_element_node, false);
	    equations_append(equations, &equation);
	    // Add equations for all list elements
	    for (uint16_t i = 0; i < n; i++) {
		ast_node_t* element_node = ast_get_child(node, i);
		// Equation: List element
		equation_t equation =
		    equation_new(element_node->type, first_element_node->type,
				 node, element_node, false);
		equations_append(equations, &equation);
	    }
	}
    } else if (node->name == MAP_LITERAL) {
	uint16_t n = ast_number_of_children(node);
	if (n == 0) {
	    // Equation: Empty map literal
	    equation_t equation = equation_new(
		node->type, type_new_empty_map_type(), node, node, false);
	    equations_append(equations, &equation);
	} else {
	    // Extract first map key-value pair
	    ast_node_t* first_map_key_value_node = ast_get_child(node, 0);
	    LOG_ASSERT(first_map_key_value_node->name == MAP_KEY_VALUE,
		       "Expected a MAP_KEY_VALUE node");
	    ast_node_t* first_key_node =
		ast_get_child(first_map_key_value_node, 0);
	    ast_node_t* first_value_node =
		ast_get_child(first_map_key_value_node, 1);
            // Equation: Map literal
	    type_t* type = type_new_map_type(
		first_key_node->type, first_value_node->type);
	    equation_t equation = equation_new(node->type, type, node,
					       first_map_key_value_node, false);
	    equations_append(equations, &equation);
	    // Add equations for all key-value pairs
	    for (uint16_t i = 0; i < n; i++) {
		ast_node_t* map_key_value_node = ast_get_child(node, i);
		ast_node_t* key_node = ast_get_child(map_key_value_node, 0);
		ast_node_t* value_node = ast_get_child(map_key_value_node, 1);
		// Equation: Map key
		equation_t equation =
		    equation_new(key_node->type, first_key_node->type, node,
				 key_node, false);
		equations_append(equations, &equation);
		// Equation: Map value
		equation = equation_new(
		    value_node->type, first_value_node->type,
		    node, value_node, false);
		equations_append(equations, &equation);
	    }
	}
    } else if (node->name == TUPLE_LITERAL) {
	// Equation: Tuple literal
	uint16_t n = ast_number_of_children(node);
	if (n == 0) {
	    // Equation: Empty tuple literal
	    equation_t equation = equation_new(
		node->type, type_new_empty_tuple_type(), node, node, false);
	    equations_append(equations, &equation);
	} else {
	    types_t* types = types_new();
	    for (uint16_t i = 0; i < n; i++) {
		ast_node_t* element_node = ast_get_child(node, i);
		types_add(types, element_node->type);
	    }
	    type_t* type = type_new_tuple_type(types);
	    equation_t equation = equation_new(node->type, type, node, node,
					       false);
	    equations_append(equations, &equation);
	}
    } else if (node->name == EQ || node->name == NE) {
	// Equation: Equal or Not Equal
	equation_t operator_equation =
	    equation_new(node->type, type_new_basic_type(TYPE_BASIC_TYPE_BOOL),
			 node, node, false);
	equations_append(equations, &operator_equation);
        // Extract the type
	ast_node_t* type_node = ast_get_child(node, 1);
	type_t* type;
	if (type_node->name == EQ_TYPE) {
	    LOG_ABORT("Not implemented yet");
	} else {
	    LOG_ASSERT(type_node->name == INT_TYPE,
		       "Expected a INT_TYPE node");
	    type = type_node->type;
	}
        // Equation: Left operand
	ast_node_t* left_node = ast_get_child(node, 0);
	equation_t left_equation =
	    equation_new(left_node->type, type, node, left_node, false);
	equations_append(equations, &left_equation);
	// Equation: Right operand
	ast_node_t* right_node = ast_get_child(node, 2);
	equation_t right_equation =
	    equation_new(right_node->type, type, node, right_node, false);
	equations_append(equations, &right_equation);
    } else if (node->name == NEG_FLOAT ||
	       node->name == NEG_INT ||
	       node->name == NOT ||
	       node->name == POS_FLOAT ||
	       node->name == POS_INT) {
	operator_types_t types = get_operator_types(node->name);
	// Equation: Operator
	equation_t operator_equation =
	    equation_new(node->type, types.return_type, node, node, false);
	equations_append(equations, &operator_equation);
	// Equation: Operand
	ast_node_t* operand_node = ast_get_child(node, 0);
	equation_t operand_equation =
	    equation_new(operand_node->type, types.operand_type, node,
			 operand_node, false);
	equations_append(equations, &operand_equation);
    } else if (node->name == CONCAT_LIST) {
	ast_node_t* left_node = ast_get_child(node, 0);
	ast_node_t* right_node = ast_get_child(node, 1);
	// Equation: Operator
	equation_t operator_equation =
	    equation_new(node->type, left_node->type, node, left_node, false);
	equations_append(equations, &operator_equation);
	// Equation: Operands
	equation_t operands_equation =
	    equation_new(left_node->type, right_node->type, node, right_node,
			 false);
	equations_append(equations, &operands_equation);
    } else if (node->name == CONCAT_MAP) {
	ast_node_t* left_node = ast_get_child(node, 0);
	ast_node_t* right_node = ast_get_child(node, 1);
	// Equation: Operator
	equation_t operator_equation =
	    equation_new(node->type, left_node->type, node, left_node, false);
	equations_append(equations, &operator_equation);
	// Equation: Operands
	equation_t operands_equation =
	    equation_new(left_node->type, right_node->type, node, right_node,
			 false);
	equations_append(equations, &operands_equation);
    } else if (node->name == ADD_FLOAT ||
	       node->name == ADD_INT ||
	       node->name == AND ||
	       node->name == BITWISE_AND ||
	       node->name == BITWISE_OR ||
	       node->name == BSL ||
	       node->name == BSR ||
	       node->name == CONCAT_STRING ||
	       node->name == CONS ||
	       node->name == DIV_FLOAT ||
	       node->name == DIV_INT ||
	       node->name == EXP ||
	       node->name == GTE_FLOAT ||
	       node->name == GTE_INT ||
	       node->name == GT_FLOAT ||
	       node->name == GT_INT ||
	       node->name == LTE_FLOAT ||
	       node->name == LTE_INT ||
	       node->name == LT_FLOAT ||
	       node->name == LT_INT ||
	       node->name == MOD ||
	       node->name == MUL_FLOAT ||
	       node->name == MUL_INT ||
	       node->name == OR ||
	       node->name == SUB_INT ||
	       node->name == SUB_FLOAT) {
	operator_types_t types = get_operator_types(node->name);
	// Equation: Operator
	equation_t operator_equation =
	    equation_new(node->type, types.return_type, node, node, false);
	equations_append(equations, &operator_equation);
	// Equation: Left operand
	ast_node_t* left_node = ast_get_child(node, 0);
	equation_t left_equation =
	    equation_new(left_node->type, types.operand_type, node, left_node,
			 false);
	equations_append(equations, &left_equation);
	// Equation: Right operand
	ast_node_t* right_node = ast_get_child(node, 1);
	equation_t right_equation =
	    equation_new(right_node->type, types.operand_type, node,
			 right_node, false);
	equations_append(equations, &right_equation);
    } else if (node->name == CONS) {
	ast_node_t* left_node = ast_get_child(node, 0);
	// Equation: Operator
	equation_t operator_equation =
	    equation_new(node->type, type_new_list_type(left_node->type),
			 node, node, false);
	equations_append(equations, &operator_equation);
	// Equation: Right operand
	ast_node_t* right_node = ast_get_child(node, 1);
	equation_t right_equation =
	    equation_new(right_node->type, type_new_list_type(left_node->type),
			 node, right_node, false);
	equations_append(equations, &right_equation);
    } else if (node->name == IF) {
	ast_node_t* if_conditional_node = ast_get_child(node, 0);
	ast_node_t* if_block_expr_node = ast_get_child(node, 1);
	LOG_ASSERT(if_block_expr_node->name == BLOCK,
		   "Expected a BLOCK node");
	// Equation: if conditional
	equation_t if_conditional_equation =
	    equation_new(if_conditional_node->type,
			 type_new_basic_type(TYPE_BASIC_TYPE_BOOL),
			 node, if_conditional_node, false);
	equations_append(equations, &if_conditional_equation);
        // Equation: if block expression
	LOG_ASSERT(node->type != NULL, "Expected a type");
	equation_t if_block_expr_equation =
	    equation_new(if_block_expr_node->type, node->type, node,
			 if_block_expr_node, false);
	equations_append(equations, &if_block_expr_equation);
	// Extract elif nodes (if any) and else node
	uint16_t n = ast_number_of_children(node);
	for (uint16_t i = 2; i < n; i++) {
	    ast_node_t* child_node = ast_get_child(node, i);
	    if (child_node->name == ELIF) {
		ast_node_t* elif_node = child_node;
		ast_node_t* elif_conditional_node = ast_get_child(elif_node, 0);
		ast_node_t* elif_block_expr_node = ast_get_child(elif_node, 1);
		LOG_ASSERT(elif_block_expr_node->name == BLOCK,
			   "Expected a BLOCK node");
		// Equation: elif conditional
		equation_t elif_conditional_equation =
		    equation_new(elif_conditional_node->type,
				 type_new_basic_type(TYPE_BASIC_TYPE_BOOL),
				 node, elif_conditional_node, false);
		equations_append(equations, &elif_conditional_equation);
		// Equation: elif block expression
		equation_t elif_block_expr_equation =
		    equation_new(elif_block_expr_node->type, node->type, node,
				 elif_block_expr_node, false);
		equations_append(equations, &elif_block_expr_equation);
	    } else {
		LOG_ASSERT(child_node->name == ELSE, "Expected an ELSE node");
		ast_node_t* else_node = child_node;
		ast_node_t* else_block_expr_node = ast_get_child(else_node, 0);
		LOG_ASSERT(else_block_expr_node->name == BLOCK,
			   "Expected a BLOCK node");
		// Equation: else block expression
		equation_t else_block_expr_equation =
		    equation_new(else_block_expr_node->type, node->type, node,
				 else_block_expr_node, false);
		equations_append(equations, &else_block_expr_equation);
	    }
	}
    } else if (node->name == SWITCH) {
	ast_node_t* switch_expr_node = ast_get_child(node, 0);
	type_t* switch_expr_type = switch_expr_node->type;
	type_t* first_branch_block_type = NULL;
	// Go through all branches. The is and as constructs must
	// adhere to the type of the switch expression
	uint16_t n = ast_number_of_children(node);
	for (uint16_t i = 1; i < n; i++) {
	    ast_node_t* child_node = ast_get_child(node, i);
	    if (child_node->name == CASE) {
		// Extract all nodes constituting a case
		uint16_t index = 0;
		ast_node_t* case_node = child_node;
		ast_node_t* case_match_exprs_node =
		    ast_get_child(case_node, index);
		ast_node_t* as_node = NULL;
		ast_node_t* is_node = NULL;
		ast_node_t* block_node;
		child_node = ast_get_child(case_node, index + 1);
		if (child_node->name == AS) {
		    as_node = child_node;
		    child_node = ast_get_child(case_node, index + 2);
		    if (child_node->name == IS) {
			is_node = child_node;
			block_node = ast_get_child(case_node, index + 3);
		    } else {
			block_node = child_node;
		    }
		} else if (child_node->name == IS) {
		    is_node = child_node;
		    block_node = ast_get_child(case_node, index + 2);
		} else {
		    block_node = child_node;
		}
		// All case match expressions must adhere to the type
		// of the switch expression
		uint16_t n = ast_number_of_children(case_match_exprs_node);
		for (uint16_t j = 0; j < n; j++) {
		    ast_node_t* match_expr_node =
			ast_get_child(case_match_exprs_node, j);
		    // Equation: Case match expression
		    equation_t match_expr_equation =
			equation_new(switch_expr_type, match_expr_node->type,
				     node, match_expr_node, false);
		    equations_append(equations, &match_expr_equation);
		}
                // All branch blocks must be of same type
		if (first_branch_block_type == NULL) {
		    first_branch_block_type = block_node->type;
		} else {
		    // Equation: Branch block
		    equation_t block_equation =
			equation_new(block_node->type, first_branch_block_type,
				     node, block_node, false);
		    equations_append(equations, &block_equation);
		}
		// The as construct must adhere to the type of the
		// switch expression
		if (as_node != NULL) {
		    ast_node_t* unbound_name_node = ast_get_child(as_node, 0);
		    // Equation: as construct
		    equation_t as_equation =
			equation_new(unbound_name_node->type,
				     switch_expr_type, node, unbound_name_node,
				     true);
		    equations_append(equations, &as_equation);
		}
		// The is construct must adhere to the type of the
		// switch expression
		if (is_node != NULL) {
		    ast_node_t* type_node = ast_get_child(is_node, 0);
		    // Equation: is construct
		    equation_t is_equation =
			equation_new(type_node->type, switch_expr_type, node,
				     type_node, false);
		    equations_append(equations, &is_equation);
		}
	    } else {
		ast_node_t* default_node = child_node;
		ast_node_t* block_node = ast_get_child(default_node, 0);
                // Equation: Default block
		equation_t block_equation =
		    equation_new(block_node->type, first_branch_block_type,
				 node, block_node, false);
		equations_append(equations, &block_equation);
	    }
	}
        // Equation: Switch expression
	equation_t switch_equation =
	    equation_new(node->type,
			 first_branch_block_type, node, switch_expr_node,
			 false);
	equations_append(equations, &switch_equation);
    } else if (node->name == FUNCTION_DEF) {
        // Extract all nodes constituting the function definition
	uint16_t index = 0;
	ast_node_t* child_node = ast_get_child(node, index);
	ast_node_t* function_name_node = NULL;
	if (child_node->name == EXPORT) {
	    function_name_node = ast_get_child(node, ++index);
	} else {
	    function_name_node = child_node;
	}
	LOG_ASSERT(function_name_node->name == FUNCTION_NAME,
		   "Expected a FUNCTION_NAME node");
	child_node = ast_get_child(node, ++index);
	ast_node_t* type_variables_node = NULL;
	ast_node_t* params_node = NULL;
	ast_node_t* return_type_node = NULL;
	ast_node_t* block_expr_node;
	if (child_node->name == TYPE_VARIABLES) {
	    type_variables_node = child_node;
	    child_node = ast_get_child(node, ++index);
	}
	if (child_node->name == PARAMS) {
	    params_node = child_node;
	    child_node = ast_get_child(node, ++index);
	}
	if (child_node->name == TYPE) {
	    return_type_node = child_node;
	    child_node = ast_get_child(node, index + 1);
	}
	block_expr_node = child_node;
	// Extract generic types (if any)
	types_t* generic_types = types_new();
	if (type_variables_node != NULL) {
	    uint16_t n = ast_number_of_children(type_variables_node);
	    for (uint16_t i = 0; i < n; i++) {
		ast_node_t* type_variable_node =
		    ast_get_child(type_variables_node, i);
		LOG_ASSERT(type_variable_node->name == TYPE_VARIABLE,
			   "Expected a TYPE_VARIABLE node");
		types_add(generic_types, type_variable_node->type);
	    }
	}
        // Extract all parameter types (if any)
	types_t* param_types = types_new();
	if (params_node != NULL) {
	    uint16_t number_of_type_variables = 0 ;
	    if (type_variables_node != NULL) {
		number_of_type_variables =
		    ast_number_of_children(type_variables_node);
	    }
	    uint16_t n = ast_number_of_children(params_node);
	    for (uint16_t i = 0; i < n; i++) {
		ast_node_t* param_node =
		    ast_get_child(params_node, i);
		LOG_ASSERT(param_node->name == PARAM_NAME,
			   "Expected a PARAM_NAME node");
		// Has a type been specified?
		if (ast_number_of_children(param_node) > 0) {
		    ast_node_t* param_type_node = ast_get_child(param_node, 0);
		    LOG_ASSERT(param_type_node->name == TYPE,
			       "Expected a TYPE node");
		    ast_node_t* type_node = ast_get_child(param_type_node, 0);
		    if (!create_type_variable_equations(
			    equations, type_variables_node,
			    number_of_type_variables, type_node,
			    error)) {
			return false;
		    }
                    // Equation: Parameter
		    equation_t param_type_equation =
			equation_new(param_node->type, type_node->type,
				     node, param_node, true);
		    equations_append(equations, &param_type_equation);
		}
		types_add(param_types, param_node->type);
	    }
	}
	// Equation: Function
	equation_t function_equation =
	    equation_new(function_name_node->type,
			 type_new_function_type(generic_types, param_types,
						block_expr_node->type),
			 node, node, false);
	equations_append(equations, &function_equation);
	// Has a return type been specified?
	if (return_type_node != NULL) {
	    // Equation: Return type
	    ast_node_t* type_node = ast_get_child(return_type_node, 0);
	    equation_t return_type_equation =
		equation_new(block_expr_node->type, type_node->type, node,
			     return_type_node, true);
	    equations_append(equations, &return_type_equation);
	}
    } else if (node->name == BIND) {
	// Extract all nodes constituting the bind expression
	uint16_t index = 0;
	ast_node_t* left_node = ast_get_child(node, index);
	ast_node_t* is_node = NULL;
	ast_node_t* as_node = NULL;
	ast_node_t* right_node;
	ast_node_t* child_node = ast_get_child(node, index + 1);
      	// Note: 'is' and 'as' can come in any order (if at all)
	if (child_node->name == IS) {
	    is_node = child_node;
	    child_node = ast_get_child(node, index + 2);
	    if (child_node->name == AS) {
		as_node = child_node;
		right_node = ast_get_child(node, index + 3);
	    } else {
		right_node = child_node;
	    }
	} else if (child_node->name == AS) {
	    as_node = child_node;
	    right_node = ast_get_child(node, index + 2);
	} else {
	    right_node = child_node;
	}
        // Equation: Operands
	LOG_ASSERT(right_node->type != NULL, "Expected a type");
	equation_t bind_left_right_equation =
	    equation_new(left_node->type, right_node->type, node, left_node,
			 false);
	equations_append(equations, &bind_left_right_equation);
	// Equation: Bind
	LOG_ASSERT(right_node->type != NULL, "Expected a type");
	equation_t bind_equation =
	    equation_new(node->type, right_node->type, node, node, false);
	equations_append(equations, &bind_equation);
	// Equation: Is
	if (is_node != NULL) {
	    ast_node_t* type_node = ast_get_child(is_node, 0);
	    equation_t is_equation =
		equation_new(left_node->type, type_node->type, node, is_node,
			     false);
	    equations_append(equations, &is_equation);
	}
	// Equation: As
	if (as_node != NULL) {
	    ast_node_t* unbound_name_node = ast_get_child(as_node, 0);
	    equation_t as_equation =
		equation_new(unbound_name_node->type, left_node->type, node,
			     as_node, false);
	    equations_append(equations, &as_equation);
	}
    } else if (node->name == POSTFIX) {
	ast_node_t* postfix_expr_node = ast_get_child(node, 0);
	uint16_t n = ast_number_of_children(node);
	ast_node_t* postfix_expr_child_node;
	for (uint16_t i = 1; i < n; i++) {
	    postfix_expr_child_node = ast_get_child(node, i);
	    if (postfix_expr_child_node->name == FUNCTION_CALL) {
		// Extract all nodes constituting the function call
		ast_node_t* function_call_node = postfix_expr_child_node;
		ast_node_t* types_node = NULL;
		ast_node_t* args_node = NULL;
		uint16_t index = 0;
		ast_node_t* function_call_child_node =
		    ast_get_child(function_call_node, index);
		if (function_call_child_node != NULL &&
		    function_call_child_node->name == TYPES) {
		    types_node = function_call_child_node;
		    function_call_child_node =
			ast_get_child(function_call_node, index + 1);
		}
		args_node = function_call_child_node;
		// Extract generic types (if any)
		types_t* generic_types = types_new();
		if (types_node != NULL) {
		    uint16_t n = ast_number_of_children(types_node);
		    for (uint16_t i = 0; i < n; i++) {
			ast_node_t* type_node = ast_get_child(types_node, i);
			LOG_ASSERT(type_node->name == TYPE,
				   "Expected a TYPE node");
			ast_node_t* type_node_child =
			    ast_get_child(type_node, 0);
			types_add(generic_types, type_node_child->type);
		    }
		}
                // Extract all argument types (if any)
		types_t* arg_types = types_new();
		if (args_node != NULL) {
		    uint16_t n = ast_number_of_children(args_node);
		    for (uint16_t i = 0; i < n; i++) {
			ast_node_t* arg_node = ast_get_child(args_node, i);
			types_add(arg_types, arg_node->type);
		    }
		}
		// Equation: Function call
		equation_t function_equation =
		    equation_new(
			postfix_expr_node->type,
			type_new_function_type(generic_types, arg_types,
					       function_call_node->type),
			node, node, false);
		equations_append(equations, &function_equation);
		// Update postfix expression
		postfix_expr_node = function_call_node;
	    } else if (postfix_expr_child_node->name == LIST_LOOKUP) {
		ast_node_t* list_lookup_node = postfix_expr_child_node;
		ast_node_t* index_node = ast_get_child(list_lookup_node, 0);
		// Equation: Index equation
		equation_t index_equation =
		    equation_new(index_node->type,
				 type_new_basic_type(TYPE_BASIC_TYPE_INT),
				 node, index_node, false);
		equations_append(equations, &index_equation);
		// Equation: List lookup
		equation_t list_lookup_equation =
		    equation_new(postfix_expr_node->type,
				 type_new_list_type(list_lookup_node->type),
				 node, node, false);
		equations_append(equations, &list_lookup_equation);
		// Update postfix expression
		postfix_expr_node = list_lookup_node;
	    } else if (postfix_expr_child_node->name == LIST_UPDATE) {
		ast_node_t* list_update_node = postfix_expr_child_node;
		// Extract first index-value pair
		ast_node_t* first_index_value_node =
		    ast_get_child(list_update_node, 0);
		LOG_ASSERT(first_index_value_node->name == INDEX_VALUE,
			   "Expected a INDEX_VALUE node");
		ast_node_t* first_index_node =
		    ast_get_child(first_index_value_node, 0);
		ast_node_t* first_value_node =
		    ast_get_child(first_index_value_node, 1);
		// Equation: List literal
		type_t* type = type_new_list_type(first_value_node->type);
		equation_t equation =
		    equation_new(list_update_node->type, type, node,
				 first_index_value_node, false);
		equations_append(equations, &equation);
                // Add equations for all index-value pairs
		uint16_t n = ast_number_of_children(list_update_node);
		for (uint16_t i = 0; i < n; i++) {
		    ast_node_t* index_value_node =
			ast_get_child(list_update_node, i);
		    LOG_ASSERT(index_value_node->name == INDEX_VALUE,
			       "Expected a INDEX_VALUE node");
		    ast_node_t* index_node = ast_get_child(index_value_node, 0);
		    ast_node_t* value_node = ast_get_child(index_value_node, 1);
		    // Equation: Index equation
		    equation_t index_equation =
			equation_new(index_node->type, first_index_node->type,
				     node, index_node, false);
		    equations_append(equations, &index_equation);
		    // Equation: Value
		    equation_t value_equation =
			equation_new(value_node->type, first_value_node->type,
				     node, value_node, false);
		    equations_append(equations, &value_equation);
		}
		// Equation: List update
		equation_t list_update_equation =
		    equation_new(postfix_expr_node->type,
				 type_new_list_type(first_value_node->type),
				 node, node, false);
		equations_append(equations, &list_update_equation);
		// Update postfix expression
		postfix_expr_node = list_update_node;
	    } else if (postfix_expr_child_node->name == LIST_SLICE) {
		ast_node_t* list_slice_node = postfix_expr_child_node;
		ast_node_t* start_node = ast_get_child(list_slice_node, 0);
		ast_node_t* end_node = ast_get_child(list_slice_node, 1);
		// Equation: List literal
		type_t* type = type_new_list_type(end_node->type);
		equation_t equation =
		    equation_new(list_slice_node->type, type, node, end_node,
				 false);
		equations_append(equations, &equation);
		// Equation: Start equation
		equation_t start_equation =
		    equation_new(start_node->type,
				 type_new_basic_type(TYPE_BASIC_TYPE_INT),
				 node, start_node, false);
		equations_append(equations, &start_equation);
                // Equation: End equation
		equation_t end_equation =
		    equation_new(end_node->type,
				 type_new_basic_type(TYPE_BASIC_TYPE_INT),
				 node, end_node, false);
		equations_append(equations, &end_equation);
		// Update postfix expression
		postfix_expr_node = list_slice_node;
	    } else if (postfix_expr_child_node->name == MAP_LOOKUP) {
		ast_node_t* map_lookup_node = postfix_expr_child_node;
		ast_node_t* key_node = ast_get_child(map_lookup_node, 0);
		// Equation: Map lookup
		equation_t map_lookup_equation =
		    equation_new(postfix_expr_node->type,
				 type_new_map_type(key_node->type,
						   map_lookup_node->type),
				 node, node, false);
		equations_append(equations, &map_lookup_equation);
		// Update postfix expression
		postfix_expr_node = map_lookup_node;
	    } else if (postfix_expr_child_node->name == MAP_UPDATE) {
		ast_node_t* map_update_node = postfix_expr_child_node;
                // Extract first key-value pair
		ast_node_t* first_map_key_value_node =
		    ast_get_child(map_update_node, 0);
		LOG_ASSERT(first_map_key_value_node->name == MAP_KEY_VALUE,
			   "Expected a MAP_KEY_VALUE node");
		ast_node_t* first_key_node =
		    ast_get_child(first_map_key_value_node, 0);
		ast_node_t* first_value_node =
		    ast_get_child(first_map_key_value_node, 1);
		// Equation: Map literal
		type_t* type = type_new_map_type(first_key_node->type,
						 first_value_node->type);
		equation_t equation =
		    equation_new(map_update_node->type, type, node,
				 first_map_key_value_node, false);
		equations_append(equations, &equation);
                // Add equations for all key-value pairs
		uint16_t n = ast_number_of_children(map_update_node);
		for (uint16_t i = 0; i < n; i++) {
		    ast_node_t* map_key_value_node =
			ast_get_child(map_update_node, i);
		    LOG_ASSERT(map_key_value_node->name == MAP_KEY_VALUE,
			       "Expected a MAP_KEY_VALUE node");
		    ast_node_t* key_node = ast_get_child(map_key_value_node, 0);
		    ast_node_t* value_node =
			ast_get_child(map_key_value_node, 1);
		    // Equation: Key equation
		    equation_t key_equation =
			equation_new(key_node->type, first_key_node->type,
				     node, key_node, false);
		    equations_append(equations, &key_equation);
		    // Equation: Value
		    equation_t value_equation =
			equation_new(value_node->type, first_value_node->type,
				     node, value_node, false);
		    equations_append(equations, &value_equation);
		}
		// Update postfix expression
		postfix_expr_node = map_update_node;
	    } else {
		assert(false);
	    }
	}
        // Equation: Postfix expression
	equation_t postfix_expr_equation =
	    equation_new(node->type, postfix_expr_child_node->type, node, node,
			 false);
	equations_append(equations, &postfix_expr_equation);
    } else if (node->name == BLOCK) {
	// Equation: Block expression
	ast_node_t* last_block_expr_node = ast_last_child(node);
	LOG_ASSERT(last_block_expr_node->type != NULL, "Expected a type");
	equation_t equation =
	    equation_new(node->type, last_block_expr_node->type, node, node,
			 false);
	equations_append(equations, &equation);
    } else {
	LOG_ABORT("Not handled node: %s\n",
		  ast_node_name_to_string(node->name));
    }

    return true;
}

static bool create_type_variable_equations(equations_t* equations,
					   ast_node_t* type_variables_node,
					   uint16_t number_of_type_variables,
					   ast_node_t* type_node,
					   satie_error_t* error) {
    if (type_node->name == TYPE_VARIABLE) {
	for (uint16_t i = 0; i < number_of_type_variables; i++) {
	    ast_node_t* type_variable_node =
		ast_get_child(type_variables_node, i);
	    LOG_ASSERT(type_variable_node->name == TYPE_VARIABLE,
		       "Expected a TYPE_VARIABLE node");
	    if (strcmp(type_variable_node->value, type_node->value) == 0) {
		// Equation: Type variable
		equation_t equation =
		    equation_new(type_node->type, type_variable_node->type,
				 type_variables_node, type_node, true);
		equations_append(equations, &equation);
		return true;
	    }
	}
	SET_ERROR_MESSAGE(error, COMPONENT_COMPILER,
			  "%d: Unknown type variable '%s'\n",
			  type_node->row, type_node->value);

	return false;
    } else if (type_node->name == LIST_TYPE) {
	if (!create_type_variable_equations(equations, type_variables_node,
					    number_of_type_variables,
					    ast_get_child(type_node, 0),
					    error)) {
	    return false;
	}
    } else if (type_node->name == FUNCTION_TYPE) {
	// Extract all nodes constituting the function type
	uint16_t index = 0;
	ast_node_t* type_variables_node = NULL;
	ast_node_t* arg_types_node = NULL;
	ast_node_t* return_type_node;
	ast_node_t* child_node = ast_get_child(type_node, index);
	if (child_node->name == TYPE_VARIABLES) {
	    type_variables_node = child_node;
	    child_node = ast_get_child(type_node, ++index);
	}
	if (child_node->name == ARG_TYPES) {
	    arg_types_node = child_node;
	    child_node = ast_get_child(type_node, ++index);
	}
	return_type_node = child_node;
	if (type_variables_node != NULL) {
	    uint16_t n = ast_number_of_children(arg_types_node);
	    for (uint16_t i = 0; i < n; i++) {
		if (!create_type_variable_equations(
			equations, type_variables_node,
			number_of_type_variables,
			ast_get_child(arg_types_node, i), error)) {
		    return false;
		}
	    }
	    if (!create_type_variable_equations(
		    equations, type_variables_node, number_of_type_variables,
		    return_type_node, error)) {
		return false;
	    }
	}
    } else if (type_node->name == TUPLE_TYPE) {
	uint16_t n = ast_number_of_children(type_node);
	for (uint16_t i = 0; i < n; i++) {
	    if (!create_type_variable_equations(
		    equations, type_variables_node, number_of_type_variables,
		    ast_get_child(type_node, i), error)) {
		return false;
	    }
	}
    } else if (type_node->name == MAP_TYPE) {
	ast_node_t* key_type_node = ast_get_child(type_node, 0);
	ast_node_t* value_type_node = ast_get_child(type_node, 1);
	if (!create_type_variable_equations(
		equations, type_variables_node, number_of_type_variables,
		key_type_node, error)) {
	    return false;
	}
	if (!create_type_variable_equations(
		equations, type_variables_node, number_of_type_variables,
		value_type_node, error)) {
	    return false;
	}

    } else if (type_node->name == CONSTRUCTOR_TYPE) {
	ast_node_t* types_node = ast_get_child(type_node, 1);
	uint16_t n = ast_number_of_children(types_node);
	for (uint16_t i = 0; i < n; i++) {
	    if (!create_type_variable_equations(
		    equations, type_variables_node, number_of_type_variables,
		    ast_get_child(types_node, i), error)) {
		return false;
	    }
	}
    }
    CLEAR_ERROR(error);
    return true;
}

static type_t* extract_type(ast_node_t* type_node) {
    switch (type_node->name) {
	case BOOL_TYPE:
	    return type_new_basic_type(TYPE_BASIC_TYPE_BOOL);
	case INT_TYPE:
	    return type_new_basic_type(TYPE_BASIC_TYPE_INT);
	case FLOAT_TYPE:
	    return type_new_basic_type(TYPE_BASIC_TYPE_FLOAT);
	case CHAR_TYPE:
	    return type_new_basic_type(TYPE_BASIC_TYPE_CHAR);
	case STRING_TYPE:
	    return type_new_basic_type(TYPE_BASIC_TYPE_STRING);
	case TASK_TYPE:
	    return type_new_basic_type(TYPE_BASIC_TYPE_TASK);
	case CONSTRUCTOR_TYPE: {
	    ast_node_t* name_node = ast_get_child(type_node, 0);
	    LOG_ASSERT(name_node->name == NAME, "Expected a NAME node");
	    types_t* types = types_new();
	    for (uint16_t i = 1; i < ast_number_of_children(type_node); i++) {
		ast_node_t* type_node = ast_get_child(type_node, i);
		type_t* type = extract_type(type_node);
		types_add(types, type);
	    }
	    return type_new_constructor_type(name_node->value, types);
	}
	case FUNCTION_TYPE: {
	    // Extract all nodes constituting the function type
	    uint16_t index = 0;
	    ast_node_t* child_node = ast_get_child(type_node, index);
	    ast_node_t* type_variables_node = NULL;
	    ast_node_t* arg_types_node = NULL;
	    ast_node_t* return_type_node;
	    if (child_node->name == TYPE_VARIABLES) {
		type_variables_node = child_node;
		child_node = ast_get_child(type_node, ++index);
	    }
	    if (child_node->name == ARG_TYPES) {
		arg_types_node = child_node;
		child_node = ast_get_child(type_node, index + 1);
	    }
	    return_type_node = child_node;
            // Extract generic types (if any)
	    types_t* generic_types = types_new();
	    if (type_variables_node != NULL) {
		for (uint16_t i = 0;
		     i < ast_number_of_children(type_variables_node); i++) {
		    ast_node_t* type_variable_node =
			ast_get_child(type_variables_node, i);
		    type_t* generic_type = extract_type(type_variable_node);
		    types_add(generic_types, generic_type);
		}
	    }
	    // Argument types (if any)
	    types_t* arg_types = types_new();
	    for (uint16_t i = 0; i < ast_number_of_children(arg_types_node);
		 i++) {
		ast_node_t* arg_type_node = ast_get_child(arg_types_node, i);
		type_t* arg_type = extract_type(arg_type_node);
		types_add(arg_types, arg_type);
	    }
	    // Return type
	    type_t* return_type = extract_type(return_type_node);
	    return type_new_function_type(generic_types, arg_types,
					  return_type);
	}
	case LIST_TYPE:
	    return type_new_list_type(
		extract_type(ast_get_child(type_node, 0)));
	case EMPTY_LIST_TYPE:
	    return type_new_empty_list_type();
	case MAP_TYPE: {
	    ast_node_t* key_type_node = ast_get_child(type_node, 0);
	    type_t* key_type = extract_type(key_type_node);
	    ast_node_t* value_type_node = ast_get_child(type_node, 1);
	    type_t* value_type = extract_type(value_type_node);
	    return type_new_map_type(key_type, value_type);
	}
	case EMPTY_MAP_TYPE:
	    return type_new_empty_map_type();
	case TUPLE_TYPE: {
	    types_t* tuple_types = types_new();
	    for (uint16_t i = 0; i < ast_number_of_children(type_node); i++) {
		ast_node_t* tuple_type_node = ast_get_child(type_node, i);
		type_t* tuple_type = extract_type(tuple_type_node);
		types_add(tuple_types, tuple_type);
	    }
	    return type_new_tuple_type(tuple_types);
	}
	case EMPTY_TUPLE_TYPE:
	    return type_new_empty_tuple_type();
	case TYPE_VARIABLE:
	    return type_new_type_variable();
	default:
	    LOG_ABORT("Unknown type node: %s",
		      ast_node_name_to_string(type_node->name));
    }
    assert(false);
}

static operator_types_t get_operator_types(node_name_t name) {
    switch (name) {
	case ADD_FLOAT:
	case EXP:
	case DIV_FLOAT:
	case SUB_FLOAT:
	case MUL_FLOAT:
	    return (operator_types_t) {
		.operand_type = type_new_basic_type(TYPE_BASIC_TYPE_FLOAT),
		.return_type = type_new_basic_type(TYPE_BASIC_TYPE_FLOAT)
	    };
	case ADD_INT:
	case DIV_INT:
	case MOD:
	case MUL_INT:
	case SUB_INT:
	    return (operator_types_t) {
		.operand_type = type_new_basic_type(TYPE_BASIC_TYPE_INT),
		.return_type = type_new_basic_type(TYPE_BASIC_TYPE_INT)
	    };
	case AND:
	case OR:
	    return (operator_types_t) {
		.operand_type = type_new_basic_type(TYPE_BASIC_TYPE_BOOL),
		.return_type = type_new_basic_type(TYPE_BASIC_TYPE_BOOL)
	    };
	case BITWISE_AND:
	case BITWISE_OR:
	case BSL:
	case BSR:
	    return (operator_types_t) {
		.operand_type = type_new_basic_type(TYPE_BASIC_TYPE_INT),
		.return_type = type_new_basic_type(TYPE_BASIC_TYPE_BOOL)
	    };
	case CONCAT_STRING:
	    return (operator_types_t) {
		.operand_type = type_new_basic_type(TYPE_BASIC_TYPE_STRING),
		.return_type = type_new_basic_type(TYPE_BASIC_TYPE_STRING)
	    };
	case GT_FLOAT:
	case GTE_FLOAT:
	case LT_FLOAT:
	case LTE_FLOAT:
	    return (operator_types_t) {
		.operand_type = type_new_basic_type(TYPE_BASIC_TYPE_FLOAT),
		.return_type = type_new_basic_type(TYPE_BASIC_TYPE_BOOL)
	    };
	case GT_INT:
	case GTE_INT:
	case LT_INT:
	case LTE_INT:
	    return (operator_types_t) {
		.operand_type = type_new_basic_type(TYPE_BASIC_TYPE_INT),
		.return_type = type_new_basic_type(TYPE_BASIC_TYPE_BOOL)
	    };
	case NEG_FLOAT:
	case POS_FLOAT:
	    return (operator_types_t) {
		.operand_type = type_new_basic_type(TYPE_BASIC_TYPE_FLOAT),
		.return_type = type_new_basic_type(TYPE_BASIC_TYPE_FLOAT)
	    };
	case NEG_INT:
	case POS_INT:
	    return (operator_types_t) {
		.operand_type = type_new_basic_type(TYPE_BASIC_TYPE_INT),
		.return_type = type_new_basic_type(TYPE_BASIC_TYPE_INT)
	    };
	case NOT:
	    return (operator_types_t) {
		.operand_type = type_new_basic_type(TYPE_BASIC_TYPE_BOOL),
		.return_type = type_new_basic_type(TYPE_BASIC_TYPE_BOOL)
	    };
	default:
	    assert(false);
    }
    assert(false);
}
