#include "protocol.h"
#include "logging.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

String_View string_view_from_parts(const char *src, size_t len) {
    String_View view = {.length = len, .p = src};
    return view;
}

String_View string_view_from_cstring(const char *src) {
    return string_view_from_parts(src, strlen(src));
}

String_View string_view_chop_by_delim(String_View *view, const char delim) {
    size_t i = 0;
    while (i < view->length && view->p[i] != delim) {
        i += 1;
    }

    String_View result = string_view_from_parts(view->p, i);

    if (i < view->length) {
        view->length -= i + 1;
        view->p += i + 1;
    } else {
        view->length -= i;
        view->p += i;
    }

    return result;
}

Token *tokenize(const char *input) {
    static Token tokens[20];
    String_View input_view = string_view_from_cstring(input);
    size_t i = 0;

    while (input_view.length > 0) {
        String_View token = string_view_chop_by_delim(&input_view, ' ');

        snprintf(tokens[i].value, token.length + 1, "%s", token.p);

        if (strncmp(token.p, "DB.CREATE", 9) == 0)
            tokens[i].type = TOKEN_DB_CREATE;
        else if (strncmp(token.p, "TS.CREATE", 9) == 0)
            tokens[i].type = TOKEN_TS_CREATE;
        else if (strncmp(token.p, "TS.INSERT", 9) == 0)
            tokens[i].type = TOKEN_TS_INSERT;
        else if (strncmp(token.p, "TS.QUERY", 7) == 0)
            tokens[i].type = TOKEN_TS_QUERY;
        else
            switch (tokens[i - 1].type) {
            case TOKEN_DB_CREATE:
            case TOKEN_TS_CREATE:
            case TOKEN_TS_INSERT:
                tokens[i].type = TOKEN_IDENTIFIER;
                break;
            case TOKEN_TS_QUERY:
                tokens[i].type = TOKEN_TS_MODIFIER;
                break;
            case TOKEN_TS_MODIFIER:
                tokens[i].type = TOKEN_IDENTIFIER;
                break;
            case TOKEN_IDENTIFIER:
                tokens[i].type = TOKEN_LITERAL;
                break;
            case TOKEN_LITERAL:
                tokens[i].type = TOKEN_LITERAL;
                break;
            }
        i++;
    }

    return tokens;
}

AST_Node *parse(Token *tokens, size_t len) {

    if (len == 0)
        return NULL;

    AST_Node *root = malloc(sizeof(*root));
    root->left = NULL;
    root->right = NULL;

    switch (tokens[0].type) {
    case TOKEN_DB_CREATE:
        // CREATE DATABASE
        root->type = NODE_DB_CREATE;
        snprintf(root->value, sizeof(root->value), "%s", tokens[0].value);
        break;
    case TOKEN_TS_CREATE:
        // CREATE TIMESERIES
        root->type = NODE_TS_CREATE;
        snprintf(root->value, sizeof(root->value), "%s", tokens[0].value);
        break;
    case TOKEN_TS_INSERT:
    case TOKEN_TS_QUERY:
        root->type =
            tokens[0].type == TOKEN_TS_INSERT ? NODE_TS_INSERT : NODE_TS_QUERY;
        snprintf(root->value, sizeof(root->value), "%s", tokens[0].value);
        break;
    default:
        free(root);
        return NULL;
    }

    AST_Node *node = root;
    // Parse the command parameters
    for (size_t i = 1; i < len; i++) {
        if (tokens[i].type == TOKEN_LITERAL ||
            tokens[i].type == TOKEN_IDENTIFIER ||
            tokens[i].type == TOKEN_TS_MODIFIER) {
            // Create a new AST node for each parameter
            AST_Node *param_node = malloc(sizeof(*param_node));
            param_node->type = tokens[i].type == TOKEN_LITERAL ? NODE_LITERAL
                               : tokens[i].type == TOKEN_IDENTIFIER
                                   ? NODE_IDENTIFIER
                                   : NODE_TS_MODIFIER;
            snprintf(param_node->value, sizeof(param_node->value), "%s",
                     tokens[i].value);
            param_node->left = NULL;
            param_node->right = NULL;

            // Attach the parameter node to the root node
            if (node->left == NULL) {
                node->left = param_node;
                node = node->left;
            } else if (node->right == NULL) {
                node->right = param_node;
                node = node->right;
            } else {
                // Handle unexpected extra parameters
                free(param_node);
                // Free already allocated nodes
                ast_free(root);
                return NULL;
            }
        } else {
            // Handle invalid parameter type
            // Free already allocated nodes
            ast_free(root);
            return NULL;
        }
    }

    return root;
}

void ast_free(AST_Node *node) {
    if (node == NULL)
        return;
    ast_free(node->right);
    ast_free(node->left);
    free(node);
}

static char *strnode[7] = {
    "NODE_DB_CREATE",   "NODE_TS_CREATE",  "NODE_DB_INSERT", "NODE_TS_QUERY",
    "NODE_TS_MODIFIER", "NODE_IDENTIFIER", "NODE_LITERAL",
};

void print_ast(AST_Node *node) {
    if (node == NULL) {
        return;
    }
    printf("%s(%s)\n", strnode[node->type], node->value);
    print_ast(node->right);
    print_ast(node->left);
}
