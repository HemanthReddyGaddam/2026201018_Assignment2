// echo builtin - print arguments

#include "builtin_echo.h"
#include<iostream>

void executeecho(char** args, int argc) {
    // print each arg with a space between them
    for (int i = 1; i < argc; i++) {
        std::cout << args[i];
        if (i < argc - 1) {
            std::cout << " ";
        }
    }
    std::cout << "\n";
}
