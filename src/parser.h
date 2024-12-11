#ifndef PARSER_H
#define PARSER_H

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IDENTIFIER_LENGTH 64
#define RECORDS_LENGTH    32

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

typedef struct token Token;

// Define aggregate function types
typedef enum { AFN_AVG, AFN_MIN, AFN_MAX } Aggregate_Function;

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
typedef enum Select_Mask {
    SM_SINGLE    = 0x01,
    SM_RANGE     = 0x02,
    SM_WHERE     = 0x04,
    SM_AGGREGATE = 0x08,
    SM_BY        = 0x10
} Select_Mask;

// Define structure for CREATE statement
typedef struct {
    char db_name[IDENTIFIER_LENGTH];
    char ts_name[IDENTIFIER_LENGTH];
    uint8_t mask;
} Statement_Create;

// Define a pair (timestamp, value) for INSERT statements
typedef struct {
    int64_t timestamp;
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
    Select_Mask mask;
} Statement_Select;

// Define statement types
typedef enum {
    STATEMENT_EMPTY,
    STATEMENT_CREATE,
    STATEMENT_INSERT,
    STATEMENT_SELECT,
    STATEMENT_UNKNOWN
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

// Parse a statement
Statement parse(const char *input);

// Debug helpers

void print_statement(const Statement *statement);

#endif // PARSER_H
