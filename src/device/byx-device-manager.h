/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __BYX_DEVICE_MANAGER_H__
#define __BYX_DEVICE_MANAGER_H__

#include "byx-device.h"

#define BYX_TYPE_DEVICE_MANAGER            (byx_device_manager_get_type ())
#define BYX_DEVICE_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BYX_TYPE_DEVICE_MANAGER, ByxDeviceManager))
#define BYX_DEVICE_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BYX_TYPE_DEVICE_MANAGER, ByxDeviceManagerClass))
#define BYX_IS_DEVICE_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BYX_TYPE_DEVICE_MANAGER))
#define BYX_IS_DEVICE_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BYX_TYPE_DEVICE_MANAGER))
#define BYX_DEVICE_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BYX_TYPE_DEVICE_MANAGER, ByxDeviceManagerClass))

typedef struct _ByxDeviceManager ByxDeviceManager;
typedef struct _ByxDeviceManagerClass ByxDeviceManagerClass;

GType byx_device_manager_get_type (void);

ByxDeviceManager *byx_device_manager_get (void);

/*****************************************************************************/

#define BYX_DEVICE_MANAGER_DEVICE_APPEAR     "device-appear"
#define BYX_DEVICE_MANAGER_DEVICE_DISAPPEAR  "device-disappear"

gboolean byx_device_manager_start (ByxDeviceManager *self, GError **error);

GSList *byx_device_manager_get_devices (ByxDeviceManager *self, gboolean sort);

#define byx_device_manager_for_each_device(manager, iter, tmp_list) \
	for (tmp_list = byx_device_manager_get_devices (manager), \
	     iter = c_list_entry (tmp_list->next, ByxDevice, devices_lst); \
	     ({ \
	         const gboolean _has_next = (&iter->devices_lst != tmp_list); \
	         \
	         if (!_has_next) \
	             iter = NULL; \
	         _has_next; \
	    }); \
	    iter = c_list_entry (iter->devices_lst.next, ByxDevice, devices_lst))


#endif /* __BYX_DEVICE_MANAGER_H__ */
