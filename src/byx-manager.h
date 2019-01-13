/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __BYX_MANAGER_H__
#define __BYX_MANAGER_H__

#define BYX_TYPE_MANAGER            (byx_manager_get_type ())
#define BYX_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BYX_TYPE_MANAGER, ByxManager))
#define BYX_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BYX_TYPE_MANAGER, ByxManagerClass))
#define BYX_IS_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BYX_TYPE_MANAGER))
#define BYX_IS_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BYX_TYPE_MANAGER))
#define BYX_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BYX_TYPE_MANAGER, ByxManagerClass))

GType byx_manager_get_type (void);

ByxManager *byx_manager_get (void);

/*****************************************************************************/

/**
 * ByxState:
 * @BYX_STATE_UNKNOWN: networking state is unknown
 * @BYX_STATE_ASLEEP: networking is not enabled
 * @BYX_STATE_DISCONNECTED: there is no active network connection
 * @BYX_STATE_DISCONNECTING: network connections are being cleaned up
 * @BYX_STATE_CONNECTING: a network connection is being started
 * @BYX_STATE_CONNECTED_LOCAL: there is only local IPv4 and/or IPv6 connectivity
 * @BYX_STATE_CONNECTED_SITE: there is only site-wide IPv4 and/or IPv6 connectivity
 * @BYX_STATE_CONNECTED_GLOBAL: there is global IPv4 and/or IPv6 Internet connectivity
 *
 * #ByxState values indicate the current overall networking state.
 **/
typedef enum {
	BYX_STATE_UNKNOWN          = 0,
	BYX_STATE_ASLEEP           = 10,
	BYX_STATE_DISCONNECTED     = 20,
	BYX_STATE_DISCONNECTING    = 30,
	BYX_STATE_CONNECTING       = 40,
	BYX_STATE_CONNECTED_LOCAL  = 50,
	BYX_STATE_CONNECTED_SITE   = 60,
	BYX_STATE_CONNECTED_GLOBAL = 70,
} ByxState;

/**
 * ByxConnectivityState:
 * @BYX_CONNECTIVITY_UNKNOWN: Network connectivity is unknown.
 * @BYX_CONNECTIVITY_NONE: The host is not connected to any network.
 * @BYX_CONNECTIVITY_PORTAL: The host is behind a captive portal and
 *   cannot reach the full Internet.
 * @BYX_CONNECTIVITY_LIMITED: The host is connected to a network, but
 *   does not appear to be able to reach the full Internet.
 * @BYX_CONNECTIVITY_FULL: The host is connected to a network, and
 *   appears to be able to reach the full Internet.
 */
typedef enum {
	BYX_CONNECTIVITY_UNKNOWN = 0,
	BYX_CONNECTIVITY_NONE    = 1,
	BYX_CONNECTIVITY_PORTAL  = 2,
	BYX_CONNECTIVITY_LIMITED = 3,
	BYX_CONNECTIVITY_FULL    = 4,
} ByxConnectivityState;

gboolean byx_manager_start (ByxManager *manager, GError **error);
void byx_manager_stop (ByxManager *manager);

ByxState byx_manager_get_state (ByxManager *manager);

/* Device handling */

NMActiveConnection *byx_manager_activate_connection     (ByxManager *manager,
                                                        NMSettingsConnection *connection,
                                                        NMConnection *applied_connection,
                                                        const char *specific_object,
                                                        NMDevice *device,
                                                        NMAuthSubject *subject,
                                                        NMActivationType activation_type,
                                                        NMActivationReason activation_reason,
                                                        GError **error);

gboolean            byx_manager_deactivate_connection   (ByxManager *manager,
                                                        NMActiveConnection *active,
                                                        NMDeviceStateReason reason,
                                                        GError **error);

void                byx_manager_set_capability   (ByxManager *self, NMCapability cap);

NMDevice *          byx_manager_get_device    (ByxManager *self,
                                              const char *ifname,
                                              NMDeviceType device_type);
gboolean            byx_manager_remove_device (ByxManager *self,
                                              const char *ifname,
                                              NMDeviceType device_type);

void byx_manager_dbus_set_property_handle (NMDBusObject *obj,
                                          const NMDBusInterfaceInfoExtended *interface_info,
                                          const NMDBusPropertyInfoExtended *property_info,
                                          GDBusConnection *connection,
                                          const char *sender,
                                          GDBusMethodInvocation *invocation,
                                          GVariant *value,
                                          gpointer user_data);


/*******************************************************/

#define BYX_MANAGER_VERSION "version"
#define BYX_MANAGER_CAPABILITIES "capabilities"
#define BYX_MANAGER_STATE "state"
#define BYX_MANAGER_STARTUP "startup"
#define BYX_MANAGER_NETWORKING_ENABLED "networking-enabled"
#define BYX_MANAGER_WIRELESS_ENABLED "wireless-enabled"
#define BYX_MANAGER_WIRELESS_HARDWARE_ENABLED "wireless-hardware-enabled"
#define BYX_MANAGER_WWAN_ENABLED "wwan-enabled"
#define BYX_MANAGER_WWAN_HARDWARE_ENABLED "wwan-hardware-enabled"
#define BYX_MANAGER_WIMAX_ENABLED "wimax-enabled"
#define BYX_MANAGER_WIMAX_HARDWARE_ENABLED "wimax-hardware-enabled"
#define BYX_MANAGER_ACTIVE_CONNECTIONS "active-connections"
#define BYX_MANAGER_CONNECTIVITY "connectivity"
#define BYX_MANAGER_CONNECTIVITY_CHECK_AVAILABLE "connectivity-check-available"
#define BYX_MANAGER_CONNECTIVITY_CHECK_ENABLED "connectivity-check-enabled"
#define BYX_MANAGER_PRIMARY_CONNECTION "primary-connection"
#define BYX_MANAGER_PRIMARY_CONNECTION_TYPE "primary-connection-type"
#define BYX_MANAGER_ACTIVATING_CONNECTION "activating-connection"
#define BYX_MANAGER_DEVICES "devices"
#define BYX_MANAGER_METERED "metered"
#define BYX_MANAGER_GLOBAL_DNS_CONFIGURATION "global-dns-configuration"
#define BYX_MANAGER_ALL_DEVICES "all-devices"
#define BYX_MANAGER_CHECKPOINTS "checkpoints"

/* Not exported */
#define BYX_MANAGER_SLEEPING "sleeping"

/* Signals */
#define BYX_MANAGER_DEVICE_ADDED              "device-added"
#define BYX_MANAGER_DEVICE_REMOVED            "device-removed"
#define BYX_MANAGER_USER_PERMISSIONS_CHANGED  "user-permissions-changed"

#define BYX_MANAGER_ACTIVE_CONNECTION_ADDED   "active-connection-added"
#define BYX_MANAGER_ACTIVE_CONNECTION_REMOVED "active-connection-removed"
#define BYX_MANAGER_CONFIGURE_QUIT            "configure-quit"
#define BYX_MANAGER_INTERNAL_DEVICE_ADDED     "internal-device-added"
#define BYX_MANAGER_INTERNAL_DEVICE_REMOVED   "internal-device-removed"



#endif /* __BYX_MANAGER_H__ */
