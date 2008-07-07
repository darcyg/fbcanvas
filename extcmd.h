/* extcmd.h - 7.7.2008 - 7.7.2008 Ari & Tero Roponen */
#ifndef EXTCMD_H
#define EXTCMD_H

struct extcommand
{
	char *name;
	void (*action) (struct document *doc, int argc, char *argv[]);
};

extern struct extcommand commands[];

#endif
