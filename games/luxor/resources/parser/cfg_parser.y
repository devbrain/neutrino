/* Luxor Config / Level / INI / PSYS Parser Grammar for Lemon */

%include {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "luxor/resources/parser/cfg_scanner_context.h"
#include "luxor/resources/parser/cfg_parser_context.h"

/* Forward declarations for Lemon */
void* CfgParseAlloc(void* (*allocProc)(size_t));
void CfgParse(void*, int, token_value_t, cfg_parser_context_t*);
void CfgParseFree(void*, void(*freeProc)(void*));
void CfgParseTrace(FILE *TraceFILE, char *zTracePrompt);
}

%name CfgParse
%token_type {token_value_t}
%extra_argument {cfg_parser_context_t* ctx}
%token_prefix TOKEN_

%syntax_error {
    cfg_parser_on_syntax_error(ctx);
}

%parse_failure {
    cfg_parser_on_parse_failure(ctx);
}

file ::= items.

items ::= .
items ::= items item.

item ::= EOL.
item ::= prop_header EQUALS value_list EOL. {
    cfg_parser_on_property_finish(ctx);
}
item ::= section_header EOL.
item ::= block.
item ::= attached_block.
item ::= UNKNOWN EOL. {
    cfg_parser_on_syntax_error(ctx);
}

section_header ::= LBRACKET section_name(S) RBRACKET. {
    cfg_parser_on_section_header(ctx, &S);
}

section_name(A) ::= IDENTIFIER(B). { A = B; }
section_name(A) ::= STRING(B). { A = B; }

block ::= block_decl opt_eols LBRACE EOL block_items RBRACE EOL. {
    cfg_parser_on_block_finish(ctx);
}

attached_block ::= attached_block_header block_items RBRACE EOL. {
    cfg_parser_on_block_finish(ctx);
}

attached_block_header ::= LBRACE. {
    cfg_parser_on_attached_block_start(ctx);
}

block_decl ::= IDENTIFIER(T) IDENTIFIER(N). {
    cfg_parser_on_block_header(ctx, &T, &N);
}
block_decl ::= IDENTIFIER(N). {
    cfg_parser_on_block_header(ctx, NULL, &N);
}

opt_eols ::= .
opt_eols ::= opt_eols EOL.

block_items ::= .
block_items ::= block_items block_item.

block_item ::= EOL.
block_item ::= prop_header EQUALS value_list EOL. {
    cfg_parser_on_property_finish(ctx);
}
block_item ::= attached_block.
block_item ::= block.
block_item ::= UNKNOWN EOL. {
    cfg_parser_on_syntax_error(ctx);
}

prop_header ::= IDENTIFIER(K). {
    cfg_parser_on_property_key(ctx, &K);
}

value_list ::= .
value_list ::= value_list value.
value_list ::= value_list PIPE.

value ::= STRING(V). {
    cfg_parser_on_value_string(ctx, &V);
}
value ::= NUMBER(V). {
    cfg_parser_on_value_number(ctx, &V);
}
value ::= BOOLEAN(V). {
    cfg_parser_on_value_bool(ctx, &V);
}
value ::= IDENTIFIER(V). {
    cfg_parser_on_value_ident(ctx, &V);
}
