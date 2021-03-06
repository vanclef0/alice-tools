/* Copyright (C) 2019 Nunuhara Cabbage <nunuhara@haniwa.technology>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <limits.h>
#include <libgen.h>
#include "system4.h"
#include "system4/afa.h"
#include "system4/ald.h"
#include "archive.h"

// dirname is allowed to return a pointer to static memory OR modify its input.
// This works around the braindamage by ALWAYS returning a pointer to static
// memory, at the cost of a string copy.
char *xdirname(const char *path)
{
	static char buf[PATH_MAX];
	strncpy(buf, path, PATH_MAX-1);
	return dirname(buf);
}

char *xbasename(const char *path)
{
	static char buf[PATH_MAX];
	strncpy(buf, path, PATH_MAX-1);
	return basename(buf);
}

static struct archive *open_ald_archive(const char *path, int *error)
{
	int count = 0;
	char *dir_name = xdirname(path);
	char *base_name = xbasename(path);
	char *ald_filenames[ALD_FILEMAX];
	int prefix_len = strlen(base_name) - 5;
	if (prefix_len <= 0)
		return NULL;

	memset(ald_filenames, 0, sizeof(char*) * ALD_FILEMAX);

	DIR *dir;
	struct dirent *d;
	char filepath[PATH_MAX];

	if (!(dir = opendir(dir_name))) {
		ERROR("Failed to open directory: %s", path);
	}

	while ((d = readdir(dir))) {
		printf("checking %s\n", d->d_name);
		int len = strlen(d->d_name);
		if (len < prefix_len + 5 || strcasecmp(d->d_name+len-4, ".ald"))
			continue;
		if (strncasecmp(d->d_name, base_name, prefix_len))
			continue;

		int dno = toupper(*(d->d_name+len-5)) - 'A';
		if (dno < 0 || dno >= ALD_FILEMAX)
			continue;

		snprintf(filepath, PATH_MAX-1, "%s/%s", dir_name, d->d_name);
		ald_filenames[dno] = strdup(filepath);
		count = max(count, dno+1);
	}

	struct archive *ar = ald_open(ald_filenames, count, ARCHIVE_MMAP, error);

	for (int i = 0; i < ALD_FILEMAX; i++) {
		free(ald_filenames[i]);
	}

	return ar;
}

struct afa3_archive *afa3_open(const char *file, int flags, int *error);
struct archive *flat_open_file(const char *path, possibly_unused int flags, int *error);
struct archive *flat_open(uint8_t *data, size_t size, int *error);

struct archive *open_archive(const char *path, enum archive_type *type, int *error)
{
	size_t len = strlen(path);
	if (len < 4)
		goto err;

	const char *ext = path + len - 4;
	if (!strcasecmp(ext, ".ald")) {
		*type = AR_ALD;
		return open_ald_archive(path, error);
	} else if (!strcasecmp(ext, ".afa")) {
		*type = AR_AFA;
		struct archive *ar = (struct archive*)afa_open(path, ARCHIVE_MMAP, error);
		if (!ar && *error == ARCHIVE_BAD_ARCHIVE_ERROR) {
			*type = AR_AFA3;
			ar = (struct archive*)afa3_open(path, ARCHIVE_MMAP, error);
		}
		return ar;
	} else if (!strcasecmp(ext, "flat")) {
		*type = AR_FLAT;
		return flat_open_file(path, 0, error);
	}
	// TODO: try to use file magic

err:
	ERROR("Couldn't determine archive type for '%s'", path);
}
