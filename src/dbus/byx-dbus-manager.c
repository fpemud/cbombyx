/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#include "byx-common.h"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include "util/util-gobject-singleton.h"
#include "byx-dbus-manager.h"

/* The base path for our GDBusObjectManagerServers.  They do not contain
 * "Bombyx" because GDBusObjectManagerServer requires that all
 * exported objects be *below* the base path, and eg the Manager object
 * is the base path already.
 */
#define OBJECT_MANAGER_SERVER_BASE_PATH "/org/fpemud"

/*****************************************************************************/

struct _ByxDBusManagerClass {
	GObjectClass parent;
};

enum {
	PRIVATE_CONNECTION_NEW,
	PRIVATE_CONNECTION_DISCONNECTED,
	LAST_SIGNAL
};
static guint signals[LAST_SIGNAL];

typedef struct {
	int dummy;
} ByxDBusManagerPrivate;

struct _ByxDBusManager {
	GObject parent;
	ByxDBusManagerPrivate _priv;
};

G_DEFINE_TYPE_WITH_PRIVATE(ByxDBusManager, byx_dbus_manager, G_TYPE_OBJECT)

BYX_DEFINE_SINGLETON_GETTER (ByxDBusManager, byx_dbus_manager_get, BYX_TYPE_DBUS_MANAGER)

/*****************************************************************************/

static void byx_dbus_manager_dispose (GObject *gobject);

static void byx_dbus_manager_class_init (ByxDBusManagerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = byx_dbus_manager_dispose;

	signals[PRIVATE_CONNECTION_NEW] =
	    g_signal_new (BYX_DBUS_MANAGER_PRIVATE_CONNECTION_NEW,
	                  G_OBJECT_CLASS_TYPE (object_class),
	                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
	                  0, NULL, NULL, NULL,
	                  G_TYPE_NONE, 2, G_TYPE_DBUS_CONNECTION, G_TYPE_DBUS_OBJECT_MANAGER_SERVER);

	signals[PRIVATE_CONNECTION_DISCONNECTED] =
	    g_signal_new (BYX_DBUS_MANAGER_PRIVATE_CONNECTION_DISCONNECTED,
	                  G_OBJECT_CLASS_TYPE (object_class),
	                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
	                  0, NULL, NULL, NULL,
	                  G_TYPE_NONE, 1, G_TYPE_POINTER);
}

static void byx_dbus_manager_init (ByxDBusManager *self)
{
/*	ByxDBusManagerPrivate *priv = byx_dbus_manager_get_instance_private (self); */
}

static void byx_dbus_manager_dispose (GObject *gobject)
{
/*    ByxDBusManagerPrivate *priv = byx_dbus_manager_get_instance_private ((ByxDBusManager *) gobject); */

	G_OBJECT_CLASS (byx_dbus_manager_parent_class)->dispose (gobject);
}

/*****************************************************************************/

gboolean byx_dbus_manager_start (ByxDBusManager *self, GError **error)
{
	return FALSE;
}

void byx_dbus_manager_stop (ByxDBusManager *self)
{
}
