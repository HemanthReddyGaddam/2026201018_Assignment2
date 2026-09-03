// small helper to clean up quoted arguments

#include "parser.h"
#include <cstring>

// turns ".txt" into .txt so grep and other cmds work properly
void stripquotes(char* token) {
    int len = strlen(token);
    if (len < 2) {
        return;
    }

    char first = token[0];
    char last = token[len - 1];
    // only strip if both ends have matching quotes
    if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
        memmove(token, token + 1, len - 2);
        token[len - 2] = '\0';
    }
}
