/* keymap.c - 12.6.2008 - 14.6.2008 Ari & Tero Roponen */
#include <glib.h>
#include "keymap.h"

static GHashTable *global_map;

void *lookup_key (int character)
{
	if (! global_map)
		return NULL;
	return g_hash_table_lookup (global_map, GINT_TO_POINTER (character));
}

void set_key (int character, void *command)
{
	if (! global_map)
		global_map = g_hash_table_new (NULL, NULL);
	g_hash_table_replace (global_map, GINT_TO_POINTER (character), command);
}
