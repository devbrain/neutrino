//
// Created by igor on 11/10/2026.
//

#pragma once

#include <stddef.h>
#include "luxor/resources/parser/cfg_scanner_context.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cfg_parser_context {
    void* user_data;
    int error_count;
    char error_message[256];
} cfg_parser_context_t;

void cfg_parser_on_property_key(cfg_parser_context_t* ctx, const token_value_t* token);
void cfg_parser_on_value_string(cfg_parser_context_t* ctx, const token_value_t* token);
void cfg_parser_on_value_number(cfg_parser_context_t* ctx, const token_value_t* token);
void cfg_parser_on_value_bool(cfg_parser_context_t* ctx, const token_value_t* token);
void cfg_parser_on_value_ident(cfg_parser_context_t* ctx, const token_value_t* token);
void cfg_parser_on_property_finish(cfg_parser_context_t* ctx);
void cfg_parser_on_section_header(cfg_parser_context_t* ctx, const token_value_t* token);
void cfg_parser_on_block_header(cfg_parser_context_t* ctx, const token_value_t* type, const token_value_t* name);
void cfg_parser_on_attached_block_start(cfg_parser_context_t* ctx);
void cfg_parser_on_block_finish(cfg_parser_context_t* ctx);
void cfg_parser_on_syntax_error(cfg_parser_context_t* ctx);
void cfg_parser_on_parse_failure(cfg_parser_context_t* ctx);

#ifdef __cplusplus
}
#endif
