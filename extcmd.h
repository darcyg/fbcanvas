/* extcmd.h - 7.7.2008 - 7.7.2008 Ari & Tero Roponen */
#ifndef EXTCMD_H
#define EXTCMD_H
#include "document.h"

typedef void (*extcmd_t) (struct document *doc, int argc, char *argv[]);

void set_extcmd (char *name, extcmd_t action);

#endif
