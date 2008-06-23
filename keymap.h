/* keymap.h - 13.6.2008 - 13.6.2008 Ari & Tero Roponen */
#ifndef KEYMAP_H
#define KEYMAP_H

void *lookup_key (int character);
void set_key (int character, void *command);

#define ALT	(1<<30);
#define SHIFT	(1<<29)
#define CONTROL	(1<<28)

#endif
