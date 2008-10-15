/* extcmd.h - 7.7.2008 - 14.7.2008 Ari & Tero Roponen */
#ifndef EXTCMD_H
#define EXTCMD_H
#include "document.h"

typedef void (*extcmd_t) (struct document *doc, int argc, char *argv[]);

void set_extcmd (char *name, extcmd_t action);
void execute_extended_command (struct document *doc, char *fmt, ...);
void register_extended_commands (void);

extern char *prompt;
extern char *ecmd_prefix;

#endif
