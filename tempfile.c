/* tempfile.c
 * Routines to create temporary files
 *
 * $Id: tempfile.c 28065 2009-04-16 04:05:39Z gerald $
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#include "tempfile.h"
#include "mkstemp.h"
#include <wsutil/file_util.h>

static const char *
setup_tmpdir(const char *dir)
{
	size_t len = strlen(dir);
	char *newdir;

	/* Append path separator if necessary */
	if (len != 0 && dir[len - 1] == G_DIR_SEPARATOR) {
		return dir;
	}
	else {
		newdir = g_strdup_printf("%s%s", dir, G_DIR_SEPARATOR_S);
		return newdir;
	}
}

static int
try_tempfile(char *namebuf, int namebuflen, const char *dir, const char *pfx)
{
	static const char suffix[] = "XXXXXXXXXX";
	int namelen = (int) (strlen(dir) + strlen(pfx) + sizeof(suffix));
	int old_umask;
	int tmp_fd;

	g_snprintf(namebuf, namebuflen, "%s%s%s", dir, pfx, suffix);
	if (namebuflen < namelen) {
		/* Stick with the truncated name, so that if this error is
		   reported with the file name, you at least get
		   something. */
		errno = ENAMETOOLONG;
		return -1;
	}

	/* The Single UNIX Specification doesn't say that "mkstemp()"
	   creates the temporary file with mode rw-------, so we
	   won't assume that all UNIXes will do so; instead, we set
	   the umask to 0077 to take away all group and other
	   permissions, attempt to create the file, and then put
	   the umask back. */
	old_umask = umask(0077);
	tmp_fd = mkstemp(namebuf);
	umask(old_umask);
	return tmp_fd;
}

static const char *tmpdir = NULL;
#ifdef _WIN32
static const char *temp = NULL;
#endif
static const char *E_tmpdir;

#ifndef P_tmpdir
#define P_tmpdir "/var/tmp"
#endif

/* create a tempfile with the given prefix (e.g. "wireshark")
 * namebuf (and namebuflen) should be 128+1 bytes long (BTW: why?)
 * returns the file descriptor of the new tempfile and
 * the name of the new file in namebuf 
 */
int
create_tempfile(char *namebuf, int namebuflen, const char *pfx)
{
	char *dir;
	int fd;
	static gboolean initialized;

	if (!initialized) {
		if ((dir = getenv("TMPDIR")) != NULL)
			tmpdir = setup_tmpdir(dir);
#ifdef _WIN32
		if ((dir = getenv("TEMP")) != NULL)
			temp = setup_tmpdir(dir);
#endif

		E_tmpdir = setup_tmpdir(P_tmpdir);
		initialized = TRUE;
	}

	if (tmpdir != NULL) {
		fd = try_tempfile(namebuf, namebuflen, tmpdir, pfx);
		if (fd != -1)
			return fd;
	}

#ifdef _WIN32
	if (temp != NULL) {
		fd = try_tempfile(namebuf, namebuflen, temp, pfx);
		if (fd != -1)
			return fd;
	}
#endif

	fd = try_tempfile(namebuf, namebuflen, E_tmpdir, pfx);
	if (fd != -1)
		return fd;

	return try_tempfile(namebuf, namebuflen, G_DIR_SEPARATOR_S "tmp", pfx);
}
