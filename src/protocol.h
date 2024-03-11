#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "timeseries.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define METRIC_MAX_LEN 1 << 9

// String view APIs definition
// A string view is merely a pointer to an existing string, or better, to a
// slice of an existing string (which may be of entire target string length),
// thus it's not nul-terminated

typedef struct {
    size_t length;
    const char *p;
} String_View;

String_View string_view_from_parts(const char *src, size_t len);

String_View string_view_from_cstring(const char *src);

String_View string_view_chop_by_delim(String_View *view, const char delim);

typedef enum {
    TOKEN_DB_CREATE,
    TOKEN_TS_CREATE,
    TOKEN_TS_INSERT,
    TOKEN_TS_QUERY,
    TOKEN_TS_MODIFIER,
    TOKEN_IDENTIFIER,
    TOKEN_LITERAL
} Token_Type;

typedef struct {
    Token_Type type;
    char value[64];
} Token;

typedef enum {
    NODE_DB_CREATE,
    NODE_TS_CREATE,
    NODE_TS_INSERT,
    NODE_TS_QUERY,
    NODE_TS_MODIFIER,
    NODE_IDENTIFIER,
    NODE_LITERAL
} Node_Type;

typedef struct ast_node {
    Node_Type type;
    char value[64];
    struct ast_node *left;
    struct ast_node *right;
} AST_Node;

// Breaks the line into tokens based on spaces
Token *tokenize(const char *input);

// Parses an array of tokens into an AST
AST_Node *parse(Token *tokens, size_t len);

// Prints an AST pre-order
void print_ast(AST_Node *node);

// Frees memory of an AST
void ast_free(AST_Node *node);

typedef enum { CMD_CREATE, CMD_INSERT, CMD_QUERY } Command_Type;

typedef struct command_create {
    int is_db;
    char name[64];
    uint64_t retention;
    Duplication_Policy policy;
} Command_Create;

typedef struct command_insert {
    char ts_name[64];
    uint64_t timestamp;
    double_t value;
} Command_Insert;

typedef struct command_query {
    char ts_name[64];
    int range;
    uint64_t start_ts;
    uint64_t end_ts;
} Command_Query;

typedef struct command {
    Command_Type type;
    union {
        Command_Create create;
        Command_Insert insert;
        Command_Query query;
    };

} Command;

Command parse_ast(AST_Node *root);

#endif
