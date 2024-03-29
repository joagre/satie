#ifndef UTILS_LOG_H
#define UTILS_LOG_H

#include <stdbool.h>
#include <stdint.h>
#include "satie_error.h"

typedef enum {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_PANIC
} log_level_t;

void log_entry_variadic(log_level_t log_level, char* file, uint32_t line,
			char* format, ...);
void log_entry(log_level_t log_level, char* file, uint32_t line, char* format,
               va_list args);
void log_satie_error(log_level_t log_level, char* file, uint32_t line,
		     satie_error_t* error);
void log_abort(char* file, uint32_t line, char* message, ...);
void log_assert(char* file, uint32_t line, bool condition, char* message, ...);

#ifdef DEBUG

// LOG_SATIE_ERROR
#ifndef MUTE_LOG_SATIE_ERROR
#define LOG_SATIE_ERROR(log_level, error) ({		   \
	    log_satie_error(log_level, __FILE__, __LINE__, error); \
})
#else
#define LOG_SATIE_ERROR(log_level, error) ((void)0)
#endif

// LOG_DEBUG
#ifndef MUTE_LOG_DEBUG
#define LOG_DEBUG(message, ...) ({ \
    log_entry_variadic(LOG_LEVEL_DEBUG, __FILE__ , __LINE__, \
		       message, ##__VA_ARGS__); \
})
#else
#define LOG_DEBUG(message, ...) ((void)0)
#endif

// LOG_INFO
#ifndef MUTE_LOG_INFO
#define LOG_INFO(message, ...) ({ \
    log_entry_variadic(LOG_LEVEL_INFO, __FILE__ , __LINE__, message, \
		       ##__VA_ARGS__); \
})
#else
#define LOG_INFO(message, ...) ((void)0)
#endif

// LOG_WARNING
#ifndef MUTE_LOG_WARNING
#define LOG_WARNING(message, ...) ({ \
    log_entry_variadic(LOG_LEVEL_WARNING, __FILE__, __LINE__, message, \
		       ##__VA_ARGS__); \
})
#else
#define LOG_WARNING(message, ...) ((void)0)
#endif

// LOG_ERROR
#define LOG_ERROR(message, ...) ({ \
    log_entry_variadic(LOG_LEVEL_ERROR, __FILE__, __LINE__, message, \
		       ##__VA_ARGS__); \
})

// LOG_PANIC
#define LOG_PANIC(message, ...) ({ \
    log_entry_variadic(LOG_LEVEL_PANIC, __FILE__, __LINE__, message, \
		       ##__VA_ARGS__); \
})

// LOG_ABORT
#define LOG_ABORT(message, ...) ({ \
    log_abort(__FILE__, __LINE__, message, ##__VA_ARGS__); \
})

// LOG_ASSERT
#define LOG_ASSERT(condition, message, ...) ({ \
    log_assert(__FILE__, __LINE__, condition, message, ##__VA_ARGS__); \
})

#else

#define LOG_SATIE_ERROR(log_level, error) ((void)0)
#define LOG_DEBUG(message, ...) ((void)0)
#define LOG_INFO(message, ...) ((void)0)
#define LOG_WARNING(message, ...) ((void)0)
#define LOG_ERROR(message, ...) ((void)0)
#define LOG_PANIC(message, ...) ((void)0)
#define LOG_ABORT(message, ...) ((void)0)
#define LOG_ASSERT(condition, message, ...) ((void)0)

#endif

#endif
