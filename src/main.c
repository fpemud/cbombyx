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
#include <locale.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <c-list.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/resource.h>

#include "main-utils.h"
#include "config/byx-config.h"

#define BYX_DEFAULT_PID_FILE          NMRUNDIR "/bombyx.pid"
#define BYX_DEFAULT_SYSTEM_STATE_FILE NMSTATEDIR "/bombyx.state"

#define CONFIG_ATOMIC_SECTION_PREFIXES ((char **) NULL)

static GMainLoop *main_loop = NULL;

static struct {
	gboolean show_version;
	gboolean print_config;
	gboolean become_daemon;
	gboolean g_fatal_warnings;
	gboolean run_from_build_dir;
	char *opt_log_level;
	char *opt_log_domains;
	char *pidfile;
} global_opt = {
	.become_daemon = TRUE,
};

static void
_set_g_fatal_warnings (void)
{
	GLogLevelFlags fatal_mask;

	fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
	fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
	g_log_set_always_fatal (fatal_mask);
}

static void
_init_nm_debug (ByxConfigManager *config)
{
	gs_free char *debug = NULL;
	enum {
		D_RLIMIT_CORE =    (1 << 0),
		D_FATAL_WARNINGS = (1 << 1),
	};
	GDebugKey keys[] = {
		{ "RLIMIT_CORE", D_RLIMIT_CORE },
		{ "fatal-warnings", D_FATAL_WARNINGS },
	};
	guint flags;
	const char *env = getenv ("NM_DEBUG");

	debug = byx_config_data_get_value (byx_config_manager_get_data_orig (config),
	                                  BYX_CONFIG_KEYFILE_GROUP_MAIN,
	                                  BYX_CONFIG_KEYFILE_KEY_MAIN_DEBUG,
	                                  BYX_MANAGER_RELOAD_FLAGS_NONE);

	flags  = byx_utils_parse_debug_string (env, keys, G_N_ELEMENTS (keys));
	flags |= byx_utils_parse_debug_string (debug, keys, G_N_ELEMENTS (keys));

#if ! defined (__SANITIZE_ADDRESS__)
	if (NM_FLAGS_HAS (flags, D_RLIMIT_CORE)) {
		/* only enable this, if explicitly requested, because it might
		 * expose sensitive data. */

		struct rlimit limit = {
			.rlim_cur = RLIM_INFINITY,
			.rlim_max = RLIM_INFINITY,
		};
		setrlimit (RLIMIT_CORE, &limit);
	}
#endif

	if (NM_FLAGS_HAS (flags, D_FATAL_WARNINGS))
		_set_g_fatal_warnings ();
}

void
nm_main_config_reload (int signal)
{
	ByxConfigChangeFlags reload_flags;

	switch (signal) {
	case SIGHUP:
		reload_flags = BYX_CONFIG_CHANGE_CAUSE_SIGHUP;
		break;
	case SIGUSR1:
		reload_flags = BYX_CONFIG_CHANGE_CAUSE_SIGUSR1;
		break;
	case SIGUSR2:
		reload_flags = BYX_CONFIG_CHANGE_CAUSE_SIGUSR2;
		break;
	default:
		g_return_if_reached ();
	}

	byx_log_info (LOGD_CORE, "reload configuration (signal %s)...", strsignal (signal));

	/* The signal handler thread is only installed after
	 * creating ByxConfigManager instance, and on shut down we
	 * no longer run the mainloop (to reach this point).
	 *
	 * Hence, a ByxConfigManager singleton instance must always be
	 * available. */
	byx_config_reload (byx_config_manager_get (), reload_flags);
}

static int
print_config (ByxConfigCmdLineOptions *config_cli)
{
	gs_unref_object ByxConfigManager *config = NULL;
	gs_free_error GError *error = NULL;
	ByxConfigData *config_data;

	byx_logging_setup ("OFF", "ALL", NULL, NULL);

	config = byx_config_new (config_cli, CONFIG_ATOMIC_SECTION_PREFIXES, &error);
	if (config == NULL) {
		fprintf (stderr, _("Failed to read configuration: %s\n"), error->message);
		return 7;
	}

	config_data = byx_config_manager_get_data (config);
	fprintf (stdout, "# NetworkManager configuration: %s\n", byx_config_data_get_config_description (config_data));
	byx_config_data_log (config_data, "", "", stdout);
	return 0;
}

