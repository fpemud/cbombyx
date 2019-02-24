/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#include "byx-common.h"

#include <assert.h>
#include "byx-device.h"

struct _ByxDeviceClass {
    GObjectClass parent;

	void (*carrier_changed) (ByxDevice *device, gboolean carrier);
};

enum {
	DEVICE_CARRIER_CHANGED,
	LAST_SIGNAL
};
static guint signals[LAST_SIGNAL];

typedef struct {
    int dummy;
} ByxDevicePrivate;

struct _ByxDevice {
    GObject parent;
    ByxDevicePrivate _priv;
};

G_DEFINE_TYPE_WITH_PRIVATE (ByxDevice, byx_device, G_TYPE_OBJECT)

/*****************************************************************************/

static void byx_device_dispose (GObject *gobject);

static void byx_device_class_init (ByxDeviceClass *device_class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (device_class);

    object_class->dispose = byx_device_dispose;

	signals[DEVICE_CARRIER_CHANGED] = g_signal_new (BYX_DEVICE_CARRIER_CHANGED,
	                                                G_OBJECT_CLASS_TYPE (object_class),
	                                                G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
	                                                0, NULL, NULL, NULL,
	                                                G_TYPE_NONE, 2, BYX_TYPE_DEVICE, G_TYPE_BOOLEAN);
}

static void byx_device_init (ByxDevice *self)
{
}

static void byx_device_dispose (GObject *gobject)
{
/*    ByxDevicePrivate *priv = byx_device_get_instance_private ((ByxDevice *) gobject);*/

    G_OBJECT_CLASS (byx_device_parent_class)->dispose (gobject);
}
