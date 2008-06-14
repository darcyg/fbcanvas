/* commands.h - 13.6.2008 - 13.6.2008 Ari & Tero Roponen */
#ifndef COMMANDS_H
#define COMMANDS_H

#include <setjmp.h>
#include "document.h"

typedef void (*command_t) (struct document *, int command, int last);

extern jmp_buf exit_loop;

void setup_keys (void);
command_t lookup_command (int character);

#endif
