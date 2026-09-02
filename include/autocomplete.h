#ifndef AUTOCOMPLETE_H
#define AUTOCOMPLETE_H

// Read a line from stdin with TAB autocomplete for commands and local files.
// Returns true on success, false on EOF (Ctrl-D on empty line).
bool read_line_with_autocomplete(char* buffer, int max_len);

#endif
