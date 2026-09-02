# POSIX Shell Implementation
**Roll Number:** 2026201018  

## File Structure & Modules
- `include/prompt.h` / `src/prompt.cpp`: Shell prompt construction and relative home paths.
- `include/builtin_cd.h` / `src/builtin_cd.cpp`: Custom `cd` implementation (`-`, `~`, `..`).
- `include/builtin_echo.h` / `src/builtin_echo.cpp`: Spaces/tabs formatting echo implementation.
- `include/builtin_pwd.h` / `src/builtin_pwd.cpp`: Print working directory.
- `include/builtin_ls.h` / `src/builtin_ls.cpp`: Custom `ls` command (`-a`, `-l`).
- `include/executor.h` / `src/executor.cpp`: Spawning foreground and background system calls using `fork` and `execvp`.
- `include/builtin_pinfo.h` / `src/builtin_pinfo.cpp`: Process info inspection using `/proc`.
- `include/builtin_search.h` / `src/builtin_search.cpp`: Recursive filesystem lookup.
- `include/redirection.h` / `src/redirection.cpp`: Handles `<`, `>`, `>>` file descriptors setup.
- `include/pipeline.h` / `src/pipeline.cpp`: Multi-piped command setup.
- `include/signals.h` / `src/signals.cpp`: Signal processing (CTRL-C, CTRL-Z, child reaping).
- `include/autocomplete.h` / `src/autocomplete.cpp`: Raw terminal execution for TAB completion.
- `include/history.h` / `src/history.cpp`: Persistent history and arrow key navigation.

## Compilation
Run `make` to compile the binary executable. Execute `./shell` to initiate.