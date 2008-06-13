/* file_info.h - 13.6.2008 - 13.6.2008 Ari & Tero Roponen */
#ifndef FILE_INFO_H
#define FILE_INFO_H

#include "fbcanvas.h"

struct file_info
{
	char *type;
	struct file_ops *ops;
};

struct file_info *get_file_info (char *filename);

#endif
