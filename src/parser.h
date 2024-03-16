#ifndef PARSER_H
#define PARSER_H

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IDENTIFIER_LENGTH 64
#define RECORDS_LENGTH 32

/*
 * String view APIs definition
 * A string view is merely a pointer to an existing string, or better, to a
 * slice of an existing string (which may be of entire target string length),
 * thus it's not nul-terminated
 */
typedef struct {
    size_t length;
    const char *p;
} String_View;

// Function to create a string view from a given source string and length
String_View string_view_from_parts(const char *src, size_t len);

// Function to create a string view from a null-terminated source string
String_View string_view_from_cstring(const char *src);

// Function to chop a string view by a delimiter and return the remaining view
String_View string_view_chop_by_delim(String_View *view, const char delim);

/*
 * Basic lexer, breaks down the input string (in the form of a String_View)
 * splitting it by space or ',' to allow the extraction of tokens.
 */
typedef struct {
    String_View view;
    size_t length;
} Lexer;

// Function to get the next token from the lexer
String_View lexer_next(Lexer *l);

// Function to get the next token by a separator from the lexer
String_View lexer_next_by_sep(Lexer *l, char sep);

// Function to peek at the next token from the lexer without consuming it
String_View lexer_peek(Lexer *l);

// Define token types
typedef enum {
    TOKEN_CREATE,
    TOKEN_INSERT,
    TOKEN_INTO,
    TOKEN_TIMESTAMP,
    TOKEN_LITERAL,
    TOKEN_SELECT,
    TOKEN_FROM,
    TOKEN_AT,
    TOKEN_RANGE,
    TOKEN_TO,
    TOKEN_WHERE,
    TOKEN_OPERATOR_EQ,
    TOKEN_OPERATOR_NE,
    TOKEN_OPERATOR_LE,
    TOKEN_OPERATOR_LT,
    TOKEN_OPERATOR_GE,
    TOKEN_OPERATOR_GT,
    TOKEN_AGGREGATE,
    TOKEN_AGGREGATE_FN,
    TOKEN_BY
} Token_Type;

// Define token structure
typedef struct {
    Token_Type type;
    char value[IDENTIFIER_LENGTH];
} Token;

// Define aggregate function types
typedef enum { AF_AVG, AF_MIN, AF_MAX } Aggregate_Function;

// Define operator types
typedef enum { OP_EQ, OP_NE, OP_GE, OP_GT, OP_LE, OP_LT } Operator;

/*
 * Select mask, to define the kind of type of query
 * - Single point lookup
 * - Range of points
 * - With a WHERE clause
 * - With an aggregation function
 * - With an interval to aggregate on
 */
enum Select_Mask {
    SM_SINGLE = 0x01,
    SM_RANGE = 0x02,
    SM_WHERE = 0x04,
    SM_AGGREGATE = 0x08,
    SM_BY = 0x10
};

// Define structure for CREATE statement
typedef struct {
    char db_name[IDENTIFIER_LENGTH];
    char ts_name[IDENTIFIER_LENGTH];
    uint8_t mask;
} Statement_Create;

// Define a pair (timestamp, value) for INSERT statements
typedef struct {
    uint64_t timestamp;
    double_t value;
} Create_Record;

// Define structure for INSERT statement
typedef struct {
    size_t record_len;
    char db_name[IDENTIFIER_LENGTH];
    char ts_name[IDENTIFIER_LENGTH];
    Create_Record records[RECORDS_LENGTH];
} Statement_Insert;

// Define structure for WHERE clause in SELECT statement
typedef struct {
    char key[IDENTIFIER_LENGTH];
    Operator operator;
    double_t value;
} Statement_Where;

// Define structure for SELECT statement
typedef struct {
    char db_name[IDENTIFIER_LENGTH];
    char ts_name[IDENTIFIER_LENGTH];
    int64_t start_time;
    int64_t end_time;
    Aggregate_Function af;
    Statement_Where where;
    uint64_t interval;
    uint8_t mask;
} Statement_Select;

// Define statement types
typedef enum {
    STATEMENT_UNKNOW,
    STATEMENT_CREATE,
    STATEMENT_INSERT,
    STATEMENT_SELECT
} Statement_Type;

// Define a generic statement
typedef struct {
    Statement_Type type;
    union {
        Statement_Create create;
        Statement_Insert insert;
        Statement_Select select;
    };
} Statement;

// Function to tokenize input string into an array of tokens
Token *tokenize(const char *input, size_t *token_count);

// Function to parse CREATE statement from tokens
Statement_Create parse_create(Token *tokens, size_t token_count);

// Function to parse INSERT statement from tokens
Statement_Insert parse_insert(Token *tokens, size_t token_count);

// Function to parse SELECT statement from tokens
Statement_Select parse_select(Token *tokens, size_t token_count);

// Parse a statement
Statement parse(Token *tokens, size_t token_count);

// Debug helpers

// Function to print CREATE statement
void print_create(Statement_Create *create);

// Function to print INSERT statement
void print_insert(Statement_Insert *insert);

// Function to print SELECT statement
void print_select(Statement_Select *select);

#endif // PARSER_H
