#include "parser.h"
#include <string.h>

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

String_View lexer_next(Lexer *l) {
    String_View lexiom = string_view_chop_by_delim(&l->view, ' ');
    l->length = l->view.length;
    return lexiom;
}

String_View lexer_next_by_sep(Lexer *l, char sep) {
    String_View lexiom = string_view_chop_by_delim(&l->view, sep);
    l->length = l->view.length;
    return lexiom;
}

String_View lexer_peek(Lexer *l) {
    size_t length = l->length;
    String_View lexiom = lexer_next(l);
    l->view.p -= length - l->length;
    l->view.length = length;
    return lexiom;
}

static Token tokens[20];

static Token *tokenize_create(Lexer *l, size_t *token_count) {
    String_View token = lexer_next(l);
    size_t i = 0;

    tokens[i].type = TOKEN_CREATE;
    strncpy(tokens[i].value, token.p, token.length);

    while (l->length > 0 && ++i) {
        token = lexer_next(l);

        if (strncmp(token.p, "INTO", token.length) == 0) {
            tokens[i].type = TOKEN_INTO;
            token = lexer_next(l);
            strncpy(tokens[i].value, token.p, token.length);
            // TODO retention and duplication policy
        }
    }

    *token_count = i;

    return tokens;
}

static Token *tokenize_insert(Lexer *l, size_t *token_count) {
    String_View token = lexer_next(l);
    size_t i = 0;

    tokens[i].type = TOKEN_INSERT;
    strncpy(tokens[i].value, token.p, token.length);
    token = lexer_next(l);

    ++i;

    if (strncmp(token.p, "INTO", token.length) == 0) {
        tokens[i].type = TOKEN_INTO;
        token = lexer_next(l);
        strncpy(tokens[i].value, token.p, token.length);
    }

    while (l->length > 0 && ++i) {
        // Timestamp, can also be * meaning set it automatically server
        // side.
        // At the moment we don't care of splitting on ',' and keeping
        // spaces around, they're all numeric values anyway and will be
        // parsed int proper type integer/double values
        token = lexer_next_by_sep(l, ',');
        tokens[i].type = TOKEN_TIMESTAMP;
        strncpy(tokens[i].value, token.p, token.length);
        ++i;
        token = lexer_next_by_sep(l, ',');
        tokens[i].type = TOKEN_LITERAL;
        strncpy(tokens[i].value, token.p, token.length);
    }

    *token_count = i;
    return tokens;
}

static Token *tokenize_select(Lexer *l, size_t *token_count) {
    String_View token = lexer_next(l);
    size_t i = 0;

    tokens[i].type = TOKEN_SELECT;
    strncpy(tokens[i].value, token.p, token.length);

    while (l->length > 0 && ++i) {
        token = lexer_next(l);

        if (strncmp(token.p, "FROM", token.length) == 0) {
            tokens[i].type = TOKEN_FROM;
            token = lexer_next(l);
            strncpy(tokens[i].value, token.p, token.length);
        } else if (strncmp(token.p, "AT", token.length) == 0) {
            tokens[i].type = TOKEN_AT;
            token = lexer_next(l);
            int64_t num;
            if (sscanf(token.p, "%li", &num) == 1)
                strncpy(tokens[i].value, token.p, token.length);
        } else if (strncmp(token.p, "RANGE", token.length) == 0) {
            tokens[i].type = TOKEN_RANGE;
            token = lexer_next(l);
            int64_t num;
            if (sscanf(token.p, "%li", &num) == 1)
                strncpy(tokens[i].value, token.p, token.length);
            // TOOD error here, missing the start timestamp
        } else if (strncmp(token.p, "TO", token.length) == 0) {
            tokens[i].type = TOKEN_TO;
            token = lexer_next(l);
            int64_t num;
            if (sscanf(token.p, "%li", &num) == 1)
                strncpy(tokens[i].value, token.p, token.length);
            // TOOD error here, missing the start timestamp
        } else if (strncmp(token.p, "WHERE", token.length) == 0) {
            tokens[i].type = TOKEN_WHERE;
            token = lexer_next(l);
            strncpy(tokens[i].value, token.p, token.length);
        } else if (strncmp(token.p, "AGGREGATE", token.length) == 0) {
            tokens[i].type = TOKEN_AGGREGATE;
        } else if (strncmp(token.p, "AVG", token.length) == 0) {
            tokens[i].type = TOKEN_AGGREGATE_FN;
        } else if (strncmp(token.p, "BY", token.length) == 0) {
            tokens[i].type = TOKEN_BY;
            token = lexer_next(l);
            uint64_t num;
            if (sscanf(token.p, "%lu", &num) == 1)
                strncpy(tokens[i].value, token.p, token.length);
            // TOOD error here, missing the start timestamp
        } else {
            if (strncmp(token.p, "=", token.length) == 0) {
                tokens[i].type = TOKEN_OPERATOR_EQ;
            } else if (strncmp(token.p, "<", token.length) == 0) {
                tokens[i].type = TOKEN_OPERATOR_LT;
            } else if (strncmp(token.p, ">", token.length) == 0) {
                tokens[i].type = TOKEN_OPERATOR_GT;
            } else if (strncmp(token.p, "<=", token.length) == 0) {
                tokens[i].type = TOKEN_OPERATOR_LE;
            } else if (strncmp(token.p, ">=", token.length) == 0) {
                tokens[i].type = TOKEN_OPERATOR_GE;
            } else if (strncmp(token.p, "!=", token.length) == 0) {
                tokens[i].type = TOKEN_OPERATOR_NE;
            }
            token = lexer_next(l);
            uint64_t num;
            if (sscanf(token.p, "%lu", &num) == 1)
                strncpy(tokens[i].value, token.p, token.length);
        }
    }

    *token_count = i;

    return tokens;
}

