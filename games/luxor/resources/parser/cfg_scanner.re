/* Luxor Configuration File Lexer for re2c */

#include "luxor/resources/parser/cfg_scanner_context.h"
#include "luxor/resources/parser/cfg_parser_context.h"
#include "cfg_parser.h"

int cfg_scan_token(scanner_context_t* ctx, token_value_t* token) {
    const char* start;

scan:
    if (ctx->cursor >= ctx->eof) {
        if (ctx->last_token != TOKEN_EOL && ctx->last_token != 0) {
            ctx->last_token = TOKEN_EOL;
            token->start = ctx->cursor;
            token->end = ctx->cursor;
            token->line = ctx->line;
            token->column = ctx->column;
            return TOKEN_EOL;
        }
        ctx->last_token = 0;
        return 0;
    }

    start = ctx->cursor;
    token->start = start;
    token->line = ctx->line;
    token->column = ctx->column;

    /*!re2c
        re2c:define:YYCTYPE = "unsigned char";
        re2c:define:YYCURSOR = ctx->cursor;
        re2c:define:YYLIMIT = ctx->limit;
        re2c:define:YYMARKER = ctx->marker;
        re2c:yyfill:enable = 0;

        [ \t\v\f]+ {
            ctx->column += (int)(ctx->cursor - start);
            goto scan;
        }

        "//" [^\r\n]* {
            ctx->column += (int)(ctx->cursor - start);
            goto scan;
        }

        ";" [^\r\n]* {
            ctx->column += (int)(ctx->cursor - start);
            goto scan;
        }

        "\r\n" | [\r\n] {
            ctx->line++;
            ctx->column = 1;
            token->end = ctx->cursor;
            ctx->last_token = TOKEN_EOL;
            return TOKEN_EOL;
        }

        "\x00" {
            if (ctx->last_token != TOKEN_EOL && ctx->last_token != 0) {
                ctx->last_token = TOKEN_EOL;
                token->end = ctx->cursor;
                return TOKEN_EOL;
            }
            token->end = ctx->cursor;
            ctx->last_token = 0;
            return 0;
        }

        "=" {
            ctx->column += (int)(ctx->cursor - start);
            token->end = ctx->cursor;
            ctx->last_token = TOKEN_EQUALS;
            return TOKEN_EQUALS;
        }

        "[" {
            ctx->column += (int)(ctx->cursor - start);
            token->end = ctx->cursor;
            ctx->last_token = TOKEN_LBRACKET;
            return TOKEN_LBRACKET;
        }

        "]" {
            ctx->column += (int)(ctx->cursor - start);
            token->end = ctx->cursor;
            ctx->last_token = TOKEN_RBRACKET;
            return TOKEN_RBRACKET;
        }

        "{" {
            ctx->column += (int)(ctx->cursor - start);
            token->end = ctx->cursor;
            ctx->last_token = TOKEN_LBRACE;
            return TOKEN_LBRACE;
        }

        "}" {
            ctx->column += (int)(ctx->cursor - start);
            token->end = ctx->cursor;
            ctx->last_token = TOKEN_RBRACE;
            return TOKEN_RBRACE;
        }

        "|" {
            ctx->column += (int)(ctx->cursor - start);
            token->end = ctx->cursor;
            ctx->last_token = TOKEN_PIPE;
            return TOKEN_PIPE;
        }

        "true" | "false" {
            ctx->column += (int)(ctx->cursor - start);
            token->end = ctx->cursor;
            ctx->last_token = TOKEN_BOOLEAN;
            return TOKEN_BOOLEAN;
        }

        [-+]? ([0-9]+ ("." [0-9]*)? | "." [0-9]+) ([eE] [-+]? [0-9]+)? {
            ctx->column += (int)(ctx->cursor - start);
            token->end = ctx->cursor;
            ctx->last_token = TOKEN_NUMBER;
            return TOKEN_NUMBER;
        }

        ["] [^"\r\n]* ["] {
            ctx->column += (int)(ctx->cursor - start);
            token->end = ctx->cursor;
            ctx->last_token = TOKEN_STRING;
            return TOKEN_STRING;
        }

        ("http://" | "https://") [a-zA-Z0-9_./\\~?&=%#:-]+ {
            ctx->column += (int)(ctx->cursor - start);
            token->end = ctx->cursor;
            ctx->last_token = TOKEN_IDENTIFIER;
            return TOKEN_IDENTIFIER;
        }

        [a-zA-Z_][a-zA-Z0-9_./\\-]* {
            ctx->column += (int)(ctx->cursor - start);
            token->end = ctx->cursor;
            ctx->last_token = TOKEN_IDENTIFIER;
            return TOKEN_IDENTIFIER;
        }

        * {
            ctx->column += (int)(ctx->cursor - start);
            token->end = ctx->cursor;
            ctx->last_token = TOKEN_UNKNOWN;
            return TOKEN_UNKNOWN;
        }
    */
}