static void
do_early_setup (int *argc, char **argv[], ByxConfigCmdLineOptions *config_cli)
{
	GOptionEntry options[] = {
		{ "version", 'V', 0, G_OPTION_ARG_NONE, &global_opt.show_version, N_("Print NetworkManager version and exit"), NULL },
		{ "no-daemon", 'n', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &global_opt.become_daemon, N_("Don't become a daemon"), NULL },
		{ "log-level", 0, 0, G_OPTION_ARG_STRING, &global_opt.opt_log_level, N_("Log level: one of [%s]"), "INFO" },
		{ "log-domains", 0, 0, G_OPTION_ARG_STRING, &global_opt.opt_log_domains, N_("Log domains separated by ',': any combination of [%s]"), "PLATFORM,RFKILL,WIFI" },
		{ "g-fatal-warnings", 0, 0, G_OPTION_ARG_NONE, &global_opt.g_fatal_warnings, N_("Make all warnings fatal"), NULL },
		{ "pid-file", 'p', 0, G_OPTION_ARG_FILENAME, &global_opt.pidfile, N_("Specify the location of a PID file"), BYX_DEFAULT_PID_FILE },
		{ "run-from-build-dir", 0, 0, G_OPTION_ARG_NONE, &global_opt.run_from_build_dir, "Run from build directory", NULL },
		{ "print-config", 0, 0, G_OPTION_ARG_NONE, &global_opt.print_config, N_("Print NetworkManager configuration and exit"), NULL },
		{NULL}
	};

	if (!nm_main_utils_early_setup ("NetworkManager",
	                                argc,
	                                argv,
	                                options,
	                                (void (*)(gpointer, GOptionContext *)) byx_config_cmd_line_options_add_to_entries,
	                                config_cli,
	                                _("NetworkManager monitors all network connections and automatically\nchooses the best connection to use.  It also allows the user to\nspecify wireless access points which wireless cards in the computer\nshould associate with.")))
		exit (1);

	global_opt.pidfile = global_opt.pidfile ?: g_strdup(BYX_DEFAULT_PID_FILE);
}

/*
 * main
 *
 */
