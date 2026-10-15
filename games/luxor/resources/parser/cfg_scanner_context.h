//
// Created by igor on 11/10/2026.
//

#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct scanner_context {
    const char* input;
    const char* cursor;
    const char* limit;
    const char* eof;
    const char* marker;
    int line;
    int column;
    int last_token;
    const char* filename;
} scanner_context_t;

typedef struct token_value {
    const char* start;
    const char* end;
    int line;
    int column;
} token_value_t;

int cfg_scan_token(scanner_context_t* ctx, token_value_t* token);

#ifdef __cplusplus
}
#endif
