/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __BYX_CONNECTION_H__
#define __BYX_CONNECTION_H__

#include <netinet/in.h>

#define BYX_TYPE_CONNECTION            (byx_connection_get_type ())
#define BYX_CONNECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BYX_TYPE_CONNECTION, ByxConnection))
#define BYX_CONNECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  BYX_TYPE_CONNECTION, ByxConnectionClass))
#define BYX_IS_CONNECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BYX_TYPE_CONNECTION))
#define BYX_IS_CONNECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  BYX_TYPE_CONNECTION))
#define BYX_CONNECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  BYX_TYPE_CONNECTION, ByxConnectionClass))

typedef struct _ByxConnection ByxConnection;
typedef struct _ByxConnectionClass ByxConnectionClass;

GType byx_connection_get_type (void);

void *byx_connection_get_uuid(ByxConnection *self);

void *byx_connection_get_name(ByxConnection *self);

const char *byx_connection_get_plugin(ByxConnection *self);

ByxNetworkType byx_connection_get_network_type(ByxConnection *self);

GSList *byx_connection_get_associated_devices(ByxConnection *self);


/*NMNetns *byx_connection_get_netns (ByxConnection *self);*/

#if 0
gboolean        byx_connection_is_software           (ByxConnection *dev);
gboolean        byx_connection_is_real               (ByxConnection *dev);
const char *    byx_connection_get_ip_iface          (ByxConnection *dev);
int             byx_connection_get_ip_ifindex        (const ByxConnection *dev);
const char *    byx_connection_get_type_desc         (ByxConnection *dev);
const char *    byx_connection_get_type_description  (ByxConnection *dev);
ByxConnectionType    byx_connection_get_device_type       (ByxConnection *dev);
NMLinkType      byx_connection_get_link_type         (ByxConnection *dev);
NMMetered       byx_connection_get_metered           (ByxConnection *dev);

guint32         byx_connection_get_route_table       (ByxConnection *self, int addr_family, gboolean fallback_main);
guint32         byx_connection_get_route_metric      (ByxConnection *dev, int addr_family);

guint32         byx_connection_get_route_metric_default (ByxConnectionType device_type);

const char *    byx_connection_get_hw_address        (ByxConnection *dev);
const char *    byx_connection_get_permanent_hw_address (ByxConnection *self);
const char *    byx_connection_get_permanent_hw_address_full (ByxConnection *self,
                                                         gboolean force_freeze,
                                                         gboolean *out_is_fake);
const char *    byx_connection_get_initial_hw_address (ByxConnection *dev);

NMDhcp4Config * byx_connection_get_dhcp4_config      (ByxConnection *dev);
NMDhcp6Config * byx_connection_get_dhcp6_config      (ByxConnection *dev);
NMIP4Config *   byx_connection_get_ip4_config        (ByxConnection *dev);
void            byx_connection_replace_vpn4_config   (ByxConnection *dev,
                                                 NMIP4Config *old,
                                                 NMIP4Config *config);

NMIP6Config *   byx_connection_get_ip6_config        (ByxConnection *dev);
void            byx_connection_replace_vpn6_config   (ByxConnection *dev,
                                                 NMIP6Config *old,
                                                 NMIP6Config *config);

void            byx_connection_capture_initial_config (ByxConnection *dev);

int             byx_connection_parent_get_ifindex    (ByxConnection *dev);
ByxConnection       *byx_connection_parent_get_device     (ByxConnection *dev);
void            byx_connection_parent_set_ifindex    (ByxConnection *self,
                                                 int parent_ifindex);
gboolean        byx_connection_parent_notify_changed (ByxConnection *self,
                                                 ByxConnection *change_candidate,
                                                 gboolean device_removed);

/* Master */
gboolean        byx_connection_is_master             (ByxConnection *dev);

/* Slave */
ByxConnection *      byx_connection_get_master            (ByxConnection *dev);

