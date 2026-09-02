#include "parser.h"
#include <cstring>

void strip_outer_quotes(char* token) {
    size_t len = strlen(token);
    if (len < 2) {
        return;
    }

    char first = token[0];
    char last = token[len - 1];
    if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
        memmove(token, token + 1, len - 2);
        token[len - 2] = '\0';
    }
}
