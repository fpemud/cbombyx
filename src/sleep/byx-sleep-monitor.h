/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* This program is free software; you can redistribute it and/or modify
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
 * (C) Copyright 2012-2016 Red Hat, Inc.
 * Author: Matthias Clasen <mclasen@redhat.com>
 */

#ifndef __BYX_SLEEP_MONITOR_H__
#define __BYX_SLEEP_MONITOR_H__

#define BYX_TYPE_SLEEP_MONITOR         (byx_sleep_monitor_get_type ())
#define BYX_SLEEP_MONITOR(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BYX_TYPE_SLEEP_MONITOR, ByxSleepMonitor))
#define BYX_SLEEP_MONITOR_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BYX_TYPE_SLEEP_MONITOR, ByxSleepMonitorClass))
#define BYX_SLEEP_MONITOR_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BYX_TYPE_SLEEP_MONITOR, ByxSleepMonitorClass))
#define BYX_IS_SLEEP_MONITOR(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BYX_TYPE_SLEEP_MONITOR))
#define BYX_IS_SLEEP_MONITOR_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BYX_TYPE_SLEEP_MONITOR))

#define BYX_SLEEP_MONITOR_SIGNAL_SLEEPING "sleeping"

typedef struct _ByxSleepMonitorClass ByxSleepMonitorClass;

GType           byx_sleep_monitor_get_type (void) G_GNUC_CONST;

ByxSleepMonitor *byx_sleep_monitor_new (void);

typedef struct _ByxSleepMonitorInhibitorHandle ByxSleepMonitorInhibitorHandle;

ByxSleepMonitorInhibitorHandle *byx_sleep_monitor_inhibit_take    (ByxSleepMonitor *self);
void                            byx_sleep_monitor_inhibit_release (ByxSleepMonitor *self, ByxSleepMonitorInhibitorHandle *handle);

#endif /* __BYX_SLEEP_MONITOR_H__ */