NMActRequest *  byx_connection_get_act_request       (ByxConnection *dev);
NMSettingsConnection *byx_connection_get_settings_connection (ByxConnection *dev);
NMConnection *  byx_connection_get_applied_connection (ByxConnection *dev);
gboolean        byx_connection_has_unmodified_applied_connection (ByxConnection *self,
                                                             NMSettingCompareFlags compare_flags);
NMSetting *     byx_connection_get_applied_setting   (ByxConnection *dev, GType setting_type);

void            byx_connection_removed               (ByxConnection *self, gboolean unconfigure_ip_config);

gboolean        byx_connection_ignore_carrier_by_default (ByxConnection *self);

gboolean        byx_connection_is_available          (ByxConnection *dev, ByxConnectionCheckDevAvailableFlags flags);
gboolean        byx_connection_has_carrier           (ByxConnection *dev);

NMConnection * byx_connection_generate_connection (ByxConnection *self,
                                              ByxConnection *master,
                                              gboolean *out_maybe_later,
                                              GError **error);

gboolean byx_connection_master_update_slave_connection (ByxConnection *master,
                                                   ByxConnection *slave,
                                                   NMConnection *connection,
                                                   GError **error);

gboolean byx_connection_can_auto_connect (ByxConnection *self,
                                     NMConnection *connection,
                                     char **specific_object);

gboolean byx_connection_complete_connection (ByxConnection *device,
                                        NMConnection *connection,
                                        const char *specific_object,
                                        NMConnection *const*existing_connections,
                                        GError **error);

gboolean byx_connection_check_connection_compatible (ByxConnection *device, NMConnection *connection);
gboolean byx_connection_check_slave_connection_compatible (ByxConnection *device, NMConnection *connection);

gboolean byx_connection_unmanage_on_quit (ByxConnection *self);

gboolean byx_connection_spec_match_list (ByxConnection *device, const GSList *specs);
int      byx_connection_spec_match_list_full (ByxConnection *self, const GSList *specs, int no_match_value);

gboolean byx_connection_is_activating (ByxConnection *dev);
gboolean byx_connection_autoconnect_allowed (ByxConnection *self);

ByxConnectionState byx_connection_get_state (ByxConnection *device);

gboolean byx_connection_get_enabled (ByxConnection *device);

void byx_connection_set_enabled (ByxConnection *device, gboolean enabled);

RfKillType byx_connection_get_rfkill_type (ByxConnection *device);

/* IPv6 prefix delegation */

void byx_connection_request_ip6_prefixes (ByxConnection *self, int needed_prefixes);

gboolean byx_connection_needs_ip6_subnet (ByxConnection *self);

void byx_connection_use_ip6_subnet (ByxConnection *self, const NMPlatformIP6Address *subnet);

void byx_connection_copy_ip6_dns_config (ByxConnection *self, ByxConnection *from_device);

/**
 * NMUnmanagedFlags:
 * @NM_UNMANAGED_NONE: placeholder value
 * @NM_UNMANAGED_SLEEPING: %TRUE when unmanaged because NM is sleeping.
 * @NM_UNMANAGED_QUITTING: %TRUE when unmanaged because NM is shutting down.
 * @NM_UNMANAGED_PARENT: %TRUE when unmanaged due to parent device being unmanaged
 * @NM_UNMANAGED_LOOPBACK: %TRUE for unmanaging loopback device
 * @NM_UNMANAGED_PLATFORM_INIT: %TRUE when unmanaged because platform link not
 *   yet initialized. Unrealized device are also unmanaged for this reason.
 * @NM_UNMANAGED_USER_EXPLICIT: %TRUE when unmanaged by explicit user decision
 *   (e.g. via a D-Bus command)
 * @NM_UNMANAGED_USER_SETTINGS: %TRUE when unmanaged by user decision via
 *   the settings plugin (for example keyfile.unmanaged-devices or ifcfg-rh's
 *   NM_CONTROLLED=no). Although this is user-configuration (provided from
 *   the settings plugins, such as NM_CONTROLLED=no in ifcfg-rh), it cannot
 *   be overruled and is authorative. That is because users may depend on
 *   dropping a ifcfg-rh file to ensure the device is unmanaged.
 * @NM_UNMANAGED_USER_CONF: %TRUE when unmanaged by user decision via
 *   the NetworkManager.conf ("unmanaged" in the [device] section).
 *   Contray to @NM_UNMANAGED_USER_SETTINGS, this can be overwritten via
 *   D-Bus.
 * @NM_UNMANAGED_BY_DEFAULT: %TRUE for certain device types where we unmanage
 *   them by default
 * @NM_UNMANAGED_USER_UDEV: %TRUE when unmanaged by user decision (via UDev rule)
 * @NM_UNMANAGED_EXTERNAL_DOWN: %TRUE when unmanaged because !IFF_UP and not created by NM
 * @NM_UNMANAGED_IS_SLAVE: indicates that the device is enslaved. Note that
 *   setting the NM_UNMANAGED_IS_SLAVE to %TRUE makes no sense, this flag has only
 *   meaning to set a slave device as managed if the parent is managed too.
 */
