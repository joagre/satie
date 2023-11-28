#ifndef __SATIE_H__
#define __SATIE_H__

#include "return_types.h"

#define SUCCESS 0
#define PARAMETER_ERROR 1

#define DEFAULT_CHECK_AFTER 100
#define DEFAULT_LOAD_PATH "./"
#define DEFAULT_TIME_SLICE 25

void usage(const char* name, uint16_t check_after, const char* load_path,
           uint32_t time_slice);
long_result_t string_to_long(const char* string);

#endif