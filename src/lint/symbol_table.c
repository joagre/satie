//#define MUTE_LOG_DEBUG 1

#include <assert.h>
#include <log.h>

#include "symbol_table.h"

// Forward declarations of local functions (alphabetical order)
static int key_cmp(void* key, hlink_t* link, void* arg);
static size_t key_hash(void* key, void*);

symbol_table_t* symbol_table_new(void) {
    symbol_table_t* table = malloc(sizeof(symbol_table_t));
    lhash_kv_init(table, NULL, key_hash, key_cmp);
    return table;
}

void symbol_table_free(symbol_table_t* table) {
    lhash_kv_clear(table);
    free(table);
}

void symbol_table_insert(symbol_table_t* table, char* name, type_t* type) {
    lhash_kv_insert(table, (void*)name, (void*)type);
}

void symbol_table_delete(symbol_table_t* table, char* name) {
    lhash_kv_remove(table, (void*)name, NULL);
}

type_t* symbol_table_lookup(symbol_table_t* table, char* name) {
    type_t* type = NULL;
    lhash_kv_find(table, (void*)name, (void**)&type);
    return type;
}

void symbol_table_merge(symbol_table_t* table, symbol_table_t* other) {
    lhash_kv_iter_t iter;
    lhash_kv_iter_init(&iter, other);
    while (!lhash_kv_iter_end(&iter)) {
	char* name;
	type_t* type;
	lhash_kv_iter_current(&iter, (void**)&name, (void**)&type);
	symbol_table_insert(table, name, type);
	lhash_kv_iter_next(&iter);
    }
}

void symbol_table_print(symbol_table_t* table) {
    lhash_kv_iter_t iter;
    lhash_kv_iter_init(&iter, table);
    while (!lhash_kv_iter_end(&iter)) {
	char* name;
	type_t* type;
	lhash_kv_iter_current(&iter, (void**)&name, (void**)&type);
	printf("%s: ", name);
	switch (type->tag) {
	    case TYPE_TAG_BASIC_TYPE:
		switch (type->basic_type) {
		    case TYPE_BASIC_TYPE_INT:
			printf("Int\n");
			break;
		    case TYPE_BASIC_TYPE_BOOL:
			printf("Bool\n");
			break;
		    default:
			assert(false);
		}
		break;
	    case TYPE_TAG_TYPE_VARIABLE:
		printf("t%d\n", type->type_variable.id);
		break;
	    case TYPE_TAG_FUNCTION_TYPE:
		printf("Function\n");
		break;
	    default:
		assert(false);
	}
	lhash_kv_iter_next(&iter);
    }
}

//
// Local functions (alphabetical order)
//

static int key_cmp(void* key, hlink_t* link, void* arg) {
    hlink_kv_t* link_kv	= (hlink_kv_t*)link;
    return strcmp((char*)key, (char*)link_kv->key);
}

static size_t key_hash(void* key, void* arg) {
    size_t hash = 0;
    int c;
    while ((c = *(char*)key++)) {
        hash = c + (hash << 6) + (hash << 16) - hash;
    }
    return hash;
};

//
// Unit test
//

void symbol_table_unit_test(void) {
    symbol_table_t* table = symbol_table_new();
    type_t* type1 = type_new_basic_type(TYPE_BASIC_TYPE_INT);
    symbol_table_insert(table, "int", type1);
    type_t* type2 = type_new_basic_type(TYPE_BASIC_TYPE_BOOL);
    symbol_table_insert(table, "bool", type2);
    type_t* type3 = type_new_type_variable(NULL);
    symbol_table_insert(table, "bar", type3);
    type_t* type = symbol_table_lookup(table, "int");
    LOG_ASSERT(type->tag == TYPE_TAG_BASIC_TYPE, "Wrong type");
    LOG_INFO("Unit test passed");
}