typedef enum { /*< skip >*/
	NM_UNMANAGED_NONE          = 0,

	/* these flags are authorative. If one of them is set,
	 * the device cannot be managed. */
	NM_UNMANAGED_SLEEPING      = (1LL <<  0),
	NM_UNMANAGED_QUITTING      = (1LL <<  1),
	NM_UNMANAGED_PARENT        = (1LL <<  2),
	NM_UNMANAGED_LOOPBACK      = (1LL <<  3),
	NM_UNMANAGED_PLATFORM_INIT = (1LL <<  4),
	NM_UNMANAGED_USER_EXPLICIT = (1LL <<  5),
	NM_UNMANAGED_USER_SETTINGS = (1LL <<  6),

	/* These flags can be non-effective and be overwritten
	 * by other flags. */
	NM_UNMANAGED_BY_DEFAULT    = (1LL <<  8),
	NM_UNMANAGED_USER_CONF     = (1LL <<  9),
	NM_UNMANAGED_USER_UDEV     = (1LL << 10),
	NM_UNMANAGED_EXTERNAL_DOWN = (1LL << 11),
	NM_UNMANAGED_IS_SLAVE      = (1LL << 12),

} NMUnmanagedFlags;

typedef enum {
	NM_UNMAN_FLAG_OP_SET_MANAGED        = FALSE,
	NM_UNMAN_FLAG_OP_SET_UNMANAGED      = TRUE,
	NM_UNMAN_FLAG_OP_FORGET             = 2,
} NMUnmanFlagOp;

const char *nm_unmanaged_flags2str (NMUnmanagedFlags flags, char *buf, gsize len);

gboolean byx_connection_get_managed (ByxConnection *device, gboolean for_user_request);
NMUnmanagedFlags byx_connection_get_unmanaged_mask (ByxConnection *device, NMUnmanagedFlags flag);
NMUnmanagedFlags byx_connection_get_unmanaged_flags (ByxConnection *device, NMUnmanagedFlags flag);
void byx_connection_set_unmanaged_flags (ByxConnection *device,
                                    NMUnmanagedFlags flags,
                                    NMUnmanFlagOp set_op);
void byx_connection_set_unmanaged_by_flags (ByxConnection *device,
                                       NMUnmanagedFlags flags,
                                       NMUnmanFlagOp set_op,
                                       ByxConnectionStateReason reason);
void byx_connection_set_unmanaged_by_flags_queue (ByxConnection *self,
                                             NMUnmanagedFlags flags,
                                             NMUnmanFlagOp set_op,
                                             ByxConnectionStateReason reason);
void byx_connection_set_unmanaged_by_user_settings (ByxConnection *self);
void byx_connection_set_unmanaged_by_user_udev (ByxConnection *self);
void byx_connection_set_unmanaged_by_user_conf (ByxConnection *self);
void byx_connection_set_unmanaged_by_quitting (ByxConnection *device);

gboolean byx_connection_is_nm_owned (ByxConnection *device);

