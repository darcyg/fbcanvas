/* extcmd.c - 7.7.2008 - 7.7.2008 Ari & Tero Roponen */
#include <stdio.h>
#include <string.h>
#include "document.h"
#include "extcmd.h"

static void ecmd_version (struct document *doc, int argc, char *argv[])
{
	extern const char *argp_program_version;
	doc->set_message (doc, (char *)argp_program_version);
}

static void ecmd_echo (struct document *doc, int argc, char *argv[])
{
	if (argc == 2)
		doc->set_message (doc, argv[1]);
	else
		doc->set_message (doc, "Usage: echo text");
}

static void ecmd_goto (struct document *doc, int argc, char *argv[])
{
	if (argc != 2)
	{
		doc->set_message (doc, "Usage: goto pagenum");
		return;
	}

	int page = atoi (argv[1]);
	if ((page > 0) && (page <= doc->pagecount))
	{
		doc->pagenum = page - 1;
		doc->update (doc);
	} else {
		doc->set_message (doc, "Invalid pagenum: %s", argv[1]);
	}
}


struct extcommand commands[] = {
	{"echo", ecmd_echo},
	{"goto", ecmd_goto},
	{"version", ecmd_version},
	{NULL, NULL}
};

void execute_extended_command (struct document *doc, char *cmd)
{
	char *argv[10];		/* XXX */
	int argc = 0;
	char *arg = cmd;

	do
	{
		argv[argc++] = arg;
		arg = strchr (arg, ' ');
		if (arg)
		{
			*arg = '\0';
			arg++;
		} else break;
	} while (argc < 9);
	argv[argc] = NULL;
	
	for (int i = 0; commands[i].name; i++)
	{
		if (strcmp (cmd, commands[i].name) == 0)
		{
			commands[i].action (doc, argc, argv);
			return;
		}
	}
}
