/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#include "byx-default.h"

#include <string.h>
#include <stdio.h>

#include "nm-utils.h"
#include "NetworkManagerUtils.h"
#include "nm-core-internal.h"
#include "nm-keyfile-internal.h"

#define BYX_SYSTEM_CONFIG_DIR                    NMLIBDIR "/conf.d"
#define BYX_SYSTEM_CONNECTION_CONFIG_DIR         NMLIBDIR "/connection.d"
#define BYX_SYSTEM_SERVICE_CONFIG_DIR            NMLIBDIR "/service.d"

#define BYX_CONFIG_MAIN_FILE                     NMCONFDIR "/bombyx.conf"
#define BYX_CONFIG_DIR                           NMCONFDIR "/conf.d"
#define BYX_CONNECTION_CONFIG_DIR                NMCONFDIR "/connection.d"
#define BYX_SERVICE_CONFIG_DIR                   NMCONFDIR "/service.d"

#define BYX_PID_FILE                             NMRUNDIR "/bombyx.pid"

#define BYX_RUN_DATA_FILE                        NMRUNDIR "/bombyx-intern.conf"
#define BYX_CONNECTION_RUN_DATA_DIR              NMRUNDIR "/connection.d"
#define BYX_SERVICE_RUN_DATA_DIR                 NMRUNDIR "/service.d"

#define BYX_PERSIST_DATA_FILE                    NMSTATEDIR "/bombyx-intern.conf"
#define BYX_CONNECTION_PERSIST_DATA_DIR          NMSTATEDIR "/connection.d"
#define BYX_SERVICE_PERSIST_DATA_DIR             NMSTATEDIR "/service.d"

#define KEYFILE_LIST_SEPARATOR ','

struct _ByxConfigManagerClass {
    GObjectClass parent;
};

typedef struct {
    ByxCmdLineOptions cli;

	char *system_config_dir;
	char *system_connection_config_dir;
	char *system_service_config_dir;

	char *config_main_file;
	char *config_dir;
	char *connection_config_dir;
	char *service_config_dir;

	char *run_data_file;
	char *connection_run_data_dir;
	char *service_run_data_dir;

	char *persist_data_file;
	char *connection_persist_data_dir;
	char *service_persist_data_dir;

    char *log_level;
    char *log_domains;

    char **atomic_section_prefixes;

    /* The state. This is actually a mutable data member and it makes sense:
     * The regular config is immutable (ByxConfigData) and can old be swapped
     * as a whole (via byx_config_set_values() or during reload). Thus, it can
     * be changed, but it is still immutable and is swapped atomically as a
     * whole. Also, we emit a config-changed signal on that occasion.
     *
     * For state, there are no events. You can query it and set it.
     * It only gets read *once* at startup, and later is cached and only
     * written out to disk. Hence, no need for the immutable dance here
     * because the state changes only on explicit actions from the daemon
     * itself. */
    State *state;

    ByxConfig *config;

    ByxConfigData *global_run_data;
    ByxConfigData *global_persist_data;

    GHashTable *connection_run_data;
    GHashTable *connection_persist_data;

    /*
     * It is true, if NM is started the first time -- contrary to a restart
     * during the same boot up. That is determined by the content of the
     * /var/run/NetworManager state directory. */
    bool first_start;
} ByxConfigManagerPrivate;

struct _ByxConfigManager {
    GObject parent;
    ByxConfigManagerPrivate _priv;
};

G_DEFINE_TYPE_WITH_PRIVATE (ByxConfigManager, byx_config_manager, G_TYPE_OBJECT)

#define BYX_CONFIG_GET_PRIVATE(self) _BYX_GET_PRIVATE (self, ByxConfigManager, BYX_IS_CONFIG_MANAGER)

BYX_DEFINE_SINGLETON_GETTER (ByxConfigManager, byx_config_manager_get, BYX_TYPE_CONFIG_MANAGER);

/*****************************************************************************/

ByxConfigManager *byx_config_manager_setup (int argc, char *argv[], GError **error)
{
    ByxConfigManagerPrivate *priv;
    GError *local;

    assert (singleton_instance == NULL);

    singleton_instance = g_object_new();
    if (singleton_instance == NULL) {
        return NULL;
    }

    priv = byx_config_manager_get_instance_private ((ByxConfigManager *) singleton_instance);
    priv->first_start = !g_file_test (NMRUNDIR, G_FILE_TEST_IS_DIR),
    byx_cmd_line_options_parse(priv->cmd_line_options, argc, argv);

/* FIXME */
#if 0
    global_opt.pidfile = global_opt.pidfile ?: g_strdup(BYX_PID_FILE);
#endif

    byx_singleton_instance_register ();
    
    /* usually, you would not see this logging line because when creating the
        * ByxConfigManager instance, the logging is not yet set up to print debug message. */
    byx_log_dbg (LOGD_CORE, "setup %s singleton (%p)", "ByxConfigManager", singleton_instance);

    return singleton_instance;
}

/*****************************************************************************/

static void byx_config_manager_classinit (ByxConfigManagerClass *config_manager_class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (config_manager_class);

    object_class->byx_config_manager_finalize = byx_config_manager_finalize;

    G_STATIC_ASSERT_EXPR (sizeof (guint) == sizeof (ByxConfigChangeFlags));
    G_STATIC_ASSERT_EXPR (((gint64) ((ByxConfigChangeFlags) -1)) > ((gint64) 0));
}

static void byx_config_manager_init (ByxConfigManager *self)
{
    self->system_config_dir = BYX_SYSTEM_CONFIG_DIR;
	self->system_connection_config_dir = BYX_SYSTEM_CONNECTION_CONFIG_DIR;
	self->system_service_config_dir = BYX_SYSTEM_SERVICE_CONFIG_DIR;

	self->config_main_file = BYX_CONFIG_MAIN_FILE;
	self->config_dir = BYX_CONFIG_DIR;
	self->connection_config_dir = BYX_CONNECTION_CONFIG_DIR;
	self->service_config_dir = BYX_SERVICE_CONFIG_DIR;

	self->run_data_file = BYX_RUN_DATA_FILE;
	self->connection_run_data_dir = BYX_CONNECTION_RUN_DATA_DIR;
	self->service_run_data_dir = BYX_SERVICE_RUN_DATA_DIR;

	self->persist_data_file = BYX_PERSIST_DATA_FILE;
	self->connection_persist_data_dir = BYX_CONNECTION_PERSIST_DATA_DIR;
	self->service_persist_data_dir = BYX_SERVICE_PERSIST_DATA_DIR;




}

static void byx_config_manager_finalize (GObject *gobject)
{
    ByxConfigManagerPrivate *priv = byx_config_manager_get_instance_private ((ByxConfigManager *) gobject);

    g_free (priv->log_level);
    g_free (priv->log_domains);
    g_strfreev (priv->atomic_section_prefixes);

    _byx_cmd_line_options_clear (&priv->cli);

    g_clear_object (&priv->config_data);
    g_clear_object (&priv->config_data_orig);

    G_OBJECT_CLASS (byx_config_parent_class)->finalize (gobject);
}

/*****************************************************************************/

const ByxConfig *byx_config_manager_get_config(ByxConfigManager *self)
{
    ByxConfigManagerPrivate *priv = byx_config_manager_get_instance_private(self);
    return priv->config;
}

/*****************************************************************************/

ByxConfigData *byx_config_manager_get_run_data (ByxConfigManager *self)
{
    ByxConfigManagerPrivate *priv = byx_config_manager_get_instance_private(self);
    return priv->global_run_data;
}

ByxConfigData *byx_config_manager_get_persist_data (ByxConfigManager *self)
{
    ByxConfigManagerPrivate *priv = byx_config_manager_get_instance_private(self);
    return priv->global_persist_data;
}

/*****************************************************************************/

static ByxConfigData *_byx_config_manager_get_connection_data (ByxConfigManager *self, const char *uuid, gboolean run_data_or_persist_data)
{
    ByxConfigManagerPrivate *priv = byx_config_manager_get_instance_private(self);
    GHashTable *htable;
    char *datadir;
    char *dataname;
    ByxConfigData *data;
    char *new_uuid;
    GError *local;

    assert (uuid != NULL);

    if (run_data_or_persist_data) {
        htable = priv->connection_run_data;
        datadir = BYX_CONNECTION_RUN_DATA_DIR;
        dataname = "run data";
    } else {
        htable = priv->connection_persist_data;
        datadir = BYX_CONNECTION_PERSIST_DATA_DIR;
        dataname = "persist data";
    }

    data = g_hash_table_lookup(htable, uuid);
    if (data != NULL) {
        return data;
    }

    data = g_malloc(sizeof(*data));
    if (data == NULL) {
        return NULL;
    }

    byx_sprintf_buf (data->filename, "%s/%s", datadir, uuid);

    data->keyfile = g_key_file_new ();                            /* FIXME: return NULL? */
    g_key_file_set_list_separator (data->keyfile, KEYFILE_LIST_SEPARATOR);

    if (access (path, F_OK) == 0) {
        local = NULL;
        if (!g_key_file_load_from_file (keyfile, data->filename, G_KEY_FILE_KEEP_COMMENTS, local)) {
            _LOGT ("failed reading connection %s \"%s\"", dataname, data->filename);
            _byx_config_data_free(data);
            g_propagate_error (error, local);
            return NULL;
        }
    }

    new_uuid = strdup(uuid);
    if (new_uuid == NULL) {
        _LOGT ("failed to allocate memory");
        _byx_config_data_free(data);
        return NULL;
    }

    g_hash_table_insert (htable, new_uuid, data);

    return data;
}

ByxConfigData *byx_config_manager_get_connection_run_data (ByxConfigManager *self, const char *connection_uuid)
{
    return _byx_config_manager_get_connection_data(self, connection_uuid, TRUE);
}

ByxConfigData *byx_config_manager_get_connection_persist_data (ByxConfigManager *self, const char *connection_uuid)
{
    return _byx_config_manager_get_connection_data(self, connection_uuid, FALSE);
}

/*****************************************************************************/

void byx_config_manager_cleanup_persist_data(ByxConfigManager *self)
{
    /* iterate all the file in BYX_CONNECTION_RUN_DATA_DIR and BYX_CONNECTION_PERSIST_DATA_DIR */
    /* to see delete data that has no corresponding connection */
}

/*****************************************************************************/

#define _NMLOG_DOMAIN      LOGD_CORE
#define _NMLOG(level, ...) __NMLOG_DEFAULT (level, _NMLOG_DOMAIN, "config", __VA_ARGS__)

/*****************************************************************************/

#define _HAS_PREFIX(str, prefix) \
    ({ \
        const char *_str = (str); \
        g_str_has_prefix ( _str, ""prefix"") && _str[BYX_STRLEN(prefix)] != '\0'; \
    })

/*****************************************************************************/

gint
byx_config_parse_boolean (const char *str,
                         gint default_value)
{
    return _byx_utils_ascii_str_to_bool (str, default_value);
}