Token *tokenize(const char *query, size_t *token_count) {
    Token *tokens = NULL;

    String_View view = string_view_from_cstring(query);
    Lexer l = {.view = view, .length = view.length};
    String_View first_token = lexer_next(&l);

    if (strncmp(first_token.p, "CREATE", first_token.length) == 0)
        tokens = tokenize_create(&l, token_count);
    else if (strncmp(first_token.p, "INSERT", first_token.length) == 0)
        tokens = tokenize_insert(&l, token_count);
    else if (strncmp(first_token.p, "SELECT", first_token.length) == 0)
        tokens = tokenize_select(&l, token_count);

    (*token_count)++;

    return tokens;
}

Statement_Create parse_create(Token *tokens, size_t token_count) {
    Statement_Create create = {.mask = 0};

    if (token_count == 1) {
        snprintf(create.db_name, sizeof(create.db_name), "%s", tokens[0].value);
    } else {
        for (size_t i = 0; i < token_count; ++i) {
            if (tokens[i].type == TOKEN_CREATE) {
                snprintf(create.ts_name, sizeof(create.ts_name), "%s",
                         tokens[i].value);
            } else if (tokens[i].type == TOKEN_INTO) {
                snprintf(create.db_name, sizeof(create.db_name), "%s",
                         tokens[i].value);
                create.mask = 1;
            }
            // TODO error here
        }
    }

    return create;
}

Statement_Insert parse_insert(Token *tokens, size_t token_count) {
    Statement_Insert insert;
    char *endptr = NULL;
    size_t j = 0;

    for (size_t i = 0; i < token_count; ++i) {
        if (tokens[i].type == TOKEN_INSERT) {
            snprintf(insert.ts_name, sizeof(insert.ts_name), "%s",
                     tokens[i].value);
        } else if (tokens[i].type == TOKEN_INTO) {
            snprintf(insert.db_name, sizeof(insert.db_name), "%s",
                     tokens[i].value);
        } else if (tokens[i].type == TOKEN_TIMESTAMP) {
            // Crude check for empty timestamp
            if (tokens[i].value[0] == '*')
                insert.records[j].timestamp = -1;
            else
                insert.records[j].timestamp = atoll(tokens[i].value);
        } else if (tokens[i].type == TOKEN_LITERAL) {
            insert.records[j++].value = strtod(tokens[i].value, &endptr);
        }
        // TODO error when j > 32
    }

    insert.record_len = j;

    return insert;
}