int
main (int argc, char *argv[])
{
	gboolean success = FALSE;
	ByxManager *manager = NULL;
	ByxConfigManager *config;
	gs_free_error GError *error = NULL;
	gboolean wrote_pidfile = FALSE;
	char *bad_domains = NULL;
	ByxConfigCmdLineOptions *config_cli;
	guint sd_id = 0;
	GError *error_invalid_logging_config = NULL;

	/* Known to cause a possible deadlock upon GDBus initialization:
	 * https://bugzilla.gnome.org/show_bug.cgi?id=674885 */
	g_type_ensure (G_TYPE_SOCKET);
	g_type_ensure (G_TYPE_DBUS_CONNECTION);
	g_type_ensure (BYX_TYPE_DBUS_MANAGER);

	_byx_utils_is_manager_process = TRUE;

	main_loop = g_main_loop_new (NULL, FALSE);

	/* we determine a first-start (contrary to a restart during the same boot)
	 * based on the existence of NMRUNDIR directory. */
	config_cli = byx_config_cmd_line_options_new (!g_file_test (NMRUNDIR, G_FILE_TEST_IS_DIR));

	do_early_setup (&argc, &argv, config_cli);

	if (global_opt.g_fatal_warnings)
		_set_g_fatal_warnings ();

	if (global_opt.show_version) {
		fprintf (stdout, VERSION "\n");
		exit (0);
	}

	if (global_opt.print_config) {
		int result;

		result = print_config (config_cli);
		byx_config_cmd_line_options_free (config_cli);
		exit (result);
	}

	byx_main_utils_ensure_root ();

	byx_main_utils_ensure_not_running_pidfile (global_opt.pidfile);

	byx_main_utils_ensure_statedir ();
	byx_main_utils_ensure_rundir ();

	/* When running from the build directory, determine our build directory
	 * base and set helper paths in the build tree */
	if (global_opt.run_from_build_dir) {
		char *path, *slash;
		int g;

		/* exe is <basedir>/src/.libs/lt-NetworkManager, so chop off
		 * the last three components */
		path = realpath ("/proc/self/exe", NULL);
		g_assert (path != NULL);
		for (g = 0; g < 3; ++g) {
			slash = strrchr (path, '/');
			g_assert (slash != NULL);
			*slash = '\0';
		}

		/* don't free these strings, we need them for the entire
		 * process lifetime */
		nm_dhcp_helper_path = g_strdup_printf ("%s/src/dhcp/nm-dhcp-helper", path);

		g_free (path);
	}

	if (!byx_logging_setup (global_opt.opt_log_level,
	                        global_opt.opt_log_domains,
	                        &bad_domains,
	                        &error)) {
		fprintf (stderr,
		         _("%s.  Please use --help to see a list of valid options.\n"),
		         error->message);
		exit (1);
	}

	/* Read the config file and CLI overrides */
	config = byx_config_setup (config_cli, CONFIG_ATOMIC_SECTION_PREFIXES, &error);
	byx_config_cmd_line_options_free (config_cli);
	config_cli = NULL;
	if (config == NULL) {
		fprintf (stderr, _("Failed to read configuration: %s\n"), error->message);
		exit (1);
	}

	_init_nm_debug (config);

	/* Initialize logging from config file *only* if not explicitly
	 * specified by commandline.
	 */
	if (global_opt.opt_log_level == NULL && global_opt.opt_log_domains == NULL) {
		if (!byx_logging_setup (byx_config_manager_get_log_level (config),
		                       byx_config_manager_get_log_domains (config),
		                       &bad_domains,
		                       &error_invalid_logging_config)) {
			/* ignore error, and print the failure reason below.
			 * Likewise, print about bad_domains below. */
		}
	}

	if (global_opt.become_daemon && !byx_config_manager_get_is_debug (config)) {
		if (daemon (0, 0) < 0) {
			int saved_errno;

			saved_errno = errno;
			fprintf (stderr, _("Could not daemonize: %s [error %u]\n"),
			         g_strerror (saved_errno),
			         saved_errno);
			exit (1);
		}
		wrote_pidfile = nm_main_utils_write_pidfile (global_opt.pidfile);
	}

	/* Set up unix signal handling - before creating threads, but after daemonizing! */
	nm_main_utils_setup_signals (main_loop);

	{
		gs_free char *v = NULL;

		v = byx_config_data_get_value (BYX_CONFIG_GET_DATA_ORIG,
		                              BYX_CONFIG_KEYFILE_GROUP_LOGGING,
		                              BYX_CONFIG_KEYFILE_KEY_LOGGING_BACKEND,
		                              BYX_CONFIG_GET_VALUE_STRIP | BYX_CONFIG_GET_VALUE_NO_EMPTY);
		byx_logging_syslog_openlog (v, byx_config_manager_get_is_debug (config));
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
		             (global_opt.opt_log_level == NULL && global_opt.opt_log_domains == NULL)
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

	manager = byx_manager_setup ();
	byx_dbus_manager_start (byx_dbus_manager_get(),
	                       byx_manager_dbus_set_property_handle,
	                       manager);

	if (!byx_manager_start (manager, &error)) {
		byx_log_err (LOGD_CORE, "failed to initialize: %s", error->message);
		goto done;
	}

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
	byx_manager_stop (manager);

	byx_config_state_set (config, TRUE, TRUE);

	nm_dns_manager_stop (nm_dns_manager_get ());

done_no_manager:
	if (global_opt.pidfile && wrote_pidfile)
		unlink (global_opt.pidfile);

	byx_log_info (LOGD_CORE, "exiting (%s)", success ? "success" : "error");

	nm_clear_g_source (&sd_id);

	exit (success ? 0 : 1);
}