gint
byx_config_keyfile_get_boolean (const GKeyFile *keyfile,
                               const char *section,
                               const char *key,
                               gint default_value)
{
    g_autofree char *str = NULL;

    g_return_val_if_fail (keyfile != NULL, default_value);
    g_return_val_if_fail (section != NULL, default_value);
    g_return_val_if_fail (key != NULL, default_value);

    str = g_key_file_get_value ((GKeyFile *) keyfile, section, key, NULL);
    return byx_config_parse_boolean (str, default_value);
}

gint64
byx_config_keyfile_get_int64 (const GKeyFile *keyfile,
                             const char *section,
                             const char *key,
                             guint base,
                             gint64 min,
                             gint64 max,
                             gint64 fallback)
{
    gint64 v;
    int errsv;
    char *str;

    g_return_val_if_fail (keyfile, fallback);
    g_return_val_if_fail (section, fallback);
    g_return_val_if_fail (key, fallback);

    str = g_key_file_get_value ((GKeyFile *) keyfile, section, key, NULL);
    v = _byx_utils_ascii_str_to_int64 (str, base, min, max, fallback);
    if (str) {
        errsv = errno;
        g_free (str);
        errno = errsv;
    }
    return v;
}

char *
byx_config_keyfile_get_value (const GKeyFile *keyfile,
                             const char *section,
                             const char *key,
                             ByxConfigGetValueFlags flags)
{
    char *value;

    if (BYX_FLAGS_HAS (flags, BYX_CONFIG_GET_VALUE_RAW))
        value = g_key_file_get_value ((GKeyFile *) keyfile, section, key, NULL);
    else
        value = g_key_file_get_string ((GKeyFile *) keyfile, section, key, NULL);

    if (!value)
        return NULL;

    if (BYX_FLAGS_HAS (flags, BYX_CONFIG_GET_VALUE_STRIP))
        g_strstrip (value);

    if (   BYX_FLAGS_HAS (flags, BYX_CONFIG_GET_VALUE_NO_EMPTY)
        && !*value) {
        g_free (value);
        return NULL;
    }

    return value;
}

void
byx_config_keyfile_set_string_list (GKeyFile *keyfile,
                                   const char *group,
                                   const char *key,
                                   const char *const* strv,
                                   gssize len)
{
    gsize l;
    char *new_value;

    if (len < 0)
        len = strv ? g_strv_length ((char **) strv) : 0;

    g_key_file_set_string_list (keyfile, group, key, strv, len);

    /* g_key_file_set_string_list() appends a trailing separator to the value.
     * We don't like that, get rid of it. */

    new_value = g_key_file_get_value (keyfile, group, key, NULL);
    if (!new_value)
        return;

    l = strlen (new_value);
    if (l > 0 && new_value[l - 1] == KEYFILE_LIST_SEPARATOR) {
        /* Maybe we should check that value doesn't end with "\\,", i.e.
         * with an escaped separator. But the way g_key_file_set_string_list()
         * is implemented (currently), it always adds a trailing separator. */
        new_value[l - 1] = '\0';
        g_key_file_set_value (keyfile, group, key, new_value);
    }
    g_free (new_value);
}

/*****************************************************************************/

ByxConfigData *
byx_config_manager_get_data (ByxConfigManager *config)
{
    g_return_val_if_fail (config != NULL, NULL);

    return BYX_CONFIG_GET_PRIVATE (config)->config_data;
}

/*****************************************************************************/

static void
_byx_cmd_line_options_copy (const ByxCmdLineOptions *cli, ByxCmdLineOptions *dst)
{
    g_return_if_fail (cli);
    g_return_if_fail (dst);
    g_return_if_fail (cli != dst);

    _byx_cmd_line_options_clear (dst);
    dst->config_dir = g_strdup (cli->config_dir);
    dst->system_config_dir = g_strdup (cli->system_config_dir);
    dst->config_main_file = g_strdup (cli->config_main_file);
    dst->run_data_file = g_strdup (cli->run_data_file);
    dst->persist_data_file = g_strdup (cli->persist_data_file);
    dst->is_debug = cli->is_debug;
    dst->connectivity_uri = g_strdup (cli->connectivity_uri);
    dst->connectivity_response = g_strdup (cli->connectivity_response);
    dst->connectivity_interval = cli->connectivity_interval;
    dst->first_start = cli->first_start;
}



/*****************************************************************************/

GKeyFile *
byx_config_create_keyfile ()
{
    GKeyFile *keyfile;

    keyfile = g_key_file_new ();
    g_key_file_set_list_separator (keyfile, KEYFILE_LIST_SEPARATOR);
    return keyfile;
}

/* this is an external variable, to make loading testable. Other then that,
 * no code is supposed to change this. */
guint _byx_config_match_nm_version = BYX_VERSION;
char *_byx_config_match_env = NULL;

static gboolean
ignore_config_snippet (GKeyFile *keyfile, gboolean is_base_config)
{
    GSList *specs;
    gboolean as_bool;
    NMMatchSpecMatchType match_type;

    if (is_base_config)
        return FALSE;

    if (!g_key_file_has_key (keyfile, BYX_CONFIG_KEYFILE_GROUP_CONFIG, BYX_CONFIG_KEYFILE_KEY_CONFIG_ENABLE, NULL))
        return FALSE;

    /* first, let's try to parse the value as plain boolean. If that is possible, we don't treat
     * the value as match-spec. */
    as_bool = byx_config_keyfile_get_boolean (keyfile, BYX_CONFIG_KEYFILE_GROUP_CONFIG, BYX_CONFIG_KEYFILE_KEY_CONFIG_ENABLE, -1);
    if (as_bool != -1)
        return !as_bool;

    if (G_UNLIKELY (!_byx_config_match_env)) {
        const char *e;

        e = g_getenv ("BYX_CONFIG_ENABLE_TAG");
        _byx_config_match_env = g_strdup (e ?: "");
    }

    /* second, interpret the value as match-spec. */
    specs = byx_config_manager_get_match_spec (keyfile, BYX_CONFIG_KEYFILE_GROUP_CONFIG, BYX_CONFIG_KEYFILE_KEY_CONFIG_ENABLE, NULL);
    match_type = nm_match_spec_config (specs,
                                       _byx_config_match_nm_version,
                                       _byx_config_match_env);
    g_slist_free_full (specs, g_free);

    return match_type != NM_MATCH_SPEC_MATCH;
}

static int
_sort_groups_cmp (const char **pa, const char **pb, gpointer dummy)
{
    const char *a, *b;
    gboolean a_is_connection, b_is_connection;
    gboolean a_is_device, b_is_device;

    a = *pa;
    b = *pb;

    a_is_connection = g_str_has_prefix (a, BYX_CONFIG_KEYFILE_GROUPPREFIX_CONNECTION);
    b_is_connection = g_str_has_prefix (b, BYX_CONFIG_KEYFILE_GROUPPREFIX_CONNECTION);

    if (a_is_connection != b_is_connection) {
        /* one is a [connection*] entry, the other not. We sort [connection*] entries
         * after.  */
        if (a_is_connection)
            return 1;
        return -1;
    }
    if (a_is_connection) {
        /* both are [connection.\+] entries. Reverse their order.
         * One of the sections might be literally [connection]. That section
         * is special and its order will be fixed later. It doesn't actually
         * matter here how it compares with [connection.\+] sections. */
        return pa > pb ? -1 : 1;
    }

    a_is_device = g_str_has_prefix (a, BYX_CONFIG_KEYFILE_GROUPPREFIX_DEVICE);
    b_is_device = g_str_has_prefix (b, BYX_CONFIG_KEYFILE_GROUPPREFIX_DEVICE);

    if (a_is_device != b_is_device) {
        /* one is a [device*] entry, the other not. We sort [device*] entries
         * after.  */
        if (a_is_device)
            return 1;
        return -1;
    }
    if (a_is_device) {
        /* both are [device.\+] entries. Reverse their order.
         * One of the sections might be literally [device]. That section
         * is special and its order will be fixed later. It doesn't actually
         * matter here how it compares with [device.\+] sections. */
        return pa > pb ? -1 : 1;
    }

    /* don't reorder the rest. */
    return 0;
}

void
_byx_config_sort_groups (char **groups, gsize ngroups)
{
    if (ngroups > 1) {
        g_qsort_with_data (groups,
                           ngroups,
                           sizeof (char *),
                           (GCompareDataFunc) _sort_groups_cmp,
                           NULL);
    }
}

static gboolean
_setting_is_device_spec (const char *group, const char *key)
{
#define _IS(group_v, key_v) (strcmp (group, (""group_v)) == 0 && strcmp (key, (""key_v)) == 0)
    return    _IS (BYX_CONFIG_KEYFILE_GROUP_MAIN, "ignore-carrier")
           || _IS (BYX_CONFIG_KEYFILE_GROUP_MAIN, "assume-ipv6ll-only")
           || _IS (BYX_CONFIG_KEYFILE_GROUP_KEYFILE, "unmanaged-devices")
           || (g_str_has_prefix (group, BYX_CONFIG_KEYFILE_GROUPPREFIX_CONNECTION) && !strcmp (key, "match-device"))
           || (g_str_has_prefix (group, BYX_CONFIG_KEYFILE_GROUPPREFIX_DEVICE    ) && !strcmp (key, "match-device"));
}

static gboolean
_setting_is_string_list (const char *group, const char *key)
{
    return    _IS (BYX_CONFIG_KEYFILE_GROUP_MAIN, BYX_CONFIG_KEYFILE_KEY_MAIN_DEBUG)
           || _IS (BYX_CONFIG_KEYFILE_GROUP_LOGGING, "domains")
           || g_str_has_prefix (group, BYX_CONFIG_KEYFILE_GROUPPREFIX_TEST_APPEND_STRINGLIST);
#undef _IS
}