gboolean byx_connection_has_capability (ByxConnection *self, ByxConnectionCapabilities caps);

/*****************************************************************************/

void byx_connection_assume_state_get (ByxConnection *self,
                                 gboolean *out_assume_state_guess_assume,
                                 const char **out_assume_state_connection_uuid);
void byx_connection_assume_state_reset (ByxConnection *self);

/*****************************************************************************/

gboolean byx_connection_realize_start      (ByxConnection *device,
                                       const NMPlatformLink *plink,
                                       gboolean assume_state_guess_assume,
                                       const char *assume_state_connection_uuid,
                                       gboolean set_nm_owned,
                                       NMUnmanFlagOp unmanaged_user_explicit,
                                       gboolean *out_compatible,
                                       GError **error);
void     byx_connection_realize_finish     (ByxConnection *self,
                                       const NMPlatformLink *plink);
gboolean byx_connection_create_and_realize (ByxConnection *self,
                                       NMConnection *connection,
                                       ByxConnection *parent,
                                       GError **error);
gboolean byx_connection_unrealize          (ByxConnection *device,
                                       gboolean remove_resources,
                                       GError **error);

void byx_connection_update_from_platform_link (ByxConnection *self,
                                          const NMPlatformLink *plink);

typedef enum {
	BYX_CONNECTION_AUTOCONNECT_BLOCKED_NONE                  = 0,

	BYX_CONNECTION_AUTOCONNECT_BLOCKED_USER                  = (1LL <<  0),

	BYX_CONNECTION_AUTOCONNECT_BLOCKED_WRONG_PIN             = (1LL <<  1),
	BYX_CONNECTION_AUTOCONNECT_BLOCKED_MANUAL_DISCONNECT     = (1LL <<  2),

	_BYX_CONNECTION_AUTOCONNECT_BLOCKED_LAST,

	BYX_CONNECTION_AUTOCONNECT_BLOCKED_ALL                   = (((_BYX_CONNECTION_AUTOCONNECT_BLOCKED_LAST - 1) << 1) - 1),

	BYX_CONNECTION_AUTOCONNECT_BLOCKED_INTERNAL              = BYX_CONNECTION_AUTOCONNECT_BLOCKED_ALL & ~BYX_CONNECTION_AUTOCONNECT_BLOCKED_USER,
} ByxConnectionAutoconnectBlockedFlags;

ByxConnectionAutoconnectBlockedFlags byx_connection_autoconnect_blocked_get (ByxConnection *device, ByxConnectionAutoconnectBlockedFlags mask);

void byx_connection_autoconnect_blocked_set_full (ByxConnection *device, ByxConnectionAutoconnectBlockedFlags mask, ByxConnectionAutoconnectBlockedFlags values);

static inline void
byx_connection_autoconnect_blocked_set (ByxConnection *device, ByxConnectionAutoconnectBlockedFlags mask)
{
	byx_connection_autoconnect_blocked_set_full (device, mask, mask);
}

static inline void
byx_connection_autoconnect_blocked_unset (ByxConnection *device, ByxConnectionAutoconnectBlockedFlags mask)
{
	byx_connection_autoconnect_blocked_set_full (device, mask, BYX_CONNECTION_AUTOCONNECT_BLOCKED_NONE);
}

void byx_connection_emit_recheck_auto_activate (ByxConnection *device);

ByxConnectionSysIfaceState byx_connection_sys_iface_state_get (ByxConnection *device);

gboolean byx_connection_sys_iface_state_is_external (ByxConnection *self);
gboolean byx_connection_sys_iface_state_is_external_or_assume (ByxConnection *self);

void byx_connection_sys_iface_state_set (ByxConnection *device, ByxConnectionSysIfaceState sys_iface_state);

void byx_connection_state_changed (ByxConnection *device,
                              ByxConnectionState state,
                              ByxConnectionStateReason reason);

void byx_connection_queue_state   (ByxConnection *self,
                              ByxConnectionState state,
                              ByxConnectionStateReason reason);

