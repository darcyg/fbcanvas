/* extcmd.c - 7.7.2008 - 14.7.2008 Ari & Tero Roponen */
#include <sys/types.h>
#include <attr/xattr.h>
#include <glib.h>
#include <stdarg.h>
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

static void reset_transformations (struct document *doc)
{
	doc->scale = 1.0;
	doc->xoffset = 0;
	doc->yoffset = 0;
	cairo_matrix_init_identity (&doc->transform);

	/* Update is needed to reset to the original image. */
	if (doc->flags & NO_GENERIC_SCALE)
		doc->update(doc);
}

static void ecmd_version (struct document *doc, int argc, char *argv[])
{
	extern const char *argp_program_version;
	doc->set_message (doc, (char *)argp_program_version);
}

static void ecmd_goto (struct document *doc, int argc, char *argv[])
{
	if (argc != 2)
	{
		doc->set_message (doc, "Usage: goto [pagenum|tag]");
		return;
	}

	int page;
	char *end;
	page = strtol (argv[1], &end, 10);
	if (*end == '\0')	/* Page number */
	{
		if ((page > 0) && (page <= doc->pagecount))
		{
			doc->pagenum = page - 1;
			doc->update (doc);
		} else {
			doc->set_message (doc, "Invalid pagenum: %s", argv[1]);
		}
	} else {
		char tag[128];
		char buf[128];
		int err;

		sprintf(tag, "user.fb.tags.%s", argv[1]);
		err = getxattr(doc->filename, tag, buf, sizeof(buf));
		if (err > 0)
		{
			doc->pagenum = atoi(buf) - 1;
			doc->update (doc);
		} else {
			doc->set_message (doc, "No such tag: '%s'", argv[1]);
		}
	}
}

static void list_tags (struct document *doc)
{
	int len = listxattr(doc->filename, NULL, 0);
	if (len < 0)
	{
		doc->set_message (doc, "No tags found");
	} else {
		doc->set_message (doc, "Page tags: %d-%d\nUser tags:",
				  1, doc->pagecount);

		char buf[len], *s;
		listxattr(doc->filename, buf, len);
		for (s = buf; s - buf < len; s += strlen (s) + 1)
			if (strncmp (s, "user.fb.tags.", 13) == 0)
				doc->set_message (doc,"%s\n", s + 13);
	}
}

static void set_tag (struct document *doc, char *tag)
{
	char *end;
	strtol (tag, &end, 10);
	if (*end == '\0')
	{
		doc->set_message (doc, "Numeric tags are reserved");
	} else {
		char tagname[128];
		char buf[128];
		int err;

		sprintf(tagname, "user.fb.tags.%s", tag);
		sprintf(buf, "%d", doc->pagenum + 1);

		err = setxattr(doc->filename, tagname, buf, strlen(buf)+1, 0);
		if (err == 0)
		{
			doc->set_message (doc, "Tagged page %d as %s",
				doc->pagenum + 1, tag);
		} else {
			doc->set_message (doc, "Failed to set tag '%s'", tag);
		}
	}
}

static void ecmd_tag (struct document *doc, int argc, char *argv[])
{
	if (argc == 1)
		list_tags (doc);
	else if (argc == 2)
		set_tag (doc, argv[1]);
	else
		doc->set_message (doc, "Usage: tag [name]");
}

static void ecmd_help (struct document *doc, int argc, char *argv[])
{
	doc->set_message (doc, "Commands:");
	GList *names = g_hash_table_get_keys (commands);
	for (GList *it = names; it; it = it->next)
	{
		doc->set_message (doc, "\n%s", it->data);
		if (it->next == names)
			break;
	}
	g_list_free (names);
}

static void scale_doc_full (struct document *doc, double xs, double ys)
{
	double w = doc->backend->width;
	double h = doc->backend->height;
	cairo_matrix_t scale;

	cairo_matrix_init (&scale, w / xs, 0.0, 0.0, h / ys, 0.0, 0.0);
	cairo_matrix_multiply (&doc->transform, &doc->transform, &scale);
}

static void ecmd_scale (struct document *doc, int argc, char *argv[])
{
	if (argc != 2)
	{
		doc->set_message (doc, "Usage: scale <num|more|less|full|width|height|reset>");
		return;
	}

	char *tail;
	double scale = strtod (argv[1], &tail);

	if ((*tail == '\0') && scale > 0.05)
	{
		goto do_scale;
	} else if (! strcmp (argv[1], "more")) {
		scale = doc->scale + 0.1;
		goto do_scale;
	} else if (! strcmp (argv[1], "less")) {
		scale = doc->scale;
		if (scale >= 0.2)
			scale -= 0.1;
		goto do_scale;
	} else if (! strcmp (argv[1], "full")) {
		reset_transformations (doc);
		scale_doc_full (doc, doc->width, doc->height);
	} else if (! strcmp (argv[1], "width")) {
		reset_transformations (doc);
		if (doc->flags & NO_GENERIC_SCALE)
		{
			doc->scale = (double)doc->backend->width / (double)doc->width;
			doc->update(doc);
		} else {
			scale_doc_full (doc, doc->width, doc->width);
		}
	} else if (! strcmp (argv[1], "height")) {
		reset_transformations (doc);
		if (doc->flags & NO_GENERIC_SCALE)
		{
			doc->scale = (double)doc->backend->height / (double)doc->height;
			doc->update(doc);
		} else {
			scale_doc_full (doc, doc->height, doc->height);
		}
	} else if (! strcmp (argv[1], "reset")) {
		reset_transformations (doc);
	} else {
		doc->set_message (doc, "Invalid scale: %s\n", argv[1]);
	}

	return;

do_scale:
	if (scale != doc->scale)
	{
		doc->scale = scale;
		if (doc->flags & NO_GENERIC_SCALE)
			doc->update(doc);
	}
}

void register_extended_commands (void)
{
	set_extcmd ("goto", ecmd_goto);
	set_extcmd ("scale", ecmd_scale);
	set_extcmd ("tag", ecmd_tag);
	set_extcmd ("version", ecmd_version);
	set_extcmd ("help", ecmd_help);
}

void execute_extended_command (struct document *doc, char *fmt, ...)
{
	va_list list;
	char *buf, *arg, *argv[10]; /* XXX */
	int argc = 0;

	va_start (list, fmt);
	vasprintf (&buf, fmt, list);
	va_end (list);

	for (arg = strtok (buf, " "); arg && argc < 9; arg = strtok (NULL, " "))
		argv[argc++] = arg;
	argv[argc] = NULL;

	if (commands)
	{
		extcmd_t command = g_hash_table_lookup (commands, argv[0]);
		if (command)
			command (doc, argc, argv);
	}
	free (buf);
}