static gboolean
read_config (GKeyFile *keyfile, gboolean is_base_config, const char *dirname, const char *path, GError **error)
{
    GKeyFile *kf;
    char **groups, **keys;
    gsize ngroups, nkeys;
    int g, k;
    g_autofree char *path_free = NULL;

    g_return_val_if_fail (keyfile, FALSE);
    g_return_val_if_fail (path, FALSE);
    g_return_val_if_fail (!error || !*error, FALSE);

    if (dirname) {
        path_free = g_build_filename (dirname, path, NULL);
        path = path_free;
    }

    if (g_file_test (path, G_FILE_TEST_EXISTS) == FALSE) {
        g_set_error (error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_NOT_FOUND, "file %s not found", path);
        return FALSE;
    }

    _LOGD ("Reading config file '%s'", path);

    kf = byx_config_create_keyfile ();
    if (!g_key_file_load_from_file (kf, path, G_KEY_FILE_NONE, error)) {
        g_prefix_error (error, "%s: ", path);
        g_key_file_free (kf);
        return FALSE;
    }

    if (ignore_config_snippet (kf, is_base_config)) {
        g_key_file_free (kf);
        return TRUE;
    }

    /* the config-group is internal to every configuration snippets. It doesn't make sense
     * to merge it into the global configuration, and it doesn't make sense to preserve the
     * group beyond this point. */
    g_key_file_remove_group (kf, BYX_CONFIG_KEYFILE_GROUP_CONFIG, NULL);

    /* Override the current settings with the new ones */
    groups = g_key_file_get_groups (kf, &ngroups);
    if (!groups)
        ngroups = 0;

    /* Within one file we reverse the order of the '[connection.\+] sections.
     * Here we merge the current file (@kf) into @keyfile. As we merge multiple
     * files, earlier sections (with lower priority) will be added first.
     * But within one file, we want a top-to-bottom order. This means we
     * must reverse the order within each file.
     * At the very end, we will revert the order of all sections again and
     * get thus the right behavior. This final reversing is done in
     * ByxConfigData:_get_connection_infos().  */
    _byx_config_sort_groups (groups, ngroups);

    for (g = 0; groups && groups[g]; g++) {
        const char *group = groups[g];

        if (g_str_has_prefix (group, BYX_CONFIG_KEYFILE_GROUPPREFIX_INTERN)) {
            /* internal groups cannot be set by user configuration. */
            continue;
        }
        keys = g_key_file_get_keys (kf, group, &nkeys, NULL);
        if (!keys)
            continue;
        for (k = 0; keys[k]; k++) {
            const char *key;
            char *new_value;
            char last_char;
            gsize key_len;

            key = keys[k];
            g_assert (key && *key);

            if (   _HAS_PREFIX (key, BYX_CONFIG_KEYFILE_KEYPREFIX_WAS)
                || _HAS_PREFIX (key, BYX_CONFIG_KEYFILE_KEYPREFIX_SET)) {
                /* these keys are protected. We ignore them if the user sets them. */
                continue;
            }

            if (!strcmp (key, BYX_CONFIG_KEYFILE_KEY_ATOMIC_SECTION_WAS)) {
                /* the "was" key is protected and it cannot be set by user configuration. */
                continue;
            }

            key_len = strlen (key);
            last_char = key[key_len - 1];
            if (   key_len > 1
                && (last_char == '+' || last_char == '-')) {
                g_autofree char *base_key = g_strndup (key, key_len - 1);
                gboolean is_string_list;

                is_string_list = _setting_is_string_list (group, base_key);

                if (   is_string_list
                    || _setting_is_device_spec (group, base_key)) {
                    gs_unref_ptrarray GPtrArray *new = g_ptr_array_new_with_free_func (g_free);
                    char **iter_val;
                    gs_strfreev  char **old_val = NULL;
                    g_autofree char **new_val = NULL;

                    if (is_string_list) {
                        old_val = g_key_file_get_string_list (keyfile, group, base_key, NULL, NULL);
                        new_val = g_key_file_get_string_list (kf, group, key, NULL, NULL);
                    } else {
                        g_autofree char *old_sval = byx_config_keyfile_get_value (keyfile, group, base_key, BYX_CONFIG_GET_VALUE_TYPE_SPEC);
                        g_autofree char *new_sval = byx_config_keyfile_get_value (kf, group, key, BYX_CONFIG_GET_VALUE_TYPE_SPEC);
                        g_autofree_slist GSList *old_specs = nm_match_spec_split (old_sval);
                        g_autofree_slist GSList *new_specs = nm_match_spec_split (new_sval);

                        /* the key is a device spec. This is a special kind of string-list, that
                         * we must split differently. */
                        old_val = _byx_utils_slist_to_strv (old_specs, FALSE);
                        new_val = _byx_utils_slist_to_strv (new_specs, FALSE);
                    }

                    /* merge the string lists, by omiting duplicates. */

                    for (iter_val = old_val; iter_val && *iter_val; iter_val++) {
                        if (   last_char != '-'
                            || byx_utils_strv_find_first (new_val, -1, *iter_val) < 0)
                            g_ptr_array_add (new, g_strdup (*iter_val));
                    }
                    for (iter_val = new_val; iter_val && *iter_val; iter_val++) {
                        /* don't add duplicates. That means an "option=a,b"; "option+=a,c" results in "option=a,b,c" */
                        if (   last_char == '+'
                            && byx_utils_strv_find_first (old_val, -1, *iter_val) < 0)
                            g_ptr_array_add (new, *iter_val);
                        else
                            g_free (*iter_val);
                    }

                    if (new->len > 0) {
                        if (is_string_list)
                            byx_config_keyfile_set_string_list (keyfile, group, base_key, (const char *const*) new->pdata, new->len);
                        else {
                            g_autofree_slist GSList *specs = NULL;
                            g_autofree char *specs_joined = NULL;

                            g_ptr_array_add (new, NULL);
                            specs = _byx_utils_strv_to_slist ((char **) new->pdata, FALSE);

                            specs_joined = nm_match_spec_join (specs);

                            g_key_file_set_value (keyfile, group, base_key, specs_joined);
                        }
                    } else {
                        if (is_string_list)
                            g_key_file_remove_key (keyfile, group, base_key, NULL);
                        else
                            g_key_file_set_value (keyfile, group, base_key, "");
                    }
                } else {
                    /* For any other settings we don't support extending the option with +/-.
                     * Just drop the key. */
                }
                continue;
            }

            new_value = g_key_file_get_value (kf, group, key, NULL);
            g_key_file_set_value (keyfile, group, key, new_value);
            g_free (new_value);
        }
        g_strfreev (keys);
    }
    g_strfreev (groups);
    g_key_file_free (kf);

    return TRUE;
}

static gboolean
read_base_config (GKeyFile *keyfile,
                  const char *cli_config_main_file,
                  char **out_config_main_file,
                  GError **error)
{
    GError *my_error = NULL;

    g_return_val_if_fail (keyfile, FALSE);
    g_return_val_if_fail (out_config_main_file && !*out_config_main_file, FALSE);
    g_return_val_if_fail (!error || !*error, FALSE);

    /* Try a user-specified config file first */
    if (cli_config_main_file) {
        /* Bad user-specific config file path is a hard error */
        if (read_config (keyfile, TRUE, NULL, cli_config_main_file, error)) {
            *out_config_main_file = g_strdup (cli_config_main_file);
            return TRUE;
        } else
            return FALSE;
    }

    /* Even though we prefer NetworkManager.conf, we need to check the
     * old nm-system-settings.conf first to preserve compat with older
     * setups.  In package managed systems dropping a NetworkManager.conf
     * onto the system would make NM use it instead of nm-system-settings.conf,
     * changing behavior during an upgrade.  We don't want that.
     */

    /* Try the standard config file location next */
    if (read_config (keyfile, TRUE, NULL, BYX_CONFIG_MAIN_FILE, &my_error)) {
        *out_config_main_file = g_strdup (BYX_CONFIG_MAIN_FILE);
        return TRUE;
    }

    if (!g_error_matches (my_error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_NOT_FOUND)) {
        _LOGW ("Default config file invalid: %s\n",
               my_error->message);
        g_propagate_error (error, my_error);
        return FALSE;
    }
    g_clear_error (&my_error);

    /* If for some reason no config file exists, use the default
     * config file path.
     */
    *out_config_main_file = g_strdup (BYX_CONFIG_MAIN_FILE);
    _LOGI ("No config file found or given; using %s\n",
           BYX_CONFIG_MAIN_FILE);
    return TRUE;
}

static GPtrArray *
_get_config_dir_files (const char *config_dir)
{
    GFile *dir;
    GFileEnumerator *direnum;
    GFileInfo *info;
    GPtrArray *confs;
    const char *name;

    g_return_val_if_fail (config_dir, NULL);

    confs = g_ptr_array_new_with_free_func (g_free);
    if (!*config_dir)
        return confs;

    dir = g_file_new_for_path (config_dir);
    direnum = g_file_enumerate_children (dir, G_FILE_ATTRIBUTE_STANDARD_NAME, 0, NULL, NULL);
    if (direnum) {
        while ((info = g_file_enumerator_next_file (direnum, NULL, NULL))) {
            name = g_file_info_get_name (info);
            if (g_str_has_suffix (name, ".conf"))
                g_ptr_array_add (confs, g_strdup (name));
            g_object_unref (info);
        }
        g_object_unref (direnum);
    }
    g_object_unref (dir);

    g_ptr_array_sort (confs, nm_strcmp_p);
    return confs;
}

static void
_confs_to_description (GString *str, const GPtrArray *confs, const char *name)
{
    guint i;

    if (!confs->len)
        return;

    for (i = 0; i < confs->len; i++) {
        if (i == 0)
            g_string_append_printf (str, " (%s: ", name);
        else
            g_string_append (str, ", ");
        g_string_append (str, confs->pdata[i]);
    }
    g_string_append (str, ")");
}

static GKeyFile *
read_entire_config (const ByxCmdLineOptions *cli,
                    const char *config_dir,
                    const char *system_config_dir,
                    char **out_config_main_file,
                    char **out_config_description,
                    GError **error)
{
    gs_unref_keyfile GKeyFile *keyfile = NULL;
    gs_unref_ptrarray GPtrArray *system_confs = NULL;
    gs_unref_ptrarray GPtrArray *confs = NULL;
    gs_unref_ptrarray GPtrArray *run_confs = NULL;
    guint i;
    g_autofree char *o_config_main_file = NULL;
    const char *run_config_dir = "";

    g_return_val_if_fail (config_dir, NULL);
    g_return_val_if_fail (system_config_dir, NULL);
    g_return_val_if_fail (!out_config_main_file || !*out_config_main_file, FALSE);
    g_return_val_if_fail (!out_config_description || !*out_config_description, NULL);
    g_return_val_if_fail (!error || !*error, FALSE);

    if (   (""RUN_DATA_DIR)[0] == '/'
        && !byx_streq (RUN_DATA_DIR, system_config_dir)
        && !byx_streq (RUN_DATA_DIR, config_dir))
        run_config_dir = RUN_DATA_DIR;

    /* create a default configuration file. */
    keyfile = byx_config_create_keyfile ();

    system_confs = _get_config_dir_files (system_config_dir);
    confs = _get_config_dir_files (config_dir);
    run_confs = _get_config_dir_files (run_config_dir);

    for (i = 0; i < system_confs->len; ) {
        const char *filename = system_confs->pdata[i];

        /* if a same named file exists in config_dir or run_config_dir, skip it. */
        if (byx_utils_strv_find_first ((char **) confs->pdata, confs->len, filename) >= 0 ||
            byx_utils_strv_find_first ((char **) run_confs->pdata, run_confs->len, filename) >= 0) {
            g_ptr_array_remove_index (system_confs, i);
            continue;
        }

        if (!read_config (keyfile, FALSE, system_config_dir, filename, error))
            return NULL;
        i++;
    }

    for (i = 0; i < run_confs->len; ) {
        const char *filename = run_confs->pdata[i];

        /* if a same named file exists in config_dir, skip it. */
        if (byx_utils_strv_find_first ((char **) confs->pdata, confs->len, filename) >= 0) {
            g_ptr_array_remove_index (run_confs, i);
            continue;
        }

        if (!read_config (keyfile, FALSE, run_config_dir, filename, error))
            return NULL;
        i++;
    }

    /* First read the base config file */
    if (!read_base_config (keyfile, cli ? cli->config_main_file : NULL, &o_config_main_file, error))
        return NULL;

    g_assert (o_config_main_file);

    for (i = 0; i < confs->len; i++) {
        if (!read_config (keyfile, FALSE, config_dir, confs->pdata[i], error))
            return NULL;
    }

    if (cli && cli->connectivity_uri && cli->connectivity_uri[0])
        g_key_file_set_string (keyfile, BYX_CONFIG_KEYFILE_GROUP_CONNECTIVITY, "uri", cli->connectivity_uri);
    if (cli && cli->connectivity_interval >= 0)
        g_key_file_set_integer (keyfile, BYX_CONFIG_KEYFILE_GROUP_CONNECTIVITY, "interval", cli->connectivity_interval);
    if (cli && cli->connectivity_response && cli->connectivity_response[0])
        g_key_file_set_string (keyfile, BYX_CONFIG_KEYFILE_GROUP_CONNECTIVITY, "response", cli->connectivity_response);

    if (out_config_description) {
        GString *str;

        str = g_string_new (o_config_main_file);
        _confs_to_description (str, system_confs, "lib");
        _confs_to_description (str, run_confs, "run");
        _confs_to_description (str, confs, "etc");
        *out_config_description = g_string_free (str, FALSE);
    }
    NM_SET_OUT (out_config_main_file, g_steal_pointer (&o_config_main_file));
    return g_steal_pointer (&keyfile);
}

