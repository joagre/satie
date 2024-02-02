#include "equation.h"

equation_t equation_new(type_t* left, type_t* right, ast_node_t* origin_node,
			ast_node_t* node) {
    equation_t equation = {
	.left = left,
	.right = right,
	.origin_node = node,
	.node = node
    };
    return equation;
}
