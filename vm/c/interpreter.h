#ifndef __INTERPRETER_H__
#define __INTERPRETER_H__

typedef enum {
    INTERPRETER_RESULT_HALT,
    INTERPRETER_RESULT_RECV,
    INTERPRETER_RESULT_TIMEOUT,
    INTERPRETER_RESULT_EXIT
} interpreter_result_t;

#endif