static gboolean
_is_atomic_section (const char *const*atomic_section_prefixes, const char *group)
{
    if (atomic_section_prefixes) {
        for (; *atomic_section_prefixes; atomic_section_prefixes++) {
            if (   **atomic_section_prefixes
                && g_str_has_prefix (group, *atomic_section_prefixes))
                return TRUE;
        }
    }
    return FALSE;
}

static void
_string_append_val (GString *str, const char *value)
{
    if (!value)
        return;
    g_string_append_c (str, '+');
    while (TRUE) {
        switch (*value) {
        case '\0':
            return;
        case '\\':
        case '+':
        case '#':
        case ':':
            g_string_append_c (str, '+');
            /* fall through */
        default:
            g_string_append_c (str, *value);
        }
        value++;
    }
}

static char *
_keyfile_serialize_section (GKeyFile *keyfile, const char *group)
{
    gs_strfreev char **keys = NULL;
    GString *str;
    guint k;

    if (keyfile)
        keys = g_key_file_get_keys (keyfile, group, NULL, NULL);
    if (!keys)
        return g_strdup ("0#");

    /* prepend a version. */
    str = g_string_new ("1#");

    for (k = 0; keys[k]; k++) {
        const char *key = keys[k];
        g_autofree char *value = NULL;

        _string_append_val (str, key);
        g_string_append_c (str, ':');

        value = g_key_file_get_value (keyfile, group, key, NULL);
        _string_append_val (str, value);
        g_string_append_c (str, '#');
    }
    return g_string_free (str, FALSE);
}

gboolean
byx_config_keyfile_has_global_dns_config (GKeyFile *keyfile, gboolean internal)
{
    gs_strfreev char **groups = NULL;
    guint g;
    const char *prefix;

    if (!keyfile)
        return FALSE;
    if (g_key_file_has_group (keyfile,
                              internal
                                  ? BYX_CONFIG_KEYFILE_GROUP_GLOBAL_DNS
                                  : BYX_CONFIG_KEYFILE_GROUP_INTERN_GLOBAL_DNS))
        return TRUE;

    groups = g_key_file_get_groups (keyfile, NULL);
    if (!groups)
        return FALSE;

    prefix = internal ? BYX_CONFIG_KEYFILE_GROUPPREFIX_INTERN_GLOBAL_DNS_DOMAIN : BYX_CONFIG_KEYFILE_GROUPPREFIX_GLOBAL_DNS_DOMAIN;

    for (g = 0; groups[g]; g++) {
        if (g_str_has_prefix (groups[g], prefix))
            return TRUE;
    }
    return FALSE;
}

/**
 * intern_config_read:
 * @filename: the filename where to store the internal config
 * @keyfile_conf: the merged configuration from user (/etc/NM/NetworkManager.conf).
 * @out_needs_rewrite: (allow-none): whether the read keyfile contains inconsistent
 *   data (compared to @keyfile_conf). If %TRUE, you might want to rewrite
 *   the file.
 *
 * Does the opposite of intern_config_write(). It reads the internal configuration.
 * Note that the actual format of how the configuration is saved in @filename
 * is different then what we return here. ByxConfigManager manages what is written internally
 * by having it inside a keyfile_intern. But we don't write that to disk as is.
 * Especially, we also store parts of @keyfile_conf as ".was" and on read we compare
 * what we have, with what ".was".
 *
 * Returns: a #GKeyFile instance with the internal configuration.
 */
static GKeyFile *
intern_config_read (const char *filename,
                    GKeyFile *keyfile_conf,
                    const char *const*atomic_section_prefixes,
                    gboolean *out_needs_rewrite)
{
    GKeyFile *keyfile_intern;
    GKeyFile *keyfile;
    gboolean needs_rewrite = FALSE;
    gs_strfreev char **groups = NULL;
    guint g, k;
    gboolean has_intern = FALSE;

    g_return_val_if_fail (filename, NULL);

    if (!*filename) {
        if (out_needs_rewrite)
            *out_needs_rewrite = FALSE;
        return NULL;
    }

    keyfile_intern = byx_config_create_keyfile ();

    keyfile = byx_config_create_keyfile ();
    if (!g_key_file_load_from_file (keyfile, filename, G_KEY_FILE_NONE, NULL)) {
        needs_rewrite = TRUE;
        goto out;
    }

    groups = g_key_file_get_groups (keyfile, NULL);
    for (g = 0; groups && groups[g]; g++) {
        gs_strfreev char **keys = NULL;
        const char *group = groups[g];
        gboolean is_intern, is_atomic;

        if (!strcmp (group, BYX_CONFIG_KEYFILE_GROUP_CONFIG))
            continue;

        keys = g_key_file_get_keys (keyfile, group, NULL, NULL);
        if (!keys)
            continue;

        is_intern = g_str_has_prefix (group, BYX_CONFIG_KEYFILE_GROUPPREFIX_INTERN);
        is_atomic = !is_intern && _is_atomic_section (atomic_section_prefixes, group);

        if (is_atomic) {
            g_autofree char *conf_section_was = NULL;
            g_autofree char *conf_section_is = NULL;

            conf_section_is = _keyfile_serialize_section (keyfile_conf, group);
            conf_section_was = g_key_file_get_string (keyfile, group, BYX_CONFIG_KEYFILE_KEY_ATOMIC_SECTION_WAS, NULL);

            if (g_strcmp0 (conf_section_was, conf_section_is) != 0) {
                /* the section no longer matches. Skip it entirely. */
                needs_rewrite = TRUE;
                continue;
            }
            /* we must set the "was" marker in our keyfile, so that we know that the section
             * from user config is overwritten. The value doesn't matter, it's just a marker
             * that this section is present. */
            g_key_file_set_value (keyfile_intern, group, BYX_CONFIG_KEYFILE_KEY_ATOMIC_SECTION_WAS, "");
        }

        for (k = 0; keys[k]; k++) {
            g_autofree char *value_set = NULL;
            const char *key = keys[k];

            value_set = g_key_file_get_value (keyfile, group, key, NULL);

            if (is_intern) {
                has_intern = TRUE;
                g_key_file_set_value (keyfile_intern, group, key, value_set);
            } else if (is_atomic) {
                if (strcmp (key, BYX_CONFIG_KEYFILE_KEY_ATOMIC_SECTION_WAS) == 0)
                    continue;
                g_key_file_set_value (keyfile_intern, group, key, value_set);
            } else if (_HAS_PREFIX (key, BYX_CONFIG_KEYFILE_KEYPREFIX_SET)) {
                const char *key_base = &key[BYX_STRLEN (BYX_CONFIG_KEYFILE_KEYPREFIX_SET)];
                g_autofree char *value_was = NULL;
                g_autofree char *value_conf = NULL;
                g_autofree char *key_was = g_strdup_printf (BYX_CONFIG_KEYFILE_KEYPREFIX_WAS"%s", key_base);

                if (keyfile_conf)
                    value_conf = g_key_file_get_value (keyfile_conf, group, key_base, NULL);
                value_was = g_key_file_get_value (keyfile, group, key_was, NULL);

                if (g_strcmp0 (value_conf, value_was) != 0) {
                    /* if value_was is no longer the same as @value_conf, it means the user
                     * changed the configuration since the last write. In this case, we
                     * drop the value. It also means our file is out-of-date, and we should
                     * rewrite it. */
                    needs_rewrite = TRUE;
                    continue;
                }
                has_intern = TRUE;
                g_key_file_set_value (keyfile_intern, group, key_base, value_set);
            } else if (_HAS_PREFIX (key, BYX_CONFIG_KEYFILE_KEYPREFIX_WAS)) {
                const char *key_base = &key[BYX_STRLEN (BYX_CONFIG_KEYFILE_KEYPREFIX_WAS)];
                g_autofree char *key_set = g_strdup_printf (BYX_CONFIG_KEYFILE_KEYPREFIX_SET"%s", key_base);
                g_autofree char *value_was = NULL;
                g_autofree char *value_conf = NULL;

                if (g_key_file_has_key (keyfile, group, key_set, NULL)) {
                    /* we have a matching "set" key too. Handle the "was" key there. */
                    continue;
                }

                if (keyfile_conf)
                    value_conf = g_key_file_get_value (keyfile_conf, group, key_base, NULL);
                value_was = g_key_file_get_value (keyfile, group, key, NULL);

                if (g_strcmp0 (value_conf, value_was) != 0) {
                    /* if value_was is no longer the same as @value_conf, it means the user
                     * changed the configuration since the last write. In this case, we
                     * don't overwrite the user-provided value. It also means our file is
                     * out-of-date, and we should rewrite it. */
                    needs_rewrite = TRUE;
                    continue;
                }
                has_intern = TRUE;
                /* signal the absence of the value. That means, we must propagate the
                 * "was" key to ByxConfigData, so that it knows to hide the corresponding
                 * user key. */
                g_key_file_set_value (keyfile_intern, group, key, "");
            } else
                needs_rewrite = TRUE;
        }
    }

out:
    /*
     * If user configuration specifies global DNS options, the DNS
     * options in internal configuration must be deleted. Otherwise a
     * deletion of options from user configuration may cause the
     * internal options to appear again.
     */
    if (byx_config_keyfile_has_global_dns_config (keyfile_conf, FALSE)) {
        if (g_key_file_remove_group (keyfile_intern, BYX_CONFIG_KEYFILE_GROUP_INTERN_GLOBAL_DNS, NULL))
            needs_rewrite = TRUE;
        for (g = 0; groups && groups[g]; g++) {
            if (   g_str_has_prefix (groups[g], BYX_CONFIG_KEYFILE_GROUPPREFIX_INTERN_GLOBAL_DNS_DOMAIN)
                && groups[g][BYX_STRLEN (BYX_CONFIG_KEYFILE_GROUPPREFIX_INTERN_GLOBAL_DNS_DOMAIN)]) {
                g_key_file_remove_group (keyfile_intern, groups[g], NULL);
                needs_rewrite = TRUE;
            }
        }
    }

    g_key_file_unref (keyfile);

    if (out_needs_rewrite)
        *out_needs_rewrite = needs_rewrite;

    _LOGD ("intern config file \"%s\"", filename);

    if (!has_intern) {
        g_key_file_unref (keyfile_intern);
        return NULL;
    }
    return keyfile_intern;
}

