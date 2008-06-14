/* commands.h - 13.6.2008 - 14.6.2008 Ari & Tero Roponen */
#ifndef COMMANDS_H
#define COMMANDS_H

#include <setjmp.h>

typedef void (*command_t) (void *data);

extern jmp_buf exit_loop;
extern int this_command;
extern int last_command;

void setup_keys (void);
command_t lookup_command (int character);

#endif
