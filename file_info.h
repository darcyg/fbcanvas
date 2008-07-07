/* file_info.h - 13.6.2008 - 7.7.2008 Ari & Tero Roponen */
#ifndef FILE_INFO_H
#define FILE_INFO_H

#include "document.h"

struct file_info
{
	char *type;
	char *extension;
	struct document_ops *ops;
	void (*setup_keys) (void);
};

struct file_info *get_file_info (char *filename);

#endif