static int
_intern_config_write_sort_fcn (const char **a, const char **b, const char *const*atomic_section_prefixes)
{
    const char *g_a = (a ? *a : NULL);
    const char *g_b = (b ? *b : NULL);
    gboolean a_is, b_is;

    a_is = g_str_has_prefix (g_a, BYX_CONFIG_KEYFILE_GROUPPREFIX_INTERN);
    b_is = g_str_has_prefix (g_b, BYX_CONFIG_KEYFILE_GROUPPREFIX_INTERN);

    if (a_is != b_is) {
        if (a_is)
            return 1;
        return -1;
    }
    if (!a_is) {
        a_is = _is_atomic_section (atomic_section_prefixes, g_a);
        b_is = _is_atomic_section (atomic_section_prefixes, g_b);

        if (a_is != b_is) {
            if (a_is)
                return 1;
            return -1;
        }
    }
    return g_strcmp0 (g_a, g_b);
}

static gboolean
intern_config_write (const char *filename,
                     GKeyFile *keyfile_intern,
                     GKeyFile *keyfile_conf,
                     const char *const*atomic_section_prefixes,
                     GError **error)
{
    GKeyFile *keyfile;
    gs_strfreev char **groups = NULL;
    guint g, k;
    gboolean success = FALSE;
    GError *local = NULL;

    g_return_val_if_fail (filename, FALSE);

    if (!*filename) {
        g_set_error (error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_NOT_FOUND, "no filename to write (use --intern-config?)");
        return FALSE;
    }

    keyfile = byx_config_create_keyfile ();

    if (keyfile_intern) {
        groups = g_key_file_get_groups (keyfile_intern, NULL);
        if (groups && groups[0]) {
            g_qsort_with_data (groups,
                               g_strv_length (groups),
                               sizeof (char *),
                               (GCompareDataFunc) _intern_config_write_sort_fcn,
                               (gpointer) atomic_section_prefixes);
        }
    }
    for (g = 0; groups && groups[g]; g++) {
        gs_strfreev char **keys = NULL;
        const char *group = groups[g];
        gboolean is_intern, is_atomic;

        keys = g_key_file_get_keys (keyfile_intern, group, NULL, NULL);
        if (!keys)
            continue;

        is_intern = g_str_has_prefix (group, BYX_CONFIG_KEYFILE_GROUPPREFIX_INTERN);
        is_atomic = !is_intern && _is_atomic_section (atomic_section_prefixes, group);

        if (is_atomic) {
            if (   (!keys[0] || (!keys[1] && strcmp (keys[0], BYX_CONFIG_KEYFILE_KEY_ATOMIC_SECTION_WAS) == 0))
                && !g_key_file_has_group (keyfile_conf, group)) {
                /* we are about to save an atomic section. However, we don't have any additional
                 * keys on our own and there is no user-provided (overlapping) section either.
                 * We don't have to write an empty section (i.e. skip the useless ".was=0#"). */
                continue;
            } else {
                g_autofree char *conf_section_is = NULL;

                conf_section_is = _keyfile_serialize_section (keyfile_conf, group);
                g_key_file_set_string (keyfile, group, BYX_CONFIG_KEYFILE_KEY_ATOMIC_SECTION_WAS, conf_section_is);
                g_key_file_set_comment (keyfile, group, NULL,
                                        " Overwrites entire section from 'NetworkManager.conf'",
                                        NULL);
            }
        }

        for (k = 0; keys[k]; k++) {
            const char *key = keys[k];
            g_autofree char *value_set = NULL;
            g_autofree char *key_set = NULL;

            if (   !is_intern
                && strcmp (key, BYX_CONFIG_KEYFILE_KEY_ATOMIC_SECTION_WAS) == 0) {
                g_warn_if_fail (is_atomic);
                continue;
            }

            value_set = g_key_file_get_value (keyfile_intern, group, key, NULL);

            if (is_intern || is_atomic)
                g_key_file_set_value (keyfile, group, key, value_set);
            else {
                g_autofree char *value_was = NULL;

                if (_HAS_PREFIX (key, BYX_CONFIG_KEYFILE_KEYPREFIX_SET)) {
                    /* Setting a key with .set prefix has no meaning, as these keys
                     * are protected. Just set the value you want to set instead.
                     * Why did this happen?? */
                    g_warn_if_reached ();
                } else if (_HAS_PREFIX (key, BYX_CONFIG_KEYFILE_KEYPREFIX_WAS)) {
                    const char *key_base = &key[BYX_STRLEN (BYX_CONFIG_KEYFILE_KEYPREFIX_WAS)];

                    if (   _HAS_PREFIX (key_base, BYX_CONFIG_KEYFILE_KEYPREFIX_SET)
                        || _HAS_PREFIX (key_base, BYX_CONFIG_KEYFILE_KEYPREFIX_WAS)) {
                        g_warn_if_reached ();
                        continue;
                    }

                    if (g_key_file_has_key (keyfile_intern, group, key_base, NULL)) {
                        /* There is also a matching key_base entry. Skip processing
                         * the .was. key ad handle the key_base in the other else branch. */
                        continue;
                    }

                    if (keyfile_conf) {
                        value_was = g_key_file_get_value (keyfile_conf, group, key_base, NULL);
                        if (value_was)
                            g_key_file_set_value (keyfile, group, key, value_was);
                    }
                } else {
                    if (keyfile_conf) {
                        value_was = g_key_file_get_value (keyfile_conf, group, key, NULL);
                        if (g_strcmp0 (value_set, value_was) == 0) {
                            /* there is no point in storing the identical value as we have via
                             * user configuration. Skip it. */
                            continue;
                        }
                        if (value_was) {
                            g_autofree char *key_was = NULL;

                            key_was = g_strdup_printf (BYX_CONFIG_KEYFILE_KEYPREFIX_WAS"%s", key);
                            g_key_file_set_value (keyfile, group, key_was, value_was);
                        }
                    }
                    key = key_set = g_strdup_printf (BYX_CONFIG_KEYFILE_KEYPREFIX_SET"%s", key);
                    g_key_file_set_value (keyfile, group, key, value_set);
                }
            }
        }
        if (   is_intern
            && g_key_file_has_group (keyfile, group)) {
            g_key_file_set_comment (keyfile, group, NULL,
                                    " Internal section. Not overwritable via user configuration in 'NetworkManager.conf'",
                                    NULL);
        }
    }

    g_key_file_set_comment (keyfile, NULL, NULL,
                            " Internal configuration file. This file is written and read\n"
                            " by NetworkManager and its configuration values are merged\n"
                            " with the configuration from 'NetworkManager.conf'.\n"
                            "\n"
                            " Keys with a \""BYX_CONFIG_KEYFILE_KEYPREFIX_SET"\" prefix specify the value to set.\n"
                            " A corresponding key with a \""BYX_CONFIG_KEYFILE_KEYPREFIX_WAS"\" prefix records the value\n"
                            " of the user configuration at the time of storing the file.\n"
                            " The value from internal configuration is rejected if the corresponding\n"
                            " \""BYX_CONFIG_KEYFILE_KEYPREFIX_WAS"\" key no longer matches the configuration from 'NetworkManager.conf'.\n"
                            " That means, if you modify a value in 'NetworkManager.conf', the internal\n"
                            " overwrite no longer matches and is ignored.\n"
                            "\n"
                            " Certain sections can only be overwritten whole, not on a per key basis.\n"
                            " Such sections are marked with a \""BYX_CONFIG_KEYFILE_KEY_ATOMIC_SECTION_WAS"\" key that records the user configuration\n"
                            " at the time of writing.\n"
                            "\n"
                            " Internal sections of the form [" BYX_CONFIG_KEYFILE_GROUPPREFIX_INTERN "*] cannot\n"
                            " be set by user configuration.\n"
                            "\n"
                            " CHANGES TO THIS FILE WILL BE OVERWRITTEN",
                            NULL);

    success = g_key_file_save_to_file (keyfile, filename, &local);

    _LOGD ("write intern config file \"%s\"%s%s", filename, success ? "" : ": ", success ? "" : local->message);
    g_key_file_unref (keyfile);
    if (!success)
        g_propagate_error (error, local);
    return success;
}

/*****************************************************************************/

GSList *
byx_config_manager_get_match_spec (const GKeyFile *keyfile, const char *group, const char *key, gboolean *out_has_key)
{
    g_autofree char *value = NULL;

    /* nm_match_spec_split() already supports full escaping and is basically
     * a modified version of g_key_file_parse_value_as_string(). So we first read
     * the raw value (g_key_file_get_value()), and do the parsing ourselves. */
    value = g_key_file_get_value ((GKeyFile *) keyfile, group, key, NULL);
    if (out_has_key)
        *out_has_key = !!value;
    return nm_match_spec_split (value);
}

/*****************************************************************************/

gboolean
byx_config_set_global_dns (ByxConfigManager *self, NMGlobalDnsConfig *global_dns, GError **error)
{
    ByxConfigManagerPrivate *priv;
    GKeyFile *keyfile;
    char **groups;
    const NMGlobalDnsConfig *old_global_dns;
    guint i;

    g_return_val_if_fail (BYX_IS_CONFIG_MANAGER (self), FALSE);

    priv = BYX_CONFIG_GET_PRIVATE (self);
    g_return_val_if_fail (priv->config_data, FALSE);

    old_global_dns = byx_config_data_get_global_dns_config (priv->config_data);
    if (old_global_dns && !nm_global_dns_config_is_internal (old_global_dns)) {
        g_set_error_literal (error, 1, 0,
                             "Global DNS configuration already set via configuration file");
        return FALSE;
    }

    keyfile = byx_config_data_clone_keyfile_intern (priv->config_data);

    /* Remove existing groups */
    g_key_file_remove_group (keyfile, BYX_CONFIG_KEYFILE_GROUP_INTERN_GLOBAL_DNS, NULL);
    groups = g_key_file_get_groups (keyfile, NULL);
    for (i = 0; groups[i]; i++) {
        if (g_str_has_prefix (groups[i], BYX_CONFIG_KEYFILE_GROUPPREFIX_INTERN_GLOBAL_DNS_DOMAIN))
            g_key_file_remove_group (keyfile, groups[i], NULL);
    }
    g_strfreev (groups);

    /* An empty configuration removes everything from internal configuration file */
    if (nm_global_dns_config_is_empty (global_dns))
        goto done;

    /* Set new values */
    byx_config_keyfile_set_string_list (keyfile, BYX_CONFIG_KEYFILE_GROUP_INTERN_GLOBAL_DNS,
                                       "searches", nm_global_dns_config_get_searches (global_dns),
                                       -1);

    byx_config_keyfile_set_string_list (keyfile, BYX_CONFIG_KEYFILE_GROUP_INTERN_GLOBAL_DNS,
                                       "options", nm_global_dns_config_get_options (global_dns),
                                       -1);

    for (i = 0; i < nm_global_dns_config_get_num_domains (global_dns); i++) {
        NMGlobalDnsDomain *domain = nm_global_dns_config_get_domain (global_dns, i);
        g_autofree char *group_name = NULL;

        group_name = g_strdup_printf (BYX_CONFIG_KEYFILE_GROUPPREFIX_INTERN_GLOBAL_DNS_DOMAIN "%s",
                                      nm_global_dns_domain_get_name (domain));

        byx_config_keyfile_set_string_list (keyfile, group_name, "servers",
                                           nm_global_dns_domain_get_servers (domain), -1);
        byx_config_keyfile_set_string_list (keyfile, group_name, "options",
                                           nm_global_dns_domain_get_options (domain), -1);
    }

done:
    byx_config_set_values (self, keyfile, TRUE, FALSE);
    g_key_file_unref (keyfile);

    return TRUE;
}