Statement_Select parse_select(Token *tokens, size_t token_count) {
    Statement_Select select = {.mask = 0};
    char *endptr = NULL;

    for (size_t i = 0; i < token_count; ++i) {
        switch (tokens[i].type) {
        case TOKEN_SELECT:
            snprintf(select.ts_name, sizeof(select.ts_name), "%s",
                     tokens[i].value);
            break;
        case TOKEN_FROM:
            snprintf(select.db_name, sizeof(select.db_name), "%s",
                     tokens[i].value);
            break;
        case TOKEN_AT:
            select.start_time = atoll(tokens[i].value);
            select.end_time = 0;
            select.mask |= SM_SINGLE;
            break;
        case TOKEN_RANGE:
            select.start_time = atoll(tokens[i].value);
            select.mask |= SM_RANGE;
            break;
        case TOKEN_TO:
            select.end_time = atoll(tokens[i].value);
            break;
        case TOKEN_BY:
            select.interval = atoll(tokens[i].value);
            select.mask |= SM_BY;
            break;
        case TOKEN_WHERE:
            snprintf(select.where.key, sizeof(select.where.key), "%s",
                     tokens[i].value);
            select.mask |= SM_WHERE;
            break;
        case TOKEN_OPERATOR_EQ:
            select.where.operator= OP_EQ;
            select.where.value = strtod(tokens[i].value, &endptr);
            break;
        case TOKEN_OPERATOR_NE:
            select.where.operator= OP_NE;
            select.where.value = strtod(tokens[i].value, &endptr);
            break;
        case TOKEN_OPERATOR_LE:
            select.where.operator= OP_LE;
            select.where.value = strtod(tokens[i].value, &endptr);
            break;
        case TOKEN_OPERATOR_LT:
            select.where.operator= OP_LT;
            select.where.value = strtod(tokens[i].value, &endptr);
            break;
        case TOKEN_OPERATOR_GE:
            select.where.operator= OP_GE;
            select.where.value = strtod(tokens[i].value, &endptr);
            break;
        case TOKEN_OPERATOR_GT:
            select.where.operator= OP_GT;
            select.where.value = strtod(tokens[i].value, &endptr);
            break;
        case TOKEN_AGGREGATE:
            select.mask |= SM_AGGREGATE;
            break;
        case TOKEN_AGGREGATE_FN:
            if (strncmp(tokens[i].value, "AVG", 3) == 0)
                select.af = AFN_AVG;
            else if (strncmp(tokens[i].value, "MIN", 3) == 0)
                select.af = AFN_MIN;
            else if (strncmp(tokens[i].value, "MAX", 3) == 0)
                select.af = AFN_MAX;
            break;
        default:
            break;
        }
    }

    return select;
}

Statement parse(Token *tokens, size_t token_count) {
    Statement statement = {.type = STATEMENT_UNKNOW};

    switch (tokens[0].type) {
    case TOKEN_CREATE:
        statement.type = STATEMENT_CREATE;
        statement.create = parse_create(tokens, token_count);
        break;
    case TOKEN_INSERT:
        statement.type = STATEMENT_INSERT;
        statement.insert = parse_insert(tokens, token_count);
        break;
    case TOKEN_SELECT:
        statement.type = STATEMENT_SELECT;
        statement.select = parse_select(tokens, token_count);
        break;
    default:
        break;
    }

    return statement;
}

void print_create(Statement_Create *create) {
    if (create->mask == 0) {
        printf("CREATE\n\t%s\n", create->db_name);
    } else {
        printf("CREATE\n\t%s\n", create->ts_name);
        printf("INTO\n\t%s\n", create->db_name);
    }
}

void print_insert(Statement_Insert *insert) {
    printf("INSERT\n\t%s\nINTO\n\t%s\n", insert->ts_name, insert->db_name);
    printf("VALUES\n\t");
    for (size_t i = 0; i < insert->record_len; ++i) {
        printf("(%lu, %.2f) ", insert->records[i].timestamp,
               insert->records[i].value);
    }
    printf("\n");
}

void print_select(Statement_Select *select) {
    printf("SELECT\n\t%s\nFROM\n\t%s\n", select->ts_name, select->db_name);
    if (select->mask & SM_SINGLE)
        printf("AT\n\t%li\n", select->start_time);
    else if (select->mask & SM_RANGE)
        printf("RANGE\n\t%li TO %li\n", select->start_time, select->end_time);
    if (select->mask & SM_WHERE)
        printf("WHERE\n\t%s %i %.2lf\n", select->where.key,
               select->where.operator, select->where.value);
    if (select->mask & SM_AGGREGATE)
        printf("AGGREAGATE\n\t%i\n", select->af);
    if (select->mask & SM_BY)
        printf("BY\n\t%lu\n", select->interval);
}
