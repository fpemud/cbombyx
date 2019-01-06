/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

/* bombyx -- A network manager that is hard to configure but simple to use
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
 * Copyright (C) 2004 - 2017 Red Hat, Inc.
 * Copyright (C) 2005 - 2008 Novell, Inc.
 */

#include "byx-default.h"

#include <getopt.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <libintl.h>
#include <locale.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/resource.h>

#include "config/byx-config-manager.h"
#include "main-utils.h"

static GMainLoop *main_loop = NULL;

static void _init_nm_debug (ByxConfig *config)
{
#if ! defined (__SANITIZE_ADDRESS__)
    guint flags = byx_config_get_debug_flags(config);

    if (BYX_FLAGS_HAS (flags, BYX_CONFIG_DEBUG_FLAG_RLIMIT_CORE)) {
        /* only enable this, if explicitly requested, because it might
         * expose sensitive data. */

        struct rlimit limit = {
            .rlim_cur = RLIM_INFINITY,
            .rlim_max = RLIM_INFINITY,
        };
        setrlimit (RLIMIT_CORE, &limit);
    }
#endif
}

/*
 * main
 *
 */
int main (int argc, char *argv[])
{
	ByxConfigManager *config_manager = NULL;
	const ByxConfig *config = NULL;
    gboolean wrote_pidfile = FALSE;
    GError *local = NULL;
    gboolean success = FALSE;


    g_autoptr(GError) error = NULL;
    char *bad_domains = NULL;
    guint sd_id = 0;
    GError *error_invalid_logging_config = NULL;

    /* Known to cause a possible deadlock upon GDBus initialization:
     * https://gitlab.gnome.org/GNOME/glib/issues/541 */
    g_type_ensure (G_TYPE_SOCKET);
    g_type_ensure (G_TYPE_DBUS_CONNECTION);
    /*g_type_ensure (BYX_TYPE_DBUS_MANAGER);*/

    /* Make GIO ignore the remote VFS service; otherwise it tries to use the
     * session bus to contact the remote service, and NM shouldn't ever be
     * talking on the session bus.  See rh #588745
     */
    setenv ("GIO_USE_VFS", "local", 1);

    /*
     * Set the umask to 0022, which results in 0666 & ~0022 = 0644.
     * Otherwise, if root (or an su'ing user) has a wacky umask, we could
     * write out an unreadable resolv.conf.
     */
    umask (022);

    /* Ensure gettext() gets the right environment (bgo #666516) */
    setlocale (LC_ALL, "");
    textdomain (GETTEXT_PACKAGE);

    /* Setup config manager */
    config_manager = byx_config_manager_setup(argc, argv, &local);
	if (config_manager == NULL) {
		g_propagate_error (error, local);
		exit (1);
	}
	config = byx_config_manager_get_config(config_manager);

    if (config->show_version) {
        fprintf (stdout, VERSION "\n");
        exit (0);
    }

    byx_main_utils_ensure_root ();
    byx_main_utils_ensure_no_running_pidfile (config->pidfile);
    byx_main_utils_ensure_statedir ();
    byx_main_utils_ensure_rundir ();

    main_loop = g_main_loop_new (NULL, FALSE);

    if (!byx_logging_setup (global_opt.opt_log_level,
                            global_opt.opt_log_domains,
                            &bad_domains,
                            &error)) {
        fprintf (stderr,
                 _("%s.  Please use --help to see a list of valid options.\n"),
                 error->message);
        exit (1);
    }

    _init_nm_debug (config);

    /* Initialize logging from config file *only* if not explicitly
     * specified by commandline.
     */
    if (config->opt_log_level == NULL && config->opt_log_domains == NULL) {
        if (!byx_logging_setup (byx_config_manager_get_log_level (config),
                               byx_config_manager_get_log_domains (config),
                               &bad_domains,
                               &error_invalid_logging_config)) {
            /* ignore error, and print the failure reason below.
             * Likewise, print about bad_domains below. */
        }
    }

    if (config->become_daemon && !config->is_debug) {
        if (daemon (0, 0) < 0) {
            int saved_errno;

            saved_errno = errno;
            fprintf (stderr, _("Could not daemonize: %s [error %u]\n"),
                     g_strerror (saved_errno),
                     saved_errno);
            exit (1);
        }
        wrote_pidfile = byx_main_utils_write_pidfile (config->pidfile);
    }

    /* Set up unix signal handling - before creating threads, but after daemonizing! */
    byx_main_utils_setup_signals (main_loop);

    {
        g_autofree char *v = NULL;

        v = byx_config_data_get_value (BYX_CONFIG_GET_DATA_ORIG,
                                      BYX_CONFIG_KEYFILE_GROUP_LOGGING,
                                      BYX_CONFIG_KEYFILE_KEY_LOGGING_BACKEND,
                                      BYX_CONFIG_GET_VALUE_STRIP | BYX_CONFIG_GET_VALUE_NO_EMPTY);
        byx_logging_openlog (v, byx_config_manager_get_is_debug (config));
    }

    byx_log_info (LOGD_CORE, "NetworkManager (version " VERSION ") is starting... (%s)",
                  byx_config_manager_is_first_start (config) ? "for the first time" : "after a restart");

    byx_log_info (LOGD_CORE, "Read config: %s", byx_config_data_get_config_description (byx_config_manager_get_data (config)));
    byx_config_data_log (byx_config_manager_get_data (config), "CONFIG: ", "  ", NULL);

    if (error_invalid_logging_config) {
        byx_log_warn (LOGD_CORE, "config: invalid logging configuration: %s", error_invalid_logging_config->message);
        g_clear_error (&error_invalid_logging_config);
    }
    if (bad_domains) {
        byx_log_warn (LOGD_CORE, "config: invalid logging domains '%s' from %s",
                     bad_domains,
                     (config->opt_log_level == NULL && config->opt_log_domains == NULL)
                       ? "config file"
                       : "command line");
        nm_clear_g_free (&bad_domains);
    }

    /* the first access to State causes the file to be read (and possibly print a warning) */
    byx_config_state_get (config);

    /* Set up platform interaction layer */
    nm_linux_platform_setup ();

    NM_UTILS_KEEP_ALIVE (config, nm_netns_get (), "ByxConfigManager-depends-on-NMNetns");

    if (!byx_dbus_manager_acquire_bus (byx_dbus_manager_get ()))
        goto done_no_manager;

    byx_dbus_manager_start (byx_dbus_manager_get(),
                           byx_manager_dbus_set_property_handle,
                           manager);

    nm_platform_process_events (NM_PLATFORM_GET);

    /* Make sure the loopback interface is up. If interface is down, we bring
     * it up and kernel will assign it link-local IPv4 and IPv6 addresses. If
     * it was already up, we assume is in clean state.
     *
     * TODO: it might be desirable to check the list of addresses and compare
     * it with a list of expected addresses (one of the protocol families
     * could be disabled). The 'lo' interface is sometimes used for assigning
     * global addresses so their availability doesn't depend on the state of
     * physical interfaces.
     */
    byx_log_dbg (LOGD_CORE, "setting up local loopback");
    nm_platform_link_set_up (NM_PLATFORM_GET, 1, NULL);

    success = TRUE;

    sd_id = nm_sd_event_attach_default ();

    g_main_loop_run (main_loop);

done:
    byx_config_state_set (config, TRUE, TRUE);

    nm_dns_manager_stop (nm_dns_manager_get ());

done_no_manager:
    if (config->pidfile && wrote_pidfile)
        unlink (config->pidfile);

    byx_log_info (LOGD_CORE, "exiting (%s)", success ? "success" : "error");

    nm_clear_g_source (&sd_id);

    exit (success ? 0 : 1);
}
