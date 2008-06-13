/* keymap.c - 12.6.2008 - 13.6.2008 Ari & Tero Roponen */
#include <stdlib.h>
#include "keymap.h"

struct keymap
{
	int character;
	void *command;
	struct keymap *next;
};

static struct keymap *global_map;

static struct keymap *lookup_key_internal (struct keymap *map, int character)
{
	for (;map; map = map->next)
		if (map->character == character)
			return map;
	return NULL;
}

void *lookup_key (int character)
{
	struct keymap *map = lookup_key_internal (global_map, character);
	if (map)
		return map->command;
	return NULL;
}

void set_key (int character, void *command)
{
	struct keymap *map = lookup_key_internal (global_map, character);
	if (map)
	{
		map->command = command;
		return;
	}

	map = malloc (sizeof (*map));
	if (! map)
		abort ();

	map->character = character;
	map->command = command;
	map->next = global_map;
	global_map = map;
}