/*****************************************************************************/

void byx_config_set_connectivity_check_enabled (ByxConfigManager *self,
                                               gboolean enabled)
{
    ByxConfigManagerPrivate *priv;
    GKeyFile *keyfile;

    g_return_if_fail (BYX_IS_CONFIG_MANAGER (self));

    priv = BYX_CONFIG_GET_PRIVATE (self);
    g_return_if_fail (priv->config_data);

    keyfile = byx_config_data_clone_keyfile_intern (priv->config_data);

    /* Remove existing groups */
    g_key_file_remove_group (keyfile, BYX_CONFIG_KEYFILE_GROUP_CONNECTIVITY, NULL);

    g_key_file_set_value (keyfile, BYX_CONFIG_KEYFILE_GROUP_CONNECTIVITY,
                          "enabled", enabled ? "true" : "false");

    byx_config_set_values (self, keyfile, TRUE, FALSE);
    g_key_file_unref (keyfile);
}

/**
 * byx_config_set_values:
 * @self: the ByxConfigManager instance
 * @keyfile_intern_new: (allow-none): the new internal settings to set.
 *   If %NULL, it is equal to an empty keyfile.
 * @allow_write: only if %TRUE, allow writing the changes to file. Otherwise,
 *   do the changes in-memory only.
 * @force_rewrite: if @allow_write is %FALSE, this has no effect. If %FALSE,
 *   only write the configuration to file, if there are any actual changes.
 *   If %TRUE, always write the configuration to file, even if tere are seemingly
 *   no changes.
 *
 *  This is the most flexible function to set values. It all depends on the
 *  keys and values you set in @keyfile_intern_new. You basically reset all
 *  internal configuration values to what is in @keyfile_intern_new.
 *
 *  There are 3 types of settings:
 *    - all groups/sections with a prefix [.intern.*] are taken as is. As these
 *      groups are separate from user configuration, there is no conflict. You set
 *      them, that's it.
 *    - there are atomic sections, i.e. sections whose name start with one of
 *      BYX_CONFIG_ATOMIC_SECTION_PREFIXES. If you put values in these sections,
 *      it means you completely replace the section from user configuration.
 *      You can also hide a user provided section by only putting the special
 *      key BYX_CONFIG_KEYFILE_KEY_ATOMIC_SECTION_WAS into that section.
 *    - otherwise you can overwrite individual values from user-configuration.
 *      Just set the value. Keys with a prefix BYX_CONFIG_KEYFILE_KEYPREFIX_*
 *      are protected -- as they are not value user keys.
 *      You can also hide a certain user setting by putting only a key
 *      BYX_CONFIG_KEYFILE_KEYPREFIX_WAS"keyname" into the keyfile.
 */
void
byx_config_set_values (ByxConfigManager *self,
                      GKeyFile *keyfile_intern_new,
                      gboolean allow_write,
                      gboolean force_rewrite)
{
    ByxConfigManagerPrivate *priv;
    GKeyFile *keyfile_intern_current;
    GKeyFile *keyfile_user;
    GKeyFile *keyfile_new;
    GError *local = NULL;
    ByxConfigData *new_data = NULL;
    gs_strfreev char **groups = NULL;
    gint g;

    g_return_if_fail (BYX_IS_CONFIG_MANAGER (self));

    priv = BYX_CONFIG_GET_PRIVATE (self);

    keyfile_intern_current = _byx_config_data_get_keyfile_intern (priv->config_data);

    keyfile_new = byx_config_create_keyfile ();
    if (keyfile_intern_new)
        _nm_keyfile_copy (keyfile_new, keyfile_intern_new);

    /* ensure that every atomic section has a .was entry. */
    groups = g_key_file_get_groups (keyfile_new, NULL);
    for (g = 0; groups && groups[g]; g++) {
        if (_is_atomic_section ((const char *const*) priv->atomic_section_prefixes, groups[g]))
            g_key_file_set_value (keyfile_new, groups[g], BYX_CONFIG_KEYFILE_KEY_ATOMIC_SECTION_WAS, "");
    }

    if (!_nm_keyfile_equals (keyfile_intern_current, keyfile_new, TRUE))
        new_data = byx_config_data_new_update_keyfile_intern (priv->config_data, keyfile_new);

    _LOGD ("set values(): %s", new_data ? "has changes" : "no changes");

    if (allow_write
        && (new_data || force_rewrite)) {
        /* We write the internal config file based on the user configuration from
         * the last load/reload. That is correct, because the intern properties might
         * be in accordance to what NM thinks is currently configured. Even if the files
         * on disk changed in the meantime.
         * But if they changed, on the next reload with might throw away our just
         * written data. That is correct, because from NM's point of view, those
         * changes on disk happened in any case *after* now. */
        if (*priv->run_data_file) {
            keyfile_user = _byx_config_data_get_keyfile_user (priv->config_data);
            if (!intern_config_write (priv->run_data_file, keyfile_new, keyfile_user,
                                      (const char *const*) priv->atomic_section_prefixes, &local)) {
                _LOGW ("error saving internal configuration \"%s\": %s", priv->run_data_file, local->message);
                g_clear_error (&local);
            }
        } else
            _LOGD ("don't persist internal configuration (no file set, use --intern-config?)");
    }
    if (new_data)
        _set_config_data (self, new_data, BYX_CONFIG_CHANGE_CAUSE_SET_VALUES);

    g_key_file_unref (keyfile_new);
}

/******************************************************************************
 * State
 ******************************************************************************/

static const char *
state_get_filename (const ByxCmdLineOptions *cli)
{
    /* For an empty filename, we assume the user wants to disable
     * state. ByxConfigManager will not try to read it nor write it out. */
    if (!cli->persist_data_file)
        return DEFAULT_STATE_FILE;
    return cli->persist_data_file[0] ? cli->persist_data_file : NULL;
}

static State *
state_new (void)
{
    State *state;

    state = g_slice_new0 (State);
    state->p.net_enabled = TRUE;
    state->p.wifi_enabled = TRUE;
    state->p.wwan_enabled = TRUE;

    return state;
}

static void
state_free (State *state)
{
    if (!state)
        return;
    g_slice_free (State, state);
}

