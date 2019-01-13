/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __BYX_DEVICE_H__
#define __BYX_DEVICE_H__

#define BYX_TYPE_DEVICE            (byx_device_get_type ())
#define BYX_DEVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BYX_TYPE_DEVICE, ByxDevice))
#define BYX_DEVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  BYX_TYPE_DEVICE, ByxDeviceClass))
#define BYX_IS_DEVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BYX_TYPE_DEVICE))
#define BYX_IS_DEVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  BYX_TYPE_DEVICE))
#define BYX_DEVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  BYX_TYPE_DEVICE, ByxDeviceClass))

typedef struct _ByxDevice ByxDevice;
typedef struct _ByxDeviceClass ByxDeviceClass;

GType byx_device_get_type (void);

void *byx_device_get_name(ByxDevice *self);

gboolean byx_device_is_ready(ByxDevice *self);


#endif /* __BYX_DEVICE_H__ */
