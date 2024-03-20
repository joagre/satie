#ifndef __UNBOUND_NAMES_H__
#define __UNBOUND_NAMES_H__

#include <stdbool.h>
#include <dynarr.h>

typedef dynarray_t unbound_names_t;

unbound_names_t* unbound_names_new(void);
void unbound_names_free(unbound_names_t* names);
void unbound_names_append(unbound_names_t* names, char* name);
char* unbound_names_get(unbound_names_t* names, uint16_t i);
uint16_t unbound_names_size(unbound_names_t* names);
bool unbound_names_member(unbound_names_t* names, char* name);
void unbound_names_print(unbound_names_t* names);

#endif
