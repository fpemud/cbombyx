/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* NetworkManager -- Network link manager
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Copyright (C) 2004 - 2012 Red Hat, Inc.
 * Copyright (C) 2005 - 2008 Novell, Inc.
 */

#include "byx-default.h"

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <locale.h>

#include <glib/gstdio.h>
#include <glib-unix.h>

#include "main-utils.h"
#include "NetworkManagerUtils.h"
#include "nm-config.h"

static gboolean
sighup_handler (gpointer user_data)
{
    byx_log_info (LOGD_CORE, "reload configuration (signal %s)...", strsignal (signal));

    /* The signal handler thread is only installed after
     * creating ByxConfigManager instance, and on shut down we
     * no longer run the mainloop (to reach this point).
     *
     * Hence, a ByxConfigManager singleton instance must always be
     * available. */
    byx_config_manager_reload (byx_config_manager_get ());

	return G_SOURCE_CONTINUE;
}

static gboolean
sigint_handler (gpointer user_data)
{
	GMainLoop *main_loop = user_data;

	byx_log_info (LOGD_CORE, "caught SIGINT, shutting down normally.");
	g_main_loop_quit (main_loop);

	return G_SOURCE_REMOVE;
}

static gboolean
sigterm_handler (gpointer user_data)
{
	GMainLoop *main_loop = user_data;

	byx_log_info (LOGD_CORE, "caught SIGTERM, shutting down normally.");
	g_main_loop_quit (main_loop);

	return G_SOURCE_REMOVE;
}

/**
 * byx_main_utils_setup_signals:
 * @main_loop: the #GMainLoop to quit when SIGINT or SIGTERM is received
 *
 * Sets up signal handling for NetworkManager.
 */
void byx_main_utils_setup_signals (GMainLoop *main_loop)
{
	assert (main_loop != NULL);

	signal (SIGPIPE, SIG_IGN);

	g_unix_signal_add (SIGHUP, sighup_handler, GINT_TO_POINTER (SIGHUP));
	g_unix_signal_add (SIGUSR1, sighup_handler, GINT_TO_POINTER (SIGUSR1));
	g_unix_signal_add (SIGUSR2, sighup_handler, GINT_TO_POINTER (SIGUSR2));
	g_unix_signal_add (SIGINT, sigint_handler, main_loop);
	g_unix_signal_add (SIGTERM, sigterm_handler, main_loop);
}

gboolean byx_main_utils_write_pidfile (const char *pidfile, GError **error)
{
	char pid[16];
	int fd;
	gboolean success = TRUE;

	assert (pidfile != NULL);

	if ((fd = open (pidfile, O_CREAT | O_WRONLY | O_TRUNC | O_CLOEXEC, 00644)) < 0) {
		fprintf (stderr, _("Opening %s failed: %s\n"), pidfile, strerror (errno));
		return FALSE;
	}

	g_snprintf (pid, sizeof (pid), "%d", getpid ());
	if (write (fd, pid, strlen (pid)) < 0) {
		fprintf (stderr, _("Writing to %s failed: %s\n"), pidfile, strerror (errno));
		success = FALSE;
	}

	if (close (fd)) {
		fprintf (stderr, _("Closing %s failed: %s\n"), pidfile, strerror (errno));
		return FALSE;
	}

	return success;
}

void byx_main_utils_ensure_statedir ()
{
	g_autofree char *parent = NULL;
	int errsv;

	parent = g_path_get_dirname (VARDIR);

	/* Ensure parent state directories exists */
	if (   parent
	    && parent[0] == '/'
	    && parent[1] != '\0'
	    && g_mkdir_with_parents (parent, 0755) != 0) {
		errsv = errno;
		fprintf (stderr, "Cannot create parents for '%s': %s", VARDIR, g_strerror (errsv));
		exit (1);
	}
	/* Ensure state directory exists */
	if (g_mkdir_with_parents (VARDIR, 0700) != 0) {
		errsv = errno;
		fprintf (stderr, "Cannot create '%s': %s", VARDIR, g_strerror (errsv));
		exit (1);
	}
}

void byx_main_utils_ensure_rundir ()
{
	int errsv;

	/* Setup runtime directory */
	if (g_mkdir_with_parents (RUNDIR, 0755) != 0) {
		errsv = errno;
		fprintf (stderr, _("Cannot create '%s': %s"), RUNDIR, g_strerror (errsv));
		exit (1);
	}
}

/**
 * byx_main_utils_ensure_no_running_pidfile:
 * @pidfile: the pid file
 *
 * Checks whether the pidfile already exists and contains PID of a running
 * process.
 *
 * Exits with code 1 if a conflicting process is running.
 */
void byx_main_utils_ensure_no_running_pidfile (const char *pidfile)
{
	g_autofree char *contents = NULL;
	g_autofree char *proc_cmdline = NULL;
	gsize len = 0;
	glong pid;
	const char *process_name;
	const char *prgname = g_get_prgname ();

	g_return_if_fail (prgname);

	if (!pidfile || !*pidfile)
		return;

	if (!g_file_get_contents (pidfile, &contents, &len, NULL))
		return;
	if (len <= 0)
		return;

	errno = 0;
	pid = strtol (contents, NULL, 10);
	if (pid <= 0 || pid > 65536 || errno)
		return;

	g_clear_pointer (&contents, g_free);
	proc_cmdline = g_strdup_printf ("/proc/%ld/cmdline", pid);
	if (!g_file_get_contents (proc_cmdline, &contents, &len, NULL))
		return;

	process_name = strrchr (contents, '/');
	if (process_name)
		process_name++;
	else
		process_name = contents;
	if (strcmp (process_name, prgname) == 0) {
		/* Check that the process exists */
		if (kill (pid, 0) == 0) {
			fprintf (stderr, _("%s is already running (pid %ld)\n"), prgname, pid);
			exit (1);
		}
	}
}

void
byx_main_utils_ensure_root ()
{
	if (getuid () != 0) {
		fprintf (stderr, _("You must be root to run %s!\n"), g_get_prgname () ?: "");
		exit (1);
	}
}
