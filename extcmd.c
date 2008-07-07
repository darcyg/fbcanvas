/* extcmd.c - 7.7.2008 - 7.7.2008 Ari & Tero Roponen */
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "document.h"
#include "extcmd.h"

static GHashTable *commands;

void set_extcmd (char *name, extcmd_t action)
{
	if (! commands)
		commands = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_replace (commands, name, action);
}

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

#if 0
static void ecmd_help (struct document *doc, int argc, char *argv[])
{
	char buf[7 * 10]; /* about 10 commands à 7 chars or something... */
	int pos = 0;

	for (int i = 0; commands[i].name; i++)
		pos += sprintf (buf + pos, "%s\n", commands[i].name);
	doc->set_message (doc, "Commands:\n%s", buf);
}
#endif

void register_extended_commands (void)
{
	set_extcmd ("echo", ecmd_echo);
	set_extcmd ("goto", ecmd_goto);
	set_extcmd ("version", ecmd_version);
//	set_extcmd ("help", ecmd_help);
}

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

	if (! commands)
		return;
	extcmd_t command = g_hash_table_lookup (commands, argv[0]);
	if (command)
		command (doc, argc, argv);
}
