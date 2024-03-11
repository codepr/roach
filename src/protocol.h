#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <math.h>
#include <stdint.h>
#include <stdio.h>

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

Token *tokenize(const char *input);
AST_Node *parse(Token *tokens, size_t len);
void print_ast(AST_Node *node);
void ast_free(AST_Node *node);

#endif