gboolean byx_connection_get_firmware_missing (ByxConnection *self);

void byx_connection_disconnect_active_connection (NMActiveConnection *active);

void byx_connection_queue_activation (ByxConnection *device, NMActRequest *req);

gboolean byx_connection_supports_vlans (ByxConnection *device);

gboolean byx_connection_add_pending_action    (ByxConnection *device, const char *action, gboolean assert_not_yet_pending);
gboolean byx_connection_remove_pending_action (ByxConnection *device, const char *action, gboolean assert_is_pending);
gboolean byx_connection_has_pending_action    (ByxConnection *device);

NMSettingsConnection *byx_connection_get_best_connection (ByxConnection *device,
                                                     const char *specific_object,
                                                     GError **error);

gboolean   byx_connection_check_connection_available (ByxConnection *device,
                                                 NMConnection *connection,
                                                 ByxConnectionCheckConAvailableFlags flags,
                                                 const char *specific_object);

gboolean byx_connection_notify_component_added (ByxConnection *device, GObject *component);

gboolean byx_connection_owns_iface (ByxConnection *device, const char *iface);

NMConnection *byx_connection_new_default_connection (ByxConnection *self);

const NMPObject *byx_connection_get_best_default_route (ByxConnection *self,
                                                   int addr_family);

void byx_connection_spawn_iface_helper (ByxConnection *self);

gboolean byx_connection_reapply (ByxConnection *self,
                            NMConnection *connection,
                            GError **error);
void byx_connection_reapply_settings_immediately (ByxConnection *self);

void byx_connection_update_firewall_zone (ByxConnection *self);
void byx_connection_update_metered (ByxConnection *self);
void byx_connection_reactivate_ip4_config (ByxConnection *device,
                                      NMSettingIPConfig *s_ip4_old,
                                      NMSettingIPConfig *s_ip4_new);
void byx_connection_reactivate_ip6_config (ByxConnection *device,
                                      NMSettingIPConfig *s_ip6_old,
                                      NMSettingIPConfig *s_ip6_new);

gboolean byx_connection_update_hw_address (ByxConnection *self);
void byx_connection_update_initial_hw_address (ByxConnection *self);
void byx_connection_update_permanent_hw_address (ByxConnection *self, gboolean force_freeze);
void byx_connection_update_dynamic_ip_setup (ByxConnection *self);
guint byx_connection_get_supplicant_timeout (ByxConnection *self);

gboolean byx_connection_auth_retries_try_next (ByxConnection *self);

typedef struct _ByxConnectionConnectivityHandle ByxConnectionConnectivityHandle;

typedef void (*ByxConnectionConnectivityCallback) (ByxConnection *self,
                                              ByxConnectionConnectivityHandle *handle,
                                              NMConnectivityState state,
                                              GError *error,
                                              gpointer user_data);

void byx_connection_check_connectivity_update_interval (ByxConnection *self);

ByxConnectionConnectivityHandle *byx_connection_check_connectivity (ByxConnection *self,
                                                          ByxConnectionConnectivityCallback callback,
                                                          gpointer user_data);

void byx_connection_check_connectivity_cancel (ByxConnectionConnectivityHandle *handle);

NMConnectivityState byx_connection_get_connectivity_state (ByxConnection *self);

typedef struct _NMBtVTableNetworkServer NMBtVTableNetworkServer;
struct _NMBtVTableNetworkServer {
	gboolean (*is_available) (const NMBtVTableNetworkServer *vtable,
	                          const char *addr);
	gboolean (*register_bridge) (const NMBtVTableNetworkServer *vtable,
	                             const char *addr,
	                             ByxConnection *device);
	gboolean (*unregister_bridge) (const NMBtVTableNetworkServer *vtable,
	                               ByxConnection *device);
};

const char *byx_connection_state_to_str (ByxConnectionState state);
const char *byx_connection_state_reason_to_str (ByxConnectionStateReason reason);
#endif

#endif /* __BYX_CONNECTION_H__ */
