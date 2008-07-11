/* keymap.h - 13.6.2008 - 7.7.2008 Ari & Tero Roponen */
#ifndef KEYMAP_H
#define KEYMAP_H
#include <glib.h>

struct event
{
	unsigned int keycode;
};

typedef GHashTable fb_keymap_t;
#define create_keymap() g_hash_table_new (NULL, NULL)

void *lookup_key (int character);
void set_key (int character, void *command);
void use_keymap (fb_keymap_t *keymap); /* NULL for default keymap. */

#define ALT	(1<<30)
#define SHIFT	(1<<29)
#define CONTROL	(1<<28)

#endif
