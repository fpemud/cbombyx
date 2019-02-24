/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#include "byx-common.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "util/util-gobject-singleton.h"
#include "byx-device-manager.h"

struct _ByxDeviceManagerClass {
    GObjectClass parent;
};

enum {
	DEVICE_APPEAR,
	DEVICE_DISAPPEAR,
	LAST_SIGNAL
};
static guint signals[LAST_SIGNAL];

typedef struct {
    int dummy;
} ByxDeviceManagerPrivate;

struct _ByxDeviceManager {
    GObject parent;
    ByxDeviceManagerPrivate _priv;
};

G_DEFINE_TYPE_WITH_PRIVATE (ByxDeviceManager, byx_device_manager, G_TYPE_OBJECT)

BYX_DEFINE_SINGLETON_GETTER (ByxDeviceManager, byx_device_manager_get, BYX_TYPE_DEVICE_MANAGER)

/*****************************************************************************/

static void byx_device_manager_dispose (GObject *gobject);

static void byx_device_manager_class_init (ByxDeviceManagerClass *device_manager_class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (device_manager_class);

    object_class->dispose = byx_device_manager_dispose;

	signals[DEVICE_APPEAR] = g_signal_new (BYX_DEVICE_MANAGER_DEVICE_APPEAR,
	                                       G_OBJECT_CLASS_TYPE (object_class),
	                                       G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
	                                       0, NULL, NULL, NULL,
	                                       G_TYPE_NONE, 1, BYX_TYPE_DEVICE);

	signals[DEVICE_DISAPPEAR] = g_signal_new (BYX_DEVICE_MANAGER_DEVICE_DISAPPEAR,
	                                          G_OBJECT_CLASS_TYPE (object_class),
	                                          G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
	                                          0, NULL, NULL, NULL,
	                                          G_TYPE_NONE, 1, BYX_TYPE_DEVICE);
}

static void byx_device_manager_init (ByxDeviceManager *self)
{
}

static void byx_device_manager_dispose (GObject *gobject)
{
/*    ByxDeviceManagerPrivate *priv = byx_device_manager_get_instance_private ((ByxDeviceManager *) gobject);*/

    G_OBJECT_CLASS (byx_device_manager_parent_class)->dispose (gobject);
}

/*****************************************************************************/

gboolean byx_device_manager_start (ByxDeviceManager *self, GError **error)
{




    return TRUE;
}

GSList *byx_device_manager_get_devices (ByxDeviceManager *self, gboolean sort)
{
    return NULL;
}
