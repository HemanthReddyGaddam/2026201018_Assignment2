#include "builtin_echo.h"
#include <iostream>

void execute_echo(char** args, int arg_count) {
    for (int i = 1; i < arg_count; ++i) {
        std::cout << args[i];
        if (i < arg_count - 1) {
            std::cout << " ";
        }
    }
    std::cout << "\n";
}