static State *
state_new_from_file (const char *filename)
{
    GKeyFile *keyfile;
    g_autofree_error GError *error = NULL;
    State *state;

    state = state_new ();

    if (!filename)
        return state;

    keyfile = g_key_file_new ();
    g_key_file_set_list_separator (keyfile, ',');
    if (!g_key_file_load_from_file (keyfile, filename, G_KEY_FILE_NONE, &error)) {
        if (g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
            _LOGD ("state: missing state file \"%s\": %s", filename, error->message);
        else
            _LOGW ("state: error reading state file \"%s\": %s", filename, error->message);
        goto out;
    }

    _LOGD ("state: successfully read state file \"%s\"", filename);

    state->p.net_enabled  = byx_config_keyfile_get_boolean (keyfile, "main", "NetworkingEnabled", state->p.net_enabled);
    state->p.wifi_enabled = byx_config_keyfile_get_boolean (keyfile, "main", "WirelessEnabled", state->p.wifi_enabled);
    state->p.wwan_enabled = byx_config_keyfile_get_boolean (keyfile, "main", "WWANEnabled", state->p.wwan_enabled);

out:
    g_key_file_unref (keyfile);
    return state;
}

const ByxConfigState *
byx_config_state_get (ByxConfigManager *self)
{
    ByxConfigManagerPrivate *priv;

    g_return_val_if_fail (BYX_IS_CONFIG_MANAGER (self), NULL);

    priv = BYX_CONFIG_GET_PRIVATE (self);

    if (G_UNLIKELY (!priv->state)) {
        /* read the state from file lazy on first access. The reason is that
         * we want to log a failure to read the file via nm-logging.
         *
         * So we cannot read the state during construction of ByxConfigManager,
         * because at that time nm-logging is not yet configured.
         */
        priv->state = state_new_from_file (state_get_filename (&priv->cli));
    }

    return &priv->state->p;
}

static void
state_write (ByxConfigManager *self)
{
    ByxConfigManagerPrivate *priv = BYX_CONFIG_GET_PRIVATE (self);
    const char *filename;
    GString *str;
    GError *error = NULL;

    filename = state_get_filename (&priv->cli);

    if (!filename) {
        priv->state->p.dirty = FALSE;
        return;
    }

    str = g_string_sized_new (256);

    /* Let's construct the keyfile data by hand. */

    g_string_append (str, "[main]\n");
    g_string_append_printf (str, "NetworkingEnabled=%s\n", priv->state->p.net_enabled ? "true" : "false");
    g_string_append_printf (str, "WirelessEnabled=%s\n", priv->state->p.wifi_enabled ? "true" : "false");
    g_string_append_printf (str, "WWANEnabled=%s\n", priv->state->p.wwan_enabled ? "true" : "false");

    if (!g_file_set_contents (filename,
                              str->str, str->len,
                              &error)) {
        _LOGD ("state: error writing state file \"%s\": %s", filename, error->message);
        g_clear_error (&error);
        /* we leave the state dirty. That potentally means, that we try to
         * write the file over and over again, although it isn't possible. */
        priv->state->p.dirty = TRUE;
    } else
        priv->state->p.dirty = FALSE;

    _LOGT ("state: success writing state file \"%s\"", filename);

    g_string_free (str, TRUE);
}

void
_byx_config_state_set (ByxConfigManager *self,
                      gboolean allow_persist,
                      gboolean force_persist,
                      ...)
{
    ByxConfigManagerPrivate *priv;
    va_list ap;
    ByxConfigRunStatePropertyType property_type;

    g_return_if_fail (BYX_IS_CONFIG_MANAGER (self));

    priv = BYX_CONFIG_GET_PRIVATE (self);

    va_start (ap, force_persist);

    /* We expect that the ByxConfigRunStatePropertyType is an integer type <= sizeof (int).
     * Smaller would be fine, since the variadic arguments get promoted to int.
     * Larger would be a problem, also, because we want that "0" is a valid sentinel. */
    G_STATIC_ASSERT_EXPR (sizeof (ByxConfigRunStatePropertyType) <= sizeof (int));

    while ((property_type = va_arg (ap, int)) != BYX_CONFIG_STATE_PROPERTY_NONE) {
        bool *p_bool, v_bool;

        switch (property_type) {
        case BYX_CONFIG_STATE_PROPERTY_NETWORKING_ENABLED:
            p_bool = &priv->state->p.net_enabled;
            break;
        case BYX_CONFIG_STATE_PROPERTY_WIFI_ENABLED:
            p_bool = &priv->state->p.wifi_enabled;
            break;
        case BYX_CONFIG_STATE_PROPERTY_WWAN_ENABLED:
            p_bool = &priv->state->p.wwan_enabled;
            break;
        default:
            va_end (ap);
            g_return_if_reached ();
        }

        v_bool = va_arg (ap, gboolean);
        if (*p_bool == v_bool)
            continue;
        *p_bool = v_bool;
        priv->state->p.dirty = TRUE;
    }

    va_end (ap);

    if (   allow_persist
        && (force_persist || priv->state->p.dirty))
        state_write (self);
}

/*****************************************************************************/

#define DEVICE_RUN_STATE_KEYFILE_GROUP_DEVICE                   "device"
#define DEVICE_RUN_STATE_KEYFILE_KEY_DEVICE_MANAGED             "managed"
#define DEVICE_RUN_STATE_KEYFILE_KEY_DEVICE_PERM_HW_ADDR_FAKE   "perm-hw-addr-fake"
#define DEVICE_RUN_STATE_KEYFILE_KEY_DEVICE_CONNECTION_UUID     "connection-uuid"
#define DEVICE_RUN_STATE_KEYFILE_KEY_DEVICE_NM_OWNED            "nm-owned"
#define DEVICE_RUN_STATE_KEYFILE_KEY_DEVICE_ROUTE_METRIC_DEFAULT_ASPIRED   "route-metric-default-aspired"
#define DEVICE_RUN_STATE_KEYFILE_KEY_DEVICE_ROUTE_METRIC_DEFAULT_EFFECTIVE "route-metric-default-effective"

static ByxConfigDeviceStateData *
_config_device_state_data_new (int ifindex, GKeyFile *kf)
{
    ByxConfigDeviceStateData *device_state;
    g_autofree char *connection_uuid = NULL;
    g_autofree char *perm_hw_addr_fake = NULL;
    gsize connection_uuid_len;
    gsize perm_hw_addr_fake_len;
    gint nm_owned = -1;
    char *p;
    guint32 route_metric_default_effective;
    guint32 route_metric_default_aspired;

    nm_assert (kf);
    nm_assert (ifindex > 0);

    switch (byx_config_keyfile_get_boolean (kf,
                                           DEVICE_RUN_STATE_KEYFILE_GROUP_DEVICE,
                                           DEVICE_RUN_STATE_KEYFILE_KEY_DEVICE_MANAGED,
                                           -1)) {
    case TRUE:
        managed_type = BYX_CONFIG_CONNECTION_DATA_MANAGED_TYPE_MANAGED;
        connection_uuid = byx_config_keyfile_get_value (kf,
                                                       DEVICE_RUN_STATE_KEYFILE_GROUP_DEVICE,
                                                       DEVICE_RUN_STATE_KEYFILE_KEY_DEVICE_CONNECTION_UUID,
                                                       BYX_CONFIG_GET_VALUE_STRIP | BYX_CONFIG_GET_VALUE_NO_EMPTY);
        break;
    case FALSE:
        managed_type = BYX_CONFIG_CONNECTION_DATA_MANAGED_TYPE_UNMANAGED;
        break;
    case -1:
        /* missing property in keyfile. */
        break;
    }

    perm_hw_addr_fake = byx_config_keyfile_get_value (kf,
                                                     DEVICE_RUN_STATE_KEYFILE_GROUP_DEVICE,
                                                     DEVICE_RUN_STATE_KEYFILE_KEY_DEVICE_PERM_HW_ADDR_FAKE,
                                                     BYX_CONFIG_GET_VALUE_STRIP | BYX_CONFIG_GET_VALUE_NO_EMPTY);
    if (perm_hw_addr_fake) {
        char *normalized;

        normalized = byx_utils_hwaddr_canonical (perm_hw_addr_fake, -1);
        g_free (perm_hw_addr_fake);
        perm_hw_addr_fake = normalized;
    }

    nm_owned = byx_config_keyfile_get_boolean (kf,
                                              DEVICE_RUN_STATE_KEYFILE_GROUP_DEVICE,
                                              DEVICE_RUN_STATE_KEYFILE_KEY_DEVICE_NM_OWNED,
                                              -1);

    /* metric zero is not a valid metric. While zero valid for IPv4, for IPv6 it is an alias
     * for 1024. Since we handle here IPv4 and IPv6 the same, we cannot allow zero. */
    route_metric_default_effective = byx_config_keyfile_get_int64 (kf,
                                                                  DEVICE_RUN_STATE_KEYFILE_GROUP_DEVICE,
                                                                  DEVICE_RUN_STATE_KEYFILE_KEY_DEVICE_ROUTE_METRIC_DEFAULT_EFFECTIVE,
                                                                  10, 1, G_MAXUINT32, 0);
    if (route_metric_default_effective) {
        route_metric_default_aspired = byx_config_keyfile_get_int64 (kf,
                                                                    DEVICE_RUN_STATE_KEYFILE_GROUP_DEVICE,
                                                                    DEVICE_RUN_STATE_KEYFILE_KEY_DEVICE_ROUTE_METRIC_DEFAULT_EFFECTIVE,
                                                                    10, 1, route_metric_default_effective,
                                                                    route_metric_default_effective);
    } else
        route_metric_default_aspired = 0;

    connection_uuid_len = connection_uuid ? strlen (connection_uuid) + 1 : 0;
    perm_hw_addr_fake_len = perm_hw_addr_fake ? strlen (perm_hw_addr_fake) + 1 : 0;

    device_state = g_malloc (sizeof (ByxConfigDeviceStateData) +
                             connection_uuid_len +
                             perm_hw_addr_fake_len);

    device_state->ifindex = ifindex;
    device_state->connection_uuid = NULL;
    device_state->perm_hw_addr_fake = NULL;
    device_state->nm_owned = nm_owned;
    device_state->route_metric_default_aspired = route_metric_default_aspired;
    device_state->route_metric_default_effective = route_metric_default_effective;

    p = (char *) (&device_state[1]);
    if (connection_uuid) {
        memcpy (p, connection_uuid, connection_uuid_len);
        device_state->connection_uuid = p;
        p += connection_uuid_len;
    }
    if (perm_hw_addr_fake) {
        memcpy (p, perm_hw_addr_fake, perm_hw_addr_fake_len);
        device_state->perm_hw_addr_fake = p;
        p += perm_hw_addr_fake_len;
    }

    return device_state;
}

/**
 * byx_config_connection_data_get:
 * @ifindex: the ifindex for which the state is to load
 *
 * Returns: (transfer full): a run state object.
 *   Must be freed with g_free().
 */
ByxConfigDeviceStateData *
byx_config_connection_data_get (int ifindex)
{
    ByxConfigDeviceStateData *device_state;
    char path[BYX_STRLEN (BYX_CONNECTION_RUN_DATA_DIR) + 60];
    gs_unref_keyfile GKeyFile *kf = NULL;
    const char *nm_owned_str;

    g_return_val_if_fail (ifindex > 0, NULL);

    byx_sprintf_buf (path, "%s/%d", BYX_CONNECTION_RUN_DATA_DIR, ifindex);

    kf = byx_config_create_keyfile ();
    if (!g_key_file_load_from_file (kf, path, G_KEY_FILE_NONE, NULL))
        return NULL;

    device_state = _config_device_state_data_new (ifindex, kf);
    nm_owned_str = device_state->nm_owned == TRUE ?
                   ", nm-owned=1" :
                   (device_state->nm_owned == FALSE ? ", nm-owned=0" : "");

    _LOGT ("device-state: %s #%d (%s); managed=%s%s%s%s%s%s%s, route-metric-default=%"G_GUINT32_FORMAT"-%"G_GUINT32_FORMAT"",
           kf ? "read" : "miss",
           ifindex, path,
           NM_PRINT_FMT_QUOTED (device_state->connection_uuid, ", connection-uuid=", device_state->connection_uuid, "", ""),
           NM_PRINT_FMT_QUOTED (device_state->perm_hw_addr_fake, ", perm-hw-addr-fake=", device_state->perm_hw_addr_fake, "", ""),
           nm_owned_str,
           device_state->route_metric_default_aspired,
           device_state->route_metric_default_effective);

    return device_state;
}

static int
_device_state_parse_filename (const char *filename)
{
    if (!filename || !filename[0])
        return 0;
    if (!NM_STRCHAR_ALL (filename, ch, g_ascii_isdigit (ch)))
        return 0;
    return _byx_utils_ascii_str_to_int64 (filename, 10, 1, G_MAXINT, 0);
}

GHashTable *
byx_config_connection_data_get_all (void)
{
    GHashTable *states;
    GDir *dir;
    const char *fn;
    int ifindex;

    states = g_hash_table_new_full (nm_direct_hash, NULL, NULL, g_free);

    dir = g_dir_open (BYX_CONNECTION_RUN_DATA_DIR, 0, NULL);
    if (!dir)
        return states;

    while ((fn = g_dir_read_name (dir))) {
        ByxConfigDeviceStateData *state;

        ifindex = _device_state_parse_filename (fn);
        if (ifindex <= 0)
            continue;

        state = byx_config_connection_data_get (ifindex);
        if (!state)
            continue;

        if (!g_hash_table_insert (states, GINT_TO_POINTER (ifindex), state))
            nm_assert_not_reached ();
    }
    g_dir_close (dir);

    return states;
}

/*****************************************************************************/

static GHashTable *
_device_state_get_all (ByxConfigManager *self)
{
    ByxConfigManagerPrivate *priv = BYX_CONFIG_GET_PRIVATE (self);

    if (G_UNLIKELY (!priv->device_states))
        priv->device_states = byx_config_connection_data_get_all ();
    return priv->device_states;
}

/**
 * byx_config_connection_data_get_all:
 * @self: the #ByxConfigManager
 *
 * This function exists to give convenient access to all
 * device states. Do not ever try to modify the returned
 * hash, it's supposed to be immutable.
 *
 * Returns: the internal #GHashTable object with all device states.
 */
const GHashTable *
byx_config_connection_data_get_all (ByxConfigManager *self)
{
    g_return_val_if_fail (BYX_IS_CONFIG_MANAGER (self), NULL);

    return _device_state_get_all (self);
}

const ByxConfigDeviceStateData *
byx_config_connection_data_get (ByxConfigManager *self,
                            int ifindex)
{
    g_return_val_if_fail (BYX_IS_CONFIG_MANAGER (self), NULL);
    g_return_val_if_fail (ifindex > 0 , NULL);

    return g_hash_table_lookup (_device_state_get_all (self), GINT_TO_POINTER (ifindex));
}

/*****************************************************************************/

void
byx_config_reload (ByxConfigManager *self, ByxConfigChangeFlags reload_flags)
{
    ByxConfigManagerPrivate *priv;
    GError *error = NULL;
    GKeyFile *keyfile, *keyfile_intern;
    ByxConfigData *new_data = NULL;
    char *config_main_file = NULL;
    char *config_description = NULL;
    gboolean intern_config_needs_rewrite;

    g_return_if_fail (BYX_IS_CONFIG_MANAGER (self));
    g_return_if_fail (   reload_flags
                      && !BYX_FLAGS_ANY (reload_flags, ~BYX_CONFIG_CHANGE_CAUSES)
                      && !BYX_FLAGS_ANY (reload_flags,   BYX_CONFIG_CHANGE_CAUSE_NO_AUTO_DEFAULT
                                                      | BYX_CONFIG_CHANGE_CAUSE_SET_VALUES));

    priv = BYX_CONFIG_GET_PRIVATE (self);

    if (!BYX_FLAGS_ANY (reload_flags, BYX_CONFIG_CHANGE_CAUSE_SIGHUP | BYX_CONFIG_CHANGE_CAUSE_CONF)) {
        /* unless SIGHUP is specified, we don't reload the configuration from disc. */
        _set_config_data (self, NULL, reload_flags);
        return;
    }

    /* pass on the original command line options. This means, that
     * options specified at command line cannot ever be reloaded from
     * file. That seems desirable.
     */
    keyfile = read_entire_config (&priv->cli,
                                  priv->config_dir,
                                  priv->system_config_dir,
                                  &config_main_file,
                                  &config_description,
                                  &error);
    if (!keyfile) {
        _LOGE ("Failed to reload the configuration: %s", error->message);
        g_clear_error (&error);
        _set_config_data (self, NULL, reload_flags);
        return;
    }

    keyfile_intern = intern_config_read (priv->run_data_file,
                                         keyfile,
                                         (const char *const*) priv->atomic_section_prefixes,
                                         &intern_config_needs_rewrite);
    if (intern_config_needs_rewrite) {
        intern_config_write (priv->run_data_file, keyfile_intern, keyfile,
                             (const char *const*) priv->atomic_section_prefixes, NULL);
    }

    new_data = byx_config_data_new (config_main_file, config_description, keyfile, keyfile_intern);
    g_free (config_main_file);
    g_free (config_description);
    g_key_file_unref (keyfile);
    if (keyfile_intern)
        g_key_file_unref (keyfile_intern);

    _set_config_data (self, new_data, reload_flags);
}

NM_UTILS_FLAGS2STR_DEFINE (byx_config_change_flags_to_string, ByxConfigChangeFlags,

    NM_UTILS_FLAGS2STR (BYX_CONFIG_CHANGE_CAUSE_CONF, "CONF"),
    NM_UTILS_FLAGS2STR (BYX_CONFIG_CHANGE_CAUSE_DNS_RC, "DNS_RC"),
    NM_UTILS_FLAGS2STR (BYX_CONFIG_CHANGE_CAUSE_DNS_FULL, "DNS_FULL"),
    NM_UTILS_FLAGS2STR (BYX_CONFIG_CHANGE_CAUSE_SIGHUP, "SIGHUP"),
    NM_UTILS_FLAGS2STR (BYX_CONFIG_CHANGE_CAUSE_SIGUSR1, "SIGUSR1"),
    NM_UTILS_FLAGS2STR (BYX_CONFIG_CHANGE_CAUSE_SIGUSR2, "SIGUSR2"),
    NM_UTILS_FLAGS2STR (BYX_CONFIG_CHANGE_CAUSE_NO_AUTO_DEFAULT, "NO_AUTO_DEFAULT"),
    NM_UTILS_FLAGS2STR (BYX_CONFIG_CHANGE_CAUSE_SET_VALUES, "SET_VALUES"),

    NM_UTILS_FLAGS2STR (BYX_CONFIG_CHANGE_CONFIG_FILES, "config-files"),
    NM_UTILS_FLAGS2STR (BYX_CONFIG_CHANGE_VALUES, "values"),
    NM_UTILS_FLAGS2STR (BYX_CONFIG_CHANGE_VALUES_USER, "values-user"),
    NM_UTILS_FLAGS2STR (BYX_CONFIG_CHANGE_VALUES_INTERN, "values-intern"),
    NM_UTILS_FLAGS2STR (BYX_CONFIG_CHANGE_CONNECTIVITY, "connectivity"),
    NM_UTILS_FLAGS2STR (BYX_CONFIG_CHANGE_DNS_MODE, "dns-mode"),
    NM_UTILS_FLAGS2STR (BYX_CONFIG_CHANGE_RC_MANAGER, "rc-manager"),
    NM_UTILS_FLAGS2STR (BYX_CONFIG_CHANGE_GLOBAL_DNS_CONFIG, "global-dns-config"),
);

static void
_set_config_data (ByxConfigManager *self, ByxConfigData *new_data, ByxConfigChangeFlags reload_flags)
{
    ByxConfigManagerPrivate *priv = BYX_CONFIG_GET_PRIVATE (self);
    ByxConfigData *old_data = priv->config_data;
    ByxConfigChangeFlags changes, changes_diff;
    gboolean had_new_data = !!new_data;

    nm_assert (reload_flags);
    nm_assert (!BYX_FLAGS_ANY (reload_flags, ~BYX_CONFIG_CHANGE_CAUSES));
    nm_assert (   NM_IN_SET (reload_flags, BYX_CONFIG_CHANGE_CAUSE_NO_AUTO_DEFAULT, BYX_CONFIG_CHANGE_CAUSE_SET_VALUES)
               || !BYX_FLAGS_ANY (reload_flags, BYX_CONFIG_CHANGE_CAUSE_NO_AUTO_DEFAULT | BYX_CONFIG_CHANGE_CAUSE_SET_VALUES));

    changes = reload_flags;

    if (new_data) {
        changes_diff = byx_config_data_diff (old_data, new_data);
        if (changes_diff == BYX_CONFIG_CHANGE_NONE)
            g_clear_object (&new_data);
        else
            changes |= changes_diff;
    }

    if (   NM_IN_SET (reload_flags,
                      BYX_CONFIG_CHANGE_CAUSE_NO_AUTO_DEFAULT,
                      BYX_CONFIG_CHANGE_CAUSE_SET_VALUES,
                      BYX_CONFIG_CHANGE_CAUSE_CONF)
        && !new_data) {
        /* no relevant changes that should be propagated. Return silently. */
        return;
    }

    if (new_data) {
        _LOGI ("signal: %s (%s)",
               byx_config_change_flags_to_string (changes, NULL, 0),
               byx_config_data_get_config_description (new_data));
        byx_config_data_log (new_data, "CONFIG: ", "  ", NULL);
        priv->config_data = new_data;
    } else if (had_new_data)
        _LOGI ("signal: %s (no changes from disk)", byx_config_change_flags_to_string (changes, NULL, 0));
    else
        _LOGI ("signal: %s", byx_config_change_flags_to_string (changes, NULL, 0));
    if (new_data)
        g_object_unref (old_data);
}

BYX_DEFINE_SINGLETON_REGISTER (ByxConfigManager);

ByxConfigManager *byx_config_manager_get (void)
{
    assert (singleton_instance != NULL);
    return singleton_instance;
}

/*****************************************************************************/

static gboolean
init_sync (GInitable *initable, GCancellable *cancellable, GError **error)
{
    ByxConfigManager *self = BYX_CONFIG_MANAGER (initable);
    ByxConfigManagerPrivate *priv = BYX_CONFIG_GET_PRIVATE (self);
    GKeyFile *keyfile, *keyfile_intern;
    char *config_main_file = NULL;
    char *config_description = NULL;
    gboolean intern_config_needs_rewrite;
    const char *s;

    if (priv->config_dir) {
        /* Object is already initialized. */
        if (priv->config_data)
            return TRUE;
        g_set_error (error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_NOT_FOUND, "unspecified error");
        g_return_val_if_reached (FALSE);
    }

    s = priv->cli.config_dir ?: ""BYX_CONFIG_DIR;
    priv->config_dir = g_strdup (s[0] == '/' ? s : "");

    s = priv->cli.system_config_dir ?: ""BYX_SYSTEM_CONFIG_DIR;
    if (   s[0] != '/'
        || byx_streq (s, priv->config_dir))
        s = "";
    priv->system_config_dir = g_strdup (s);

    if (priv->cli.run_data_file)
        priv->run_data_file = g_strdup (priv->cli.run_data_file);
    else
        priv->run_data_file = g_strdup (PERSIST_DATA_DIR);

    keyfile = read_entire_config (&priv->cli,
                                  priv->config_dir,
                                  priv->system_config_dir,
                                  &config_main_file,
                                  &config_description,
                                  error);
    if (!keyfile)
        return FALSE;

    /* Initialize read only private members */

    priv->log_level = nm_strstrip (g_key_file_get_string (keyfile, BYX_CONFIG_KEYFILE_GROUP_LOGGING, "level", NULL));
    priv->log_domains = nm_strstrip (g_key_file_get_string (keyfile, BYX_CONFIG_KEYFILE_GROUP_LOGGING, "domains", NULL));

    keyfile_intern = intern_config_read (priv->run_data_file,
                                         keyfile,
                                         (const char *const*) priv->atomic_section_prefixes,
                                         &intern_config_needs_rewrite);
    if (intern_config_needs_rewrite) {
        intern_config_write (priv->run_data_file, keyfile_intern, keyfile,
                             (const char *const*) priv->atomic_section_prefixes, NULL);
    }

    priv->config_data_orig = byx_config_data_new (config_main_file, config_description, keyfile, keyfile_intern);

    priv->config_data = g_object_ref (priv->config_data_orig);

    g_free (config_main_file);
    g_free (config_description);
    g_key_file_unref (keyfile);
    if (keyfile_intern)
        g_key_file_unref (keyfile_intern);
    return TRUE;
}

/*****************************************************************************/

ByxConfigManager *
byx_config_new (const ByxCmdLineOptions *cli, char **atomic_section_prefixes, GError **error)
{
    return BYX_CONFIG_MANAGER (g_initable_new (BYX_TYPE_CONFIG_MANAGER,
                                      NULL,
                                      error,
                                      BYX_CONFIG_CMD_LINE_OPTIONS, cli,
                                      BYX_CONFIG_ATOMIC_SECTION_PREFIXES, atomic_section_prefixes,
                                      NULL));
}


#define RUN_DATA_DIR                         NMRUNDIR "/conf.d"
#define PERSIST_DATA_DIR                     NMSTATEDIR "/bombyx-intern.conf"
#define DEFAULT_STATE_FILE                   NMSTATEDIR "/bombyx.state"

