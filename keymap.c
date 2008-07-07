/* keymap.c - 12.6.2008 - 7.7.2008 Ari & Tero Roponen */
#include <glib.h>
#include "keymap.h"

static GHashTable *global_map;
static GHashTable *current_map;

void *lookup_key (int character)
{
	if (! current_map)
		return NULL;
	return g_hash_table_lookup (current_map, GINT_TO_POINTER (character));
}

void set_key (int character, void *command)
{
	if (! current_map)
		return;
	g_hash_table_replace (current_map, GINT_TO_POINTER (character), command);
}

void use_keymap (GHashTable *keymap)
{
	if (keymap)
		current_map = keymap;
	else
	{
		if (! global_map)
			global_map = g_hash_table_new (NULL, NULL);
		current_map = global_map;
	}
}
