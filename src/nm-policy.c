/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* NetworkManager -- Network link manager
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
 * Copyright (C) 2004 - 2013 Red Hat, Inc.
 * Copyright (C) 2007 - 2008 Novell, Inc.
 */

#include "nm-default.h"

#include "nm-policy.h"

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>

#include "NetworkManagerUtils.h"
#include "nm-act-request.h"
#include "devices/nm-device.h"
#include "nm-setting-ip4-config.h"
#include "nm-setting-connection.h"
#include "platform/nm-platform.h"
#include "dns/nm-dns-manager.h"
#include "vpn/nm-vpn-manager.h"
#include "nm-auth-utils.h"
#include "nm-firewall-manager.h"
#include "nm-utils.h"
#include "nm-core-internal.h"
#include "nm-manager.h"
#include "settings/nm-settings.h"
#include "settings/nm-settings-connection.h"
#include "settings/nm-agent-manager.h"
#include "nm-dhcp4-config.h"
#include "nm-dhcp6-config.h"
#include "nm-config.h"
#include "nm-netns.h"
#include "nm-hostname-manager.h"

/*****************************************************************************/

NM_GOBJECT_PROPERTIES_DEFINE (NMPolicy,
	PROP_MANAGER,
	PROP_SETTINGS,
	PROP_DEFAULT_IP4_DEVICE,
	PROP_DEFAULT_IP6_DEVICE,
	PROP_ACTIVATING_IP4_DEVICE,
	PROP_ACTIVATING_IP6_DEVICE,
);

typedef struct {
	ByxManager *manager;
	NMNetns *netns;
	NMFirewallManager *firewall_manager;
	CList pending_activation_checks;

	NMAgentManager *agent_mgr;

	GHashTable *devices;
	GHashTable *pending_active_connections;

	GSList *pending_secondaries;

	NMSettings *settings;

	NMHostnameManager *hostname_manager;

	NMDevice *default_device4, *activating_device4;
	NMDevice *default_device6, *activating_device6;

	NMDnsManager *dns_manager;
	gulong config_changed_id;

	guint reset_retries_id;  /* idle handler for resetting the retries count */

	guint schedule_activate_all_id; /* idle handler for schedule_activate_all(). */

	GArray *ip6_prefix_delegations; /* pool of ip6 prefixes delegated to all devices */
} NMPolicyPrivate;

struct _NMPolicy {
	GObject parent;
	NMPolicyPrivate _priv;
};

struct _NMPolicyClass {
	GObjectClass parent;
};

G_DEFINE_TYPE (NMPolicy, nm_policy, G_TYPE_OBJECT)

#define NM_POLICY_GET_PRIVATE(self) _BYX_GET_PRIVATE (self, NMPolicy, NM_IS_POLICY)

static NMPolicy *
_PRIV_TO_SELF (NMPolicyPrivate *priv)
{
	NMPolicy *self;

	nm_assert (priv);

	self = (NMPolicy *) (((char *) priv) - G_STRUCT_OFFSET (NMPolicy, _priv));

	nm_assert (NM_IS_POLICY (self));
	return self;
}

/*****************************************************************************/

#define _NMLOG_PREFIX_NAME    "policy"
#define _NMLOG(level, domain, ...) \
    G_STMT_START { \
        byx_log ((level), (domain), NULL, NULL, \
                "%s" _NM_UTILS_MACRO_FIRST (__VA_ARGS__), \
                _NMLOG_PREFIX_NAME": " \
                _NM_UTILS_MACRO_REST (__VA_ARGS__)); \
    } G_STMT_END

/*****************************************************************************/

static void schedule_activate_all (NMPolicy *self);
static void schedule_activate_check (NMPolicy *self, NMDevice *device);

/*****************************************************************************/

typedef struct {
	NMPlatformIP6Address prefix;
	NMDevice *device;             /* The requesting ("uplink") device */
	guint64 next_subnet;          /* Cache of the next subnet number to be
	                               * assigned from this prefix */
	GHashTable *subnets;          /* ifindex -> NMPlatformIP6Address */
} IP6PrefixDelegation;

static void
_clear_ip6_subnet (gpointer key, gpointer value, gpointer user_data)
{
	NMPlatformIP6Address *subnet = value;
	NMDevice *device = byx_manager_get_device_by_ifindex (byx_manager_get (),
	                                                     GPOINTER_TO_INT (key));

	if (device) {
		/* We can not remove a subnet we already started announcing.
		 * Just un-prefer it. */
		subnet->preferred = 0;
		nm_device_use_ip6_subnet (device, subnet);
	}
	g_slice_free (NMPlatformIP6Address, subnet);
}

static void
clear_ip6_prefix_delegation (gpointer data)
{
	IP6PrefixDelegation *delegation = data;

	_LOGD (LOGD_IP6, "ipv6-pd: undelegating prefix %s/%d",
	       byx_utils_inet6_ntop (&delegation->prefix.address, NULL),
	       delegation->prefix.plen);

	g_hash_table_foreach (delegation->subnets, _clear_ip6_subnet, NULL);
	g_hash_table_destroy (delegation->subnets);
}

static void
expire_ip6_delegations (NMPolicy *self)
{
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);
	guint32 now = byx_utils_get_monotonic_timestamp_s ();
	IP6PrefixDelegation *delegation = NULL;
	guint i;

	for (i = 0; i < priv->ip6_prefix_delegations->len; i++) {
		delegation = &g_array_index (priv->ip6_prefix_delegations,
		                             IP6PrefixDelegation, i);
		if (delegation->prefix.timestamp + delegation->prefix.lifetime < now)
			g_array_remove_index_fast (priv->ip6_prefix_delegations, i);
	}
}

/*
 * Try to obtain a new subnet for a particular active connection from given
 * delegated prefix, possibly reusing the existing subnet.
 * Return value of FALSE indicates no more subnets are available from
 * this prefix (and other prefix should be used -- and requested if necessary).
 */
static gboolean
ip6_subnet_from_delegation (IP6PrefixDelegation *delegation, NMDevice *device)
{
	NMPlatformIP6Address *subnet;
	int ifindex = nm_device_get_ifindex (device);

	subnet = g_hash_table_lookup (delegation->subnets, GINT_TO_POINTER (ifindex));
	if (!subnet) {
		/* Check for out-of-prefixes condition. */
		if (delegation->next_subnet >= (1 << (64 - delegation->prefix.plen))) {
			_LOGD (LOGD_IP6, "ipv6-pd: no more prefixes in %s/%d",
			       byx_utils_inet6_ntop (&delegation->prefix.address, NULL),
			       delegation->prefix.plen);
			return FALSE;
		}

		/* Allocate a new subnet. */
		subnet = g_slice_new0 (NMPlatformIP6Address);
		g_hash_table_insert (delegation->subnets, GINT_TO_POINTER (ifindex), subnet);

		subnet->plen = 64;
		subnet->address.s6_addr32[0] =   delegation->prefix.address.s6_addr32[0]
		                               | htonl (delegation->next_subnet >> 32);
		subnet->address.s6_addr32[1] =   delegation->prefix.address.s6_addr32[1]
		                               | htonl (delegation->next_subnet);

		/* Out subnet pool management is pretty unsophisticated. We only add
		 * the subnets and index them by ifindex. That keeps the implementation
		 * simple and the dead entries make it easy to reuse the same subnet on
		 * subsequent activations. On the other hand they may waste the subnet
		 * space. */
		delegation->next_subnet++;
	}

	subnet->timestamp = delegation->prefix.timestamp;
	subnet->lifetime = delegation->prefix.lifetime;
	subnet->preferred = delegation->prefix.preferred;

	_LOGD (LOGD_IP6, "ipv6-pd: %s allocated from a /%d prefix on %s",
	       byx_utils_inet6_ntop (&subnet->address, NULL),
	       delegation->prefix.plen,
	       nm_device_get_iface (device));

	nm_device_use_ip6_subnet (device, subnet);

	return TRUE;
}

/*
 * Try to obtain a subnet from each prefix delegated to given requesting
 * ("uplink") device and assign it to the downlink device.
 * Requests a new prefix if no subnet could be found.
 */
static void
ip6_subnet_from_device (NMPolicy *self, NMDevice *from_device, NMDevice *device)
{
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);
	IP6PrefixDelegation *delegation = NULL;
	gboolean got_subnet = FALSE;
	guint have_prefixes = 0;
	guint i;

	expire_ip6_delegations (self);

	for (i = 0; i < priv->ip6_prefix_delegations->len; i++) {
		delegation = &g_array_index (priv->ip6_prefix_delegations,
		                             IP6PrefixDelegation, i);

		if (delegation->device != from_device)
			continue;

		if (ip6_subnet_from_delegation (delegation, device))
			got_subnet = TRUE;
		have_prefixes++;
	}

	if (!got_subnet) {
		_LOGI (LOGD_IP6, "ipv6-pd: none of %u prefixes of %s can be shared on %s",
		       have_prefixes, nm_device_get_iface (from_device),
		       nm_device_get_iface (device));
		nm_device_request_ip6_prefixes (from_device, have_prefixes + 1);
	}
}

static void
ip6_remove_device_prefix_delegations (NMPolicy *self, NMDevice *device)
{
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);
	IP6PrefixDelegation *delegation = NULL;
	guint i;

	for (i = 0; i < priv->ip6_prefix_delegations->len; i++) {
		delegation = &g_array_index (priv->ip6_prefix_delegations,
		                             IP6PrefixDelegation, i);
		if (delegation->device == device)
			g_array_remove_index_fast (priv->ip6_prefix_delegations, i);
	}
}

static void
device_ip6_prefix_delegated (NMDevice *device,
                             NMPlatformIP6Address *prefix,
                             gpointer user_data)
{
	NMPolicyPrivate *priv = user_data;
	NMPolicy *self = _PRIV_TO_SELF (priv);
	IP6PrefixDelegation *delegation = NULL;
	guint i;
	const CList *tmp_list;
	NMActiveConnection *ac;

	_LOGI (LOGD_IP6, "ipv6-pd: received a prefix %s/%d from %s",
	       byx_utils_inet6_ntop (&prefix->address, NULL),
	       prefix->plen,
	       nm_device_get_iface (device));

	expire_ip6_delegations (self);

	for (i = 0; i < priv->ip6_prefix_delegations->len; i++) {
		/* Look for an already known prefix to update. */
		delegation = &g_array_index (priv->ip6_prefix_delegations, IP6PrefixDelegation, i);
		if (IN6_ARE_ADDR_EQUAL (&delegation->prefix.address, &prefix->address))
			break;
	}

	if (i == priv->ip6_prefix_delegations->len) {
		/* Allocate a delegation delegation for new prefix. */
		g_array_set_size (priv->ip6_prefix_delegations, i + 1);
		delegation = &g_array_index (priv->ip6_prefix_delegations, IP6PrefixDelegation, i);
		delegation->subnets = g_hash_table_new (nm_direct_hash, NULL);
		delegation->next_subnet = 0;
	}

	delegation->device = device;
	delegation->prefix = *prefix;

	/* The newly activated connections are added to the list beginning,
	 * so traversing it from the beginning makes it likely for newly
	 * activated connections that have no subnet assigned to be served
	 * first. That is a simple yet fair policy, which is good. */
	byx_manager_for_each_active_connection (priv->manager, ac, tmp_list) {
		NMDevice *to_device;

		to_device = nm_active_connection_get_device (ac);
		if (nm_device_needs_ip6_subnet (to_device))
			ip6_subnet_from_delegation (delegation, to_device);
	}
}

static void
device_ip6_subnet_needed (NMDevice *device,
                          gpointer user_data)
{
	NMPolicyPrivate *priv = user_data;
	NMPolicy *self = _PRIV_TO_SELF (priv);

	_LOGD (LOGD_IP6, "ipv6-pd: %s needs a subnet",
	       nm_device_get_iface (device));

	if (!priv->default_device6) {
		/* We request the prefixes when the default IPv6 device is set. */
		_LOGI (LOGD_IP6, "ipv6-pd: no device to obtain a subnet to share on %s from",
		       nm_device_get_iface (device));
		return;
	}
	ip6_subnet_from_device (self, priv->default_device6, device);
	nm_device_copy_ip6_dns_config (device, priv->default_device6);
}

/*****************************************************************************/

static NMDevice *
get_best_ip_device (NMPolicy *self,
                    int addr_family,
                    gboolean fully_activated)
{
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);
	const CList *tmp_lst;
	NMDevice *device;
	NMDevice *best_device;
	NMDevice *prev_device;
	guint32 best_metric = G_MAXUINT32;
	gboolean best_is_fully_activated = FALSE;

	nm_assert (NM_IN_SET (addr_family, AF_INET, AF_INET6));

	/* we prefer the current device in case of identical metric.
	 * Hence, try that one first.*/
	best_device = NULL;
	prev_device =   addr_family == AF_INET
	              ? (fully_activated ? priv->default_device4 : priv->activating_device4)
	              : (fully_activated ? priv->default_device6 : priv->activating_device6);

	byx_manager_for_each_device (priv->manager, device, tmp_lst) {
		NMDeviceState state;
		const NMPObject *r;
		NMConnection *connection;
		guint32 metric;
		gboolean is_fully_activated;

		state = nm_device_get_state (device);
		if (   state <= NM_DEVICE_STATE_DISCONNECTED
		    || state >= NM_DEVICE_STATE_DEACTIVATING)
			continue;

		if (nm_device_sys_iface_state_is_external (device))
			continue;

		r = nm_device_get_best_default_route (device, addr_family);
		if (r) {
			/* NOTE: the best route might have rt_source NM_IP_CONFIG_SOURCE_VPN,
			 * which means it was injected by a VPN, not added by device.
			 *
			 * In this case, is it really the best device? Why do we even need the best
			 * device?? */
			metric = byx_utils_ip_route_metric_normalize (addr_family,
			                                             NMP_OBJECT_CAST_IP_ROUTE (r)->metric);
			is_fully_activated = TRUE;
		} else if (   !fully_activated
		           && (connection = nm_device_get_applied_connection (device))
		           && byx_utils_connection_has_default_route (connection, addr_family, NULL)) {
			metric = byx_utils_ip_route_metric_normalize (addr_family,
			                                             nm_device_get_route_metric (device, addr_family));
			is_fully_activated = FALSE;
		} else
			continue;

		if (   !best_device
		    || (!best_is_fully_activated && is_fully_activated)
		    || (   metric < best_metric
		        || (metric == best_metric && device == prev_device))) {
			best_device = device;
			best_metric = metric;
			best_is_fully_activated = is_fully_activated;
		}
	}

	if (   !fully_activated
	    && best_device
	    && best_is_fully_activated) {
		/* There's only a best activating device if the best device
		 * among all activating and already-activated devices is a
		 * still-activating one. */
		return NULL;
	}

	return best_device;
}

static gboolean
all_devices_not_active (NMPolicy *self)
{
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);
	const CList *tmp_lst;
	NMDevice *device;

	byx_manager_for_each_device (priv->manager, device, tmp_lst) {
		NMDeviceState state;

		state = nm_device_get_state (device);
		if (   state <= NM_DEVICE_STATE_DISCONNECTED
		    || state >= NM_DEVICE_STATE_DEACTIVATING) {
			continue;
		}
		return FALSE;
	}
	return TRUE;
}

static void
update_default_ac (NMPolicy *self,
                   int addr_family,
                   NMActiveConnection *best)
{
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);
	const CList *tmp_list;
	NMActiveConnection *ac;

	/* Clear the 'default[6]' flag on all active connections that aren't the new
	 * default active connection.  We'll set the new default after; this ensures
	 * we don't ever have two marked 'default[6]' simultaneously.
	 */
	byx_manager_for_each_active_connection (priv->manager, ac, tmp_list) {
		if (ac != best)
			nm_active_connection_set_default (ac, addr_family, FALSE);
	}

	/* Mark new default active connection */
	if (best)
		nm_active_connection_set_default (best, addr_family, TRUE);
}

static gpointer
get_best_ip_config (NMPolicy *self,
                    int addr_family,
                    const char **out_ip_iface,
                    NMActiveConnection **out_ac,
                    NMDevice **out_device,
                    ByxService **out_vpn)
{
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);
	NMDevice *device;
	gpointer conf;
	const CList *tmp_list;
	NMActiveConnection *ac;

	nm_assert (NM_IN_SET (addr_family, AF_INET, AF_INET6));

	byx_manager_for_each_active_connection (priv->manager, ac, tmp_list) {
		ByxService *candidate;
		NMVpnConnectionState vpn_state;

		if (!NM_IS_VPN_CONNECTION (ac))
			continue;

		candidate = NM_VPN_CONNECTION (ac);

		vpn_state = nm_vpn_connection_get_vpn_state (candidate);
		if (vpn_state != NM_VPN_CONNECTION_STATE_ACTIVATED)
			continue;

		if (addr_family == AF_INET)
			conf = nm_vpn_connection_get_ip4_config (candidate);
		else
			conf = nm_vpn_connection_get_ip6_config (candidate);
		if (!conf)
			continue;

		if (addr_family == AF_INET) {
			if (!nm_ip4_config_best_default_route_get (conf))
				continue;
		} else {
			if (!nm_ip6_config_best_default_route_get (conf))
				continue;
		}

		/* FIXME: in case of multiple VPN candidates, choose the one with the
		 * best metric. */
		NM_SET_OUT (out_device, NULL);
		NM_SET_OUT (out_vpn, candidate);
		NM_SET_OUT (out_ac, ac);
		NM_SET_OUT (out_ip_iface, nm_vpn_connection_get_ip_iface (candidate, TRUE));
		return conf;
	}

	device = get_best_ip_device (self, addr_family, TRUE);
	if (device) {
		NMActRequest *req;

		if (addr_family == AF_INET)
			conf = nm_device_get_ip4_config (device);
		else
			conf = nm_device_get_ip6_config (device);
		req = nm_device_get_act_request (device);

		if (conf && req) {
			NM_SET_OUT (out_device, device);
			NM_SET_OUT (out_vpn, NULL);
			NM_SET_OUT (out_ac, NM_ACTIVE_CONNECTION (req));
			NM_SET_OUT (out_ip_iface, nm_device_get_ip_iface (device));
			return conf;
		}
	}

	NM_SET_OUT (out_device, NULL);
	NM_SET_OUT (out_vpn, NULL);
	NM_SET_OUT (out_ac, NULL);
	NM_SET_OUT (out_ip_iface, NULL);
	return NULL;
}

static void
update_ip4_routing (NMPolicy *self, gboolean force_update)
{
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);
	NMDevice *best = NULL;
	ByxService *vpn = NULL;
	NMActiveConnection *best_ac = NULL;
	const char *ip_iface = NULL;
	const CList *tmp_list;
	NMActiveConnection *ac;

	/* Note that we might have an IPv4 VPN tunneled over an IPv6-only device,
	 * so we can get (vpn != NULL && best == NULL).
	 */
	if (!get_best_ip_config (self, AF_INET, &ip_iface, &best_ac, &best, &vpn)) {
		if (nm_clear_g_object (&priv->default_device4)) {
			_LOGt (LOGD_DNS, "set-default-device-4: %p", NULL);
			_notify (self, PROP_DEFAULT_IP4_DEVICE);
		}
		return;
	}
	g_assert ((best || vpn) && best_ac);

	if (   !force_update
	    && best
	    && best == priv->default_device4)
		return;

	if (best) {
		byx_manager_for_each_active_connection (priv->manager, ac, tmp_list) {
			if (   NM_IS_VPN_CONNECTION (ac)
			    && nm_vpn_connection_get_ip4_config (NM_VPN_CONNECTION (ac))
			    && !nm_active_connection_get_device (ac))
				nm_active_connection_set_device (ac, best);
		}
	}

	if (vpn)
		best = nm_active_connection_get_device (NM_ACTIVE_CONNECTION (vpn));

	update_default_ac (self, AF_INET, best_ac);

	if (!nm_g_object_ref_set (&priv->default_device4, best))
		return;
	_LOGt (LOGD_DNS, "set-default-device-4: %p", priv->default_device4);

	_LOGI (LOGD_CORE, "set '%s' (%s) as default for IPv4 routing and DNS",
	       nm_connection_get_id (nm_active_connection_get_applied_connection (best_ac)),
	       ip_iface);
	_notify (self, PROP_DEFAULT_IP4_DEVICE);
}

static void
update_ip6_dns_delegation (NMPolicy *self)
{
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);
	NMDevice *device;
	NMActiveConnection *ac;
	const CList *tmp_list;

	byx_manager_for_each_active_connection (priv->manager, ac, tmp_list) {
		device = nm_active_connection_get_device (ac);
		if (device && nm_device_needs_ip6_subnet (device))
			nm_device_copy_ip6_dns_config (device, priv->default_device6);
	}
}

static void
update_ip6_prefix_delegation (NMPolicy *self)
{
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);
	NMDevice *device;
	NMActiveConnection *ac;
	const CList *tmp_list;

	/* There's new default IPv6 connection, try to get a prefix for everyone. */
	byx_manager_for_each_active_connection (priv->manager, ac, tmp_list) {
		device = nm_active_connection_get_device (ac);
		if (device && nm_device_needs_ip6_subnet (device))
			ip6_subnet_from_device (self, priv->default_device6, device);
	}
}

static void
update_ip6_routing (NMPolicy *self, gboolean force_update)
{
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);
	NMDevice *best = NULL;
	ByxService *vpn = NULL;
	NMActiveConnection *best_ac = NULL;
	const char *ip_iface = NULL;
	NMActiveConnection *ac;
	const CList *tmp_list;

	/* Note that we might have an IPv6 VPN tunneled over an IPv4-only device,
	 * so we can get (vpn != NULL && best == NULL).
	 */
	if (!get_best_ip_config (self, AF_INET6, &ip_iface, &best_ac, &best, &vpn)) {
		if (nm_clear_g_object (&priv->default_device6)) {
			_LOGt (LOGD_DNS, "set-default-device-6: %p", NULL);
			_notify (self, PROP_DEFAULT_IP6_DEVICE);
		}
		return;
	}
	g_assert ((best || vpn) && best_ac);

	if (   !force_update
	    && best
	    && best == priv->default_device6)
		return;

	if (best) {
		byx_manager_for_each_active_connection (priv->manager, ac, tmp_list) {
			if (   NM_IS_VPN_CONNECTION (ac)
			    && nm_vpn_connection_get_ip6_config (NM_VPN_CONNECTION (ac))
			    && !nm_active_connection_get_device (ac))
				nm_active_connection_set_device (ac, best);
		}
	}

	if (vpn)
		best = nm_active_connection_get_device (NM_ACTIVE_CONNECTION (vpn));

	update_default_ac (self, AF_INET6, best_ac);

	if (!nm_g_object_ref_set (&priv->default_device6, best))
		return;
	_LOGt (LOGD_DNS, "set-default-device-6: %p", priv->default_device6);

	update_ip6_prefix_delegation (self);

	_LOGI (LOGD_CORE, "set '%s' (%s) as default for IPv6 routing and DNS",
	       nm_connection_get_id (nm_active_connection_get_applied_connection (best_ac)),
	       ip_iface);
	_notify (self, PROP_DEFAULT_IP6_DEVICE);
}

static void
update_ip_dns (NMPolicy *self, int addr_family)
{
	gpointer ip_config;
	const char *ip_iface = NULL;
	ByxService *vpn = NULL;

	nm_assert_addr_family (addr_family);

	ip_config = get_best_ip_config (self, addr_family, &ip_iface, NULL, NULL, &vpn);
	if (ip_config) {
		/* Tell the DNS manager this config is preferred by re-adding it with
		 * a different IP config type.
		 */
		nm_dns_manager_set_ip_config (NM_POLICY_GET_PRIVATE (self)->dns_manager,
		                              ip_config,
		                              vpn
		                                ? NM_DNS_IP_CONFIG_TYPE_VPN
		                                : NM_DNS_IP_CONFIG_TYPE_BEST_DEVICE);
	}

	if (addr_family == AF_INET6)
		update_ip6_dns_delegation (self);
}

static void
update_routing_and_dns (NMPolicy *self, gboolean force_update)
{
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);

	nm_dns_manager_begin_updates (priv->dns_manager, __func__);

	update_ip_dns (self, AF_INET);
	update_ip_dns (self, AF_INET6);

	update_ip4_routing (self, force_update);
	update_ip6_routing (self, force_update);

	nm_dns_manager_end_updates (priv->dns_manager, __func__);
}

static void
check_activating_devices (NMPolicy *self)
{
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);
	NMDevice *best4, *best6 = NULL;

	best4 = get_best_ip_device (self, AF_INET, FALSE);
	best6 = get_best_ip_device (self, AF_INET6, FALSE);

	g_object_freeze_notify (G_OBJECT (self));

	if (nm_g_object_ref_set (&priv->activating_device4, best4)) {
		_LOGt (LOGD_DNS, "set-activating-device-4: %p", priv->activating_device4);
		_notify (self, PROP_ACTIVATING_IP4_DEVICE);
	}
	if (nm_g_object_ref_set (&priv->activating_device6, best6)) {
		_LOGt (LOGD_DNS, "set-activating-device-6: %p", priv->activating_device6);
		_notify (self, PROP_ACTIVATING_IP6_DEVICE);
	}

	g_object_thaw_notify (G_OBJECT (self));
}

typedef struct {
	CList pending_lst;
	NMPolicy *policy;
	NMDevice *device;
	guint autoactivate_id;
} ActivateData;

static void
activate_data_free (ActivateData *data)
{
	nm_device_remove_pending_action (data->device, NM_PENDING_ACTION_AUTOACTIVATE, TRUE);
	c_list_unlink_stale (&data->pending_lst);
	nm_clear_g_source (&data->autoactivate_id);
	g_object_unref (data->device);
	g_slice_free (ActivateData, data);
}

static void
pending_ac_gone (gpointer data, GObject *where_the_object_was)
{
	NMPolicy *self = NM_POLICY (data);
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);

	/* Active connections should reach the DEACTIVATED state
	 * before disappearing. */
	nm_assert_not_reached();

	if (g_hash_table_remove (priv->pending_active_connections, where_the_object_was))
		g_object_unref (self);
}

static void
pending_ac_state_changed (NMActiveConnection *ac, guint state, guint reason, NMPolicy *self)
{
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);
	NMSettingsConnection *con;

	if (state >= NM_ACTIVE_CONNECTION_STATE_DEACTIVATING) {
		/* The AC is being deactivated before the device had a chance
		 * to move to PREPARE. Schedule a new auto-activation on the
		 * device, but block the current connection to avoid an activation
		 * loop.
		 */
		if (reason != NM_ACTIVE_CONNECTION_STATE_REASON_DEVICE_DISCONNECTED) {
			con = nm_active_connection_get_settings_connection (ac);
			nm_settings_connection_autoconnect_blocked_reason_set (con, NM_SETTINGS_AUTO_CONNECT_BLOCKED_REASON_FAILED, TRUE);
			schedule_activate_check (self, nm_active_connection_get_device (ac));
		}

		/* Cleanup */
		g_signal_handlers_disconnect_by_func (ac, pending_ac_state_changed, self);
		if (!g_hash_table_remove (priv->pending_active_connections, ac))
			nm_assert_not_reached ();
		g_object_weak_unref (G_OBJECT (ac), pending_ac_gone, self);
		g_object_unref (self);
	}
}

static void
auto_activate_device (NMPolicy *self,
                      NMDevice *device)
{
	NMPolicyPrivate *priv;
	NMSettingsConnection *best_connection;
	gs_free char *specific_object = NULL;
	gs_free NMSettingsConnection **connections = NULL;
	guint i, len;
	gs_free_error GError *error = NULL;
	gs_unref_object NMAuthSubject *subject = NULL;
	NMActiveConnection *ac;

	nm_assert (NM_IS_POLICY (self));
	nm_assert (NM_IS_DEVICE (device));

	priv = NM_POLICY_GET_PRIVATE (self);

	// FIXME: if a device is already activating (or activated) with a connection
	// but another connection now overrides the current one for that device,
	// deactivate the device and activate the new connection instead of just
	// bailing if the device is already active
	if (nm_device_get_act_request (device))
		return;

	if (!nm_device_autoconnect_allowed (device))
		return;

	connections = byx_manager_get_activatable_connections (priv->manager, &len, TRUE);
	if (!connections[0])
		return;

	/* Find the first connection that should be auto-activated */
	best_connection = NULL;
	for (i = 0; i < len; i++) {
		NMSettingsConnection *candidate = NM_SETTINGS_CONNECTION (connections[i]);
		NMSettingConnection *s_con;
		const char *permission;

		if (nm_settings_connection_autoconnect_is_blocked (candidate))
			continue;

		s_con = nm_connection_get_setting_connection (NM_CONNECTION (candidate));
		if (!nm_setting_connection_get_autoconnect (s_con))
			continue;

		permission = byx_utils_get_shared_wifi_permission (NM_CONNECTION (candidate));
		if (   permission
		    && !nm_settings_connection_check_permission (candidate, permission))
			continue;

		if (nm_device_can_auto_connect (device, (NMConnection *) candidate, &specific_object)) {
			best_connection = candidate;
			break;
		}
	}

	if (!best_connection)
		return;

	_LOGI (LOGD_DEVICE, "auto-activating connection '%s' (%s)",
	       nm_settings_connection_get_id (best_connection),
	       nm_settings_connection_get_uuid (best_connection));

	subject = nm_auth_subject_new_internal ();
	ac = byx_manager_activate_connection (priv->manager,
	                                     best_connection,
	                                     NULL,
	                                     specific_object,
	                                     device,
	                                     subject,
	                                     NM_ACTIVATION_TYPE_MANAGED,
	                                     NM_ACTIVATION_REASON_AUTOCONNECT,
	                                     &error);
	if (!ac) {
		_LOGI (LOGD_DEVICE, "connection '%s' auto-activation failed: %s",
		       nm_settings_connection_get_id (best_connection),
		       error->message);
		nm_settings_connection_autoconnect_blocked_reason_set (best_connection,
		                                                       NM_SETTINGS_AUTO_CONNECT_BLOCKED_REASON_FAILED,
		                                                       TRUE);
		schedule_activate_check (self, device);
		return;
	}

	/* Subscribe to AC state-changed signal to detect when the
	 * activation fails in early stages without changing device
	 * state.
	 */
	if (g_hash_table_add (priv->pending_active_connections, ac)) {
		g_signal_connect (ac, NM_ACTIVE_CONNECTION_STATE_CHANGED,
		                  G_CALLBACK (pending_ac_state_changed), g_object_ref (self));
		g_object_weak_ref (G_OBJECT (ac), (GWeakNotify) pending_ac_gone, self);
	}
}

static gboolean
auto_activate_device_cb (gpointer user_data)
{
	ActivateData *data = user_data;

	g_assert (data);
	g_assert (NM_IS_POLICY (data->policy));
	g_assert (NM_IS_DEVICE (data->device));

	data->autoactivate_id = 0;
	auto_activate_device (data->policy, data->device);
	activate_data_free (data);
	return G_SOURCE_REMOVE;
}

static ActivateData *
find_pending_activation (NMPolicy *self, NMDevice *device)
{
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);
	ActivateData *data;

	c_list_for_each_entry (data, &priv->pending_activation_checks, pending_lst) {
		if (data->device == device)
			return data;
	}
	return NULL;
}

/*****************************************************************************/

typedef struct {
	NMDevice *device;
	GSList *secondaries;
} PendingSecondaryData;

static PendingSecondaryData *
pending_secondary_data_new (NMDevice *device, GSList *secondaries)
{
	PendingSecondaryData *data;

	data = g_slice_new (PendingSecondaryData);
	data->device = g_object_ref (device);
	data->secondaries = secondaries;
	return data;
}

static void
pending_secondary_data_free (PendingSecondaryData *data)
{
	g_object_unref (data->device);
	g_slist_free_full (data->secondaries, g_object_unref);
	g_slice_free (PendingSecondaryData, data);
}

static void
process_secondaries (NMPolicy *self,
                     NMActiveConnection *active,
                     gboolean connected)
{
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);
	GSList *iter, *iter2, *next, *next2;

	/* Loop through devices waiting for secondary connections to activate */
	for (iter = priv->pending_secondaries; iter; iter = next) {
		PendingSecondaryData *secondary_data = (PendingSecondaryData *) iter->data;
		NMDevice *item_device = secondary_data->device;

		next = g_slist_next (iter);

		/* Look for 'active' in each device's secondary connections list */
		for (iter2 = secondary_data->secondaries; iter2; iter2 = next2) {
			NMActiveConnection *secondary_active = NM_ACTIVE_CONNECTION (iter2->data);

			next2 = g_slist_next (iter2);

			if (active != secondary_active)
				continue;

			if (connected) {
				_LOGD (LOGD_DEVICE, "secondary connection '%s' succeeded; active path '%s'",
				       nm_active_connection_get_settings_connection_id (active),
				       nm_dbus_object_get_path (NM_DBUS_OBJECT (active)));

				/* Secondary connection activated */
				secondary_data->secondaries = g_slist_remove (secondary_data->secondaries, secondary_active);
				g_object_unref (secondary_active);
				if (!secondary_data->secondaries) {
					/* No secondary UUID remained -> remove the secondary data item */
					priv->pending_secondaries = g_slist_remove (priv->pending_secondaries, secondary_data);
					pending_secondary_data_free (secondary_data);
					if (nm_device_get_state (item_device) == NM_DEVICE_STATE_SECONDARIES)
						nm_device_state_changed (item_device, NM_DEVICE_STATE_ACTIVATED, NM_DEVICE_STATE_REASON_NONE);
					break;
				}
			} else {
				_LOGD (LOGD_DEVICE, "secondary connection '%s' failed; active path '%s'",
				       nm_active_connection_get_settings_connection_id (active),
				       nm_dbus_object_get_path (NM_DBUS_OBJECT (active)));

				/* Secondary connection failed -> do not watch other connections */
				priv->pending_secondaries = g_slist_remove (priv->pending_secondaries, secondary_data);
				pending_secondary_data_free (secondary_data);
				if (   nm_device_get_state (item_device) == NM_DEVICE_STATE_SECONDARIES
				    || nm_device_get_state (item_device) == NM_DEVICE_STATE_ACTIVATED)
					nm_device_state_changed (item_device, NM_DEVICE_STATE_FAILED,
					                                      NM_DEVICE_STATE_REASON_SECONDARY_CONNECTION_FAILED);
				break;
			}
		}
	}
}

static gboolean
reset_autoconnect_all (NMPolicy *self,
                       NMDevice *device, /* if present, only reset connections compatible with @device */
                       gboolean only_no_secrets)
{
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);
	NMSettingsConnection *const*connections = NULL;
	guint i;
	gboolean changed = FALSE;

	_LOGD (LOGD_DEVICE, "re-enabling autoconnect for all connections%s%s%s",
	       device ? " on " : "",
	       device ? nm_device_get_iface (device) : "",
	       only_no_secrets ? " (only clear no-secrets flag)" : "");

	connections = nm_settings_get_connections (priv->settings, NULL);
	for (i = 0; connections[i]; i++) {
		NMSettingsConnection *connection = connections[i];

		if (   device
		    && !nm_device_check_connection_compatible (device, NM_CONNECTION (connection)))
			continue;

		if (only_no_secrets) {
			/* we only reset the no-secrets blocked flag. */
			if (nm_settings_connection_autoconnect_blocked_reason_set (connection,
			                                                           NM_SETTINGS_AUTO_CONNECT_BLOCKED_REASON_NO_SECRETS,
			                                                           FALSE)) {
				/* maybe the connection is still blocked afterwards for other reasons
				 * and in the larger picture nothing changed. But it's too complicated
				 * to find out exactly. Just assume, something changed to be sure. */
				if (!nm_settings_connection_autoconnect_is_blocked (connection))
					changed = TRUE;
			}
		} else {
			/* we reset the tries-count and any blocked-reason */
			if (nm_settings_connection_autoconnect_retries_get (connection) == 0)
				changed = TRUE;
			nm_settings_connection_autoconnect_retries_reset (connection);

			if (nm_settings_connection_autoconnect_blocked_reason_set (connection,
			                                                             NM_SETTINGS_AUTO_CONNECT_BLOCKED_REASON_ALL
			                                                           & ~NM_SETTINGS_AUTO_CONNECT_BLOCKED_REASON_USER_REQUEST,
			                                                           FALSE)) {
				if (!nm_settings_connection_autoconnect_is_blocked (connection))
					changed = TRUE;
			}
		}
	}
	return changed;
}

static void
sleeping_changed (ByxManager *manager, GParamSpec *pspec, gpointer user_data)
{
	NMPolicyPrivate *priv = user_data;
	NMPolicy *self = _PRIV_TO_SELF (priv);
	gboolean sleeping = FALSE, enabled = FALSE;

	g_object_get (G_OBJECT (manager), BYX_MANAGER_SLEEPING, &sleeping, NULL);
	g_object_get (G_OBJECT (manager), BYX_MANAGER_NETWORKING_ENABLED, &enabled, NULL);

	/* Reset retries on all connections so they'll checked on wakeup */
	if (sleeping || !enabled)
		reset_autoconnect_all (self, NULL, FALSE);
}

static void
schedule_activate_check (NMPolicy *self, NMDevice *device)
{
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);
	ActivateData *data;
	NMActiveConnection *ac;
	const CList *tmp_list;

	if (byx_manager_get_state (priv->manager) == NM_STATE_ASLEEP)
		return;

	if (!nm_device_autoconnect_allowed (device))
		return;

	if (find_pending_activation (self, device))
		return;

	byx_manager_for_each_active_connection (priv->manager, ac, tmp_list) {
		if (nm_active_connection_get_device (ac) == device)
			return;
	}

	nm_device_add_pending_action (device, NM_PENDING_ACTION_AUTOACTIVATE, TRUE);

	data = g_slice_new0 (ActivateData);
	data->policy = self;
	data->device = g_object_ref (device);
	data->autoactivate_id = g_idle_add (auto_activate_device_cb, data);
	c_list_link_tail (&priv->pending_activation_checks, &data->pending_lst);
}

static gboolean
reset_connections_retries (gpointer user_data)
{
	NMPolicy *self = (NMPolicy *) user_data;
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);
	NMSettingsConnection *const*connections = NULL;
	guint i;
	gint32 con_stamp, min_stamp, now;
	gboolean changed = FALSE;

	priv->reset_retries_id = 0;

	min_stamp = 0;
	now = byx_utils_get_monotonic_timestamp_s ();
	connections = nm_settings_get_connections (priv->settings, NULL);
	for (i = 0; connections[i]; i++) {
		NMSettingsConnection *connection = connections[i];

		con_stamp = nm_settings_connection_autoconnect_retries_blocked_until (connection);
		if (con_stamp == 0)
			continue;

		if (con_stamp <= now) {
			nm_settings_connection_autoconnect_retries_reset (connection);
			changed = TRUE;
		} else if (min_stamp == 0 || min_stamp > con_stamp)
			min_stamp = con_stamp;
	}

	/* Schedule the handler again if there are some stamps left */
	if (min_stamp != 0)
		priv->reset_retries_id = g_timeout_add_seconds (min_stamp - now, reset_connections_retries, self);

	/* If anything changed, try to activate the newly re-enabled connections */
	if (changed)
		schedule_activate_all (self);

	return FALSE;
}

static void
_connection_autoconnect_retries_set (NMPolicy *self,
                                     NMSettingsConnection *connection,
                                     int tries)
{
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);

	nm_assert (NM_IS_SETTINGS_CONNECTION (connection));
	nm_assert (tries >= 0);

	nm_settings_connection_autoconnect_retries_set (connection, tries);

	if (tries == 0) {
		/* Schedule a handler to reset retries count */
		if (!priv->reset_retries_id) {
			gint32 retry_time = nm_settings_connection_autoconnect_retries_blocked_until (connection);

			g_warn_if_fail (retry_time != 0);
			priv->reset_retries_id = g_timeout_add_seconds (MAX (0, retry_time - byx_utils_get_monotonic_timestamp_s ()), reset_connections_retries, self);
		}
	}
}

static void
activate_slave_connections (NMPolicy *self, NMDevice *device)
{
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);
	const char *master_device, *master_uuid_settings = NULL, *master_uuid_applied = NULL;
	guint i;
	NMActRequest *req;
	gboolean internal_activation = FALSE;
	NMSettingsConnection *const*connections;
	gboolean changed;

	master_device = nm_device_get_iface (device);
	g_assert (master_device);

	req = nm_device_get_act_request (device);
	if (req) {
		NMConnection *con;
		NMAuthSubject *subject;

		con = nm_active_connection_get_applied_connection (NM_ACTIVE_CONNECTION (req));
		if (con)
			master_uuid_applied = nm_connection_get_uuid (con);
		con = NM_CONNECTION (nm_active_connection_get_settings_connection (NM_ACTIVE_CONNECTION (req)));
		if (con) {
			master_uuid_settings = nm_connection_get_uuid (con);
			if (!g_strcmp0 (master_uuid_settings, master_uuid_applied))
				master_uuid_settings = NULL;
		}

		subject = nm_active_connection_get_subject (NM_ACTIVE_CONNECTION (req));
		internal_activation = subject && nm_auth_subject_is_internal (subject);
	}

	changed = FALSE;
	connections = nm_settings_get_connections (priv->settings, NULL);
	for (i = 0; connections[i]; i++) {
		NMSettingsConnection *connection = connections[i];
		NMSettingConnection *s_slave_con;
		const char *slave_master;

		s_slave_con = nm_connection_get_setting_connection (NM_CONNECTION (connection));
		slave_master = nm_setting_connection_get_master (s_slave_con);
		if (!slave_master)
			continue;
		if (!NM_IN_STRSET (slave_master, master_device,
		                                 master_uuid_applied,
		                                 master_uuid_settings))
			continue;

		if (!internal_activation) {
			if (nm_settings_connection_autoconnect_retries_get (connection) == 0)
				changed = TRUE;
			nm_settings_connection_autoconnect_retries_reset (connection);
		}
		if (nm_settings_connection_autoconnect_blocked_reason_set (connection,
		                                                           NM_SETTINGS_AUTO_CONNECT_BLOCKED_REASON_FAILED,
		                                                           FALSE)) {
			if (!nm_settings_connection_autoconnect_is_blocked (connection))
				changed = TRUE;
		}
	}

	if (changed)
		schedule_activate_all (self);
}

static gboolean
activate_secondary_connections (NMPolicy *self,
                                NMConnection *connection,
                                NMDevice *device)
{
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);
	NMSettingConnection *s_con;
	NMSettingsConnection *settings_con;
	NMActiveConnection *ac;
	PendingSecondaryData *secondary_data;
	GSList *secondary_ac_list = NULL;
	GError *error = NULL;
	guint32 i;
	gboolean success = TRUE;

	s_con = nm_connection_get_setting_connection (connection);
	g_assert (s_con);

	for (i = 0; i < nm_setting_connection_get_num_secondaries (s_con); i++) {
		const char *sec_uuid = nm_setting_connection_get_secondary (s_con, i);
		NMActRequest *req;

		settings_con = nm_settings_get_connection_by_uuid (priv->settings, sec_uuid);
		if (!settings_con) {
			_LOGW (LOGD_DEVICE, "secondary connection '%s' auto-activation failed: The connection doesn't exist.",
			       sec_uuid);
			success = FALSE;
			break;
		}
		if (!nm_connection_is_type (NM_CONNECTION (settings_con), NM_SETTING_VPN_SETTING_NAME)) {
			_LOGW (LOGD_DEVICE, "secondary connection '%s (%s)' auto-activation failed: The connection is not a VPN.",
			       nm_settings_connection_get_id (settings_con), sec_uuid);
			success = FALSE;
			break;
		}

		req = nm_device_get_act_request (device);
		g_assert (req);

		_LOGD (LOGD_DEVICE, "activating secondary connection '%s (%s)' for base connection '%s (%s)'",
		       nm_settings_connection_get_id (settings_con), sec_uuid,
		       nm_connection_get_id (connection), nm_connection_get_uuid (connection));
		ac = byx_manager_activate_connection (priv->manager,
		                                     settings_con,
		                                     NULL,
		                                     nm_dbus_object_get_path (NM_DBUS_OBJECT (req)),
		                                     device,
		                                     nm_active_connection_get_subject (NM_ACTIVE_CONNECTION (req)),
		                                     NM_ACTIVATION_TYPE_MANAGED,
		                                     nm_active_connection_get_activation_reason (NM_ACTIVE_CONNECTION (req)),
		                                     &error);
		if (ac)
			secondary_ac_list = g_slist_append (secondary_ac_list, g_object_ref (ac));
		else {
			_LOGW (LOGD_DEVICE, "secondary connection '%s (%s)' auto-activation failed: (%d) %s",
			       nm_settings_connection_get_id (settings_con), sec_uuid,
			       error->code,
			       error->message);
			g_clear_error (&error);
			success = FALSE;
			break;
		}
	}

	if (success && secondary_ac_list != NULL) {
		secondary_data = pending_secondary_data_new (device, secondary_ac_list);
		priv->pending_secondaries = g_slist_append (priv->pending_secondaries, secondary_data);
	} else
		g_slist_free_full (secondary_ac_list, g_object_unref);

	return success;
}

static void
device_state_changed (NMDevice *device,
                      NMDeviceState new_state,
                      NMDeviceState old_state,
                      NMDeviceStateReason reason,
                      gpointer user_data)
{
	NMPolicyPrivate *priv = user_data;
	NMPolicy *self = _PRIV_TO_SELF (priv);
	NMActiveConnection *ac;
	NMSettingsConnection *connection = nm_device_get_settings_connection (device);
	NMIP4Config *ip4_config;
	NMIP6Config *ip6_config;
	NMSettingConnection *s_con = NULL;

	switch (nm_device_state_reason_check (reason)) {
	case NM_DEVICE_STATE_REASON_GSM_REGISTRATION_DENIED:
	case NM_DEVICE_STATE_REASON_GSM_REGISTRATION_NOT_SEARCHING:
	case NM_DEVICE_STATE_REASON_GSM_SIM_NOT_INSERTED:
	case NM_DEVICE_STATE_REASON_GSM_SIM_PIN_REQUIRED:
	case NM_DEVICE_STATE_REASON_GSM_SIM_PUK_REQUIRED:
	case NM_DEVICE_STATE_REASON_GSM_SIM_WRONG:
	case NM_DEVICE_STATE_REASON_SIM_PIN_INCORRECT:
	case NM_DEVICE_STATE_REASON_MODEM_INIT_FAILED:
	case NM_DEVICE_STATE_REASON_GSM_APN_FAILED:
		/* Block autoconnect of the just-failed connection for situations
		 * where a retry attempt would just fail again.
		 */
		if (connection) {
			nm_settings_connection_autoconnect_blocked_reason_set (connection,
			                                                       NM_SETTINGS_AUTO_CONNECT_BLOCKED_REASON_FAILED,
			                                                       TRUE);
		}
		break;
	default:
		break;
	}

	switch (new_state) {
	case NM_DEVICE_STATE_FAILED:
		/* Mark the connection invalid if it failed during activation so that
		 * it doesn't get automatically chosen over and over and over again.
		 */
		if (   connection
		    && old_state >= NM_DEVICE_STATE_PREPARE
		    && old_state <= NM_DEVICE_STATE_ACTIVATED) {
			gboolean block_no_secrets = FALSE;
			int tries;
			guint64 con_v;

			if (nm_device_state_reason_check (reason) == NM_DEVICE_STATE_REASON_NO_SECRETS) {
				/* we want to block the connection from auto-connect if it failed due to no-secrets.
				 * However, if a secret-agent registered, since the connection made the last
				 * secret-request, we do not block it. The new secret-agent might not yet
				 * been consulted, and it may be able to provide the secrets.
				 *
				 * We detect this by using a version-id of the agent-manager, which increments
				 * whenever new agents register. Note that the agent-manager's version-id is
				 * never zero and strictly increasing.
				 *
				 * A connection's version-id of zero means that the connection never tried to request secrets.
				 * That can happen when nm_settings_connection_get_secrets() fails early without actually
				 * consulting any agents.
				 */
				con_v = nm_settings_connection_get_last_secret_agent_version_id (connection);
				if (   con_v == 0
				    || con_v == nm_agent_manager_get_agent_version_id (priv->agent_mgr))
					block_no_secrets = TRUE;
			}

			if (block_no_secrets) {
				_LOGD (LOGD_DEVICE, "connection '%s' now blocked from autoconnect due to no secrets",
				       nm_settings_connection_get_id (connection));
				nm_settings_connection_autoconnect_blocked_reason_set (connection, NM_SETTINGS_AUTO_CONNECT_BLOCKED_REASON_NO_SECRETS, TRUE);
			} else {
				tries = nm_settings_connection_autoconnect_retries_get (connection);
				if (tries > 0) {
					_LOGD (LOGD_DEVICE, "connection '%s' failed to autoconnect; %d tries left",
					       nm_settings_connection_get_id (connection), tries - 1);
					_connection_autoconnect_retries_set (self, connection, tries - 1);
				} else if (tries != 0) {
					_LOGD (LOGD_DEVICE, "connection '%s' failed to autoconnect; infinite tries left",
					       nm_settings_connection_get_id (connection));
				}
			}

			nm_connection_clear_secrets (NM_CONNECTION (connection));
		}
		break;
	case NM_DEVICE_STATE_ACTIVATED:
		if (connection) {
			/* Reset auto retries back to default since connection was successful */
			nm_settings_connection_autoconnect_retries_reset (connection);

			/* And clear secrets so they will always be requested from the
			 * settings service when the next connection is made.
			 */

			nm_connection_clear_secrets (NM_CONNECTION (connection));
		}

		/* Add device's new IPv4 and IPv6 configs to DNS */

		nm_dns_manager_begin_updates (priv->dns_manager, __func__);

		ip4_config = nm_device_get_ip4_config (device);
		if (ip4_config)
			nm_dns_manager_set_ip_config (priv->dns_manager, NM_IP_CONFIG_CAST (ip4_config), NM_DNS_IP_CONFIG_TYPE_DEFAULT);
		ip6_config = nm_device_get_ip6_config (device);
		if (ip6_config)
			nm_dns_manager_set_ip_config (priv->dns_manager, NM_IP_CONFIG_CAST (ip6_config), NM_DNS_IP_CONFIG_TYPE_DEFAULT);

		update_routing_and_dns (self, FALSE);

		nm_dns_manager_end_updates (priv->dns_manager, __func__);
		break;
	case NM_DEVICE_STATE_UNMANAGED:
	case NM_DEVICE_STATE_UNAVAILABLE:
		if (old_state > NM_DEVICE_STATE_DISCONNECTED)
			update_routing_and_dns (self, FALSE);
		break;
	case NM_DEVICE_STATE_DEACTIVATING:
		if (connection) {
			NMSettingsAutoconnectBlockedReason blocked_reason = NM_SETTINGS_AUTO_CONNECT_BLOCKED_REASON_NONE;

			switch (nm_device_state_reason_check (reason)) {
			case NM_DEVICE_STATE_REASON_USER_REQUESTED:
				 blocked_reason = NM_SETTINGS_AUTO_CONNECT_BLOCKED_REASON_USER_REQUEST;
				break;
			case NM_DEVICE_STATE_REASON_DEPENDENCY_FAILED:
				blocked_reason = NM_SETTINGS_AUTO_CONNECT_BLOCKED_REASON_FAILED;
				break;
			default:
				break;
			}
			if (blocked_reason != NM_SETTINGS_AUTO_CONNECT_BLOCKED_REASON_NONE) {
				_LOGD (LOGD_DEVICE, "blocking autoconnect of connection '%s': %s",
				       nm_settings_connection_get_id (connection),
				       NM_UTILS_LOOKUP_STR (nm_device_state_reason_to_str,
				                            nm_device_state_reason_check (reason)));
				nm_settings_connection_autoconnect_blocked_reason_set (connection, blocked_reason, TRUE);
			}
		}
		ip6_remove_device_prefix_delegations (self, device);
		break;
	case NM_DEVICE_STATE_DISCONNECTED:
		/* Reset retry counts for a device's connections when carrier on; if cable
		 * was unplugged and plugged in again, we should try to reconnect.
		 */
		if (   nm_device_state_reason_check (reason) == NM_DEVICE_STATE_REASON_CARRIER
		    && old_state == NM_DEVICE_STATE_UNAVAILABLE)
			reset_autoconnect_all (self, device, FALSE);

		if (old_state > NM_DEVICE_STATE_DISCONNECTED)
			update_routing_and_dns (self, FALSE);

		/* Device is now available for auto-activation */
		schedule_activate_check (self, device);
		break;

	case NM_DEVICE_STATE_PREPARE:
		/* Reset auto-connect retries of all slaves and schedule them for
		 * activation. */
		activate_slave_connections (self, device);

		/* Now that the device state is progressing, we don't care
		 * anymore for the AC state. */
		ac = (NMActiveConnection *) nm_device_get_act_request (device);
		if (ac && g_hash_table_remove (priv->pending_active_connections, ac)) {
			g_signal_handlers_disconnect_by_func (ac, pending_ac_state_changed, self);
			g_object_weak_unref (G_OBJECT (ac), pending_ac_gone, self);
			g_object_unref (self);
		}
		break;
	case NM_DEVICE_STATE_IP_CONFIG:
		/* We must have secrets if we got here. */
		if (connection)
			nm_settings_connection_autoconnect_blocked_reason_set (connection, NM_SETTINGS_AUTO_CONNECT_BLOCKED_REASON_ALL, FALSE);
		break;
	case NM_DEVICE_STATE_SECONDARIES:
		if (connection)
			s_con = nm_connection_get_setting_connection (NM_CONNECTION (connection));
		if (s_con && nm_setting_connection_get_num_secondaries (s_con) > 0) {
			/* Make routes and DNS up-to-date before activating dependent connections */
			update_routing_and_dns (self, FALSE);

			/* Activate secondary (VPN) connections */
			if (!activate_secondary_connections (self, NM_CONNECTION (connection), device))
				nm_device_queue_state (device, NM_DEVICE_STATE_FAILED,
				                       NM_DEVICE_STATE_REASON_SECONDARY_CONNECTION_FAILED);
		} else
			nm_device_queue_state (device, NM_DEVICE_STATE_ACTIVATED,
			                       NM_DEVICE_STATE_REASON_NONE);
		break;

	default:
		break;
	}

	check_activating_devices (self);
}

static void
device_ip_config_changed (NMDevice *device,
                          NMIPConfig *new_config,
                          NMIPConfig *old_config,
                          gpointer user_data)
{
	NMPolicyPrivate *priv = user_data;
	NMPolicy *self = _PRIV_TO_SELF (priv);
	int addr_family;

	nm_assert (new_config || old_config);
	nm_assert (!new_config || NM_IS_IP_CONFIG (new_config, AF_UNSPEC));
	nm_assert (!old_config || NM_IS_IP_CONFIG (old_config, AF_UNSPEC));

	if (new_config) {
		addr_family = nm_ip_config_get_addr_family (new_config);
		nm_assert (!old_config || addr_family == nm_ip_config_get_addr_family (old_config));
	} else
		addr_family = nm_ip_config_get_addr_family (old_config);

	nm_dns_manager_begin_updates (priv->dns_manager, __func__);

	/* We catch already all the IP events registering on the device state changes but
	 * the ones where the IP changes but the device state keep stable (i.e., activated):
	 * ignore IP config changes but when the device is in activated state.
	 * Prevents unecessary changes to DNS information.
	 */
	if (nm_device_get_state (device) == NM_DEVICE_STATE_ACTIVATED) {
		if (old_config != new_config) {
			if (new_config)
				nm_dns_manager_set_ip_config (priv->dns_manager, new_config, NM_DNS_IP_CONFIG_TYPE_DEFAULT);
			if (old_config)
				nm_dns_manager_set_ip_config (priv->dns_manager, old_config, NM_DNS_IP_CONFIG_TYPE_REMOVED);
		}
		update_ip_dns (self, addr_family);
		if (addr_family == AF_INET)
			update_ip4_routing (self, TRUE);
		else
			update_ip6_routing (self, TRUE);
	} else {
		/* Old configs get removed immediately */
		if (old_config)
			nm_dns_manager_set_ip_config (priv->dns_manager, old_config, NM_DNS_IP_CONFIG_TYPE_REMOVED);
	}

	nm_dns_manager_end_updates (priv->dns_manager, __func__);
}

/*****************************************************************************/

static void
device_autoconnect_changed (NMDevice *device,
                            GParamSpec *pspec,
                            gpointer user_data)
{
	NMPolicyPrivate *priv = user_data;
	NMPolicy *self = _PRIV_TO_SELF (priv);

	schedule_activate_check (self, device);
}

static void
device_recheck_auto_activate (NMDevice *device, gpointer user_data)
{
	NMPolicyPrivate *priv = user_data;
	NMPolicy *self = _PRIV_TO_SELF (priv);

	schedule_activate_check (self, device);
}

static void
devices_list_unregister (NMPolicy *self, NMDevice *device)
{
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);

	g_signal_handlers_disconnect_by_data ((GObject *) device, priv);
}

static void
devices_list_register (NMPolicy *self, NMDevice *device)
{
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);

	/* Connect state-changed with _after, so that the handler is invoked after other handlers. */
	g_signal_connect_after (device, NM_DEVICE_STATE_CHANGED,          (GCallback) device_state_changed, priv);
	g_signal_connect       (device, NM_DEVICE_IP4_CONFIG_CHANGED,     (GCallback) device_ip_config_changed, priv);
	g_signal_connect       (device, NM_DEVICE_IP6_CONFIG_CHANGED,     (GCallback) device_ip_config_changed, priv);
	g_signal_connect       (device, NM_DEVICE_IP6_PREFIX_DELEGATED,   (GCallback) device_ip6_prefix_delegated, priv);
	g_signal_connect       (device, NM_DEVICE_IP6_SUBNET_NEEDED,      (GCallback) device_ip6_subnet_needed, priv);
	g_signal_connect       (device, "notify::" NM_DEVICE_AUTOCONNECT, (GCallback) device_autoconnect_changed, priv);
	g_signal_connect       (device, NM_DEVICE_RECHECK_AUTO_ACTIVATE,  (GCallback) device_recheck_auto_activate, priv);
}

static void
device_added (ByxManager *manager, NMDevice *device, gpointer user_data)
{
	NMPolicyPrivate *priv = user_data;
	NMPolicy *self = _PRIV_TO_SELF (priv);

	g_return_if_fail (NM_IS_POLICY (self));

	priv = NM_POLICY_GET_PRIVATE (self);

	if (!g_hash_table_add (priv->devices, device))
		g_return_if_reached ();

	devices_list_register (self, device);
}

static void
device_removed (ByxManager *manager, NMDevice *device, gpointer user_data)
{
	NMPolicyPrivate *priv = user_data;
	NMPolicy *self = _PRIV_TO_SELF (priv);
	ActivateData *data;

	/* XXX is this needed? The delegations are cleaned up
	 * on transition to deactivated too. */
	ip6_remove_device_prefix_delegations (self, device);

	/* Clear any idle callbacks for this device */
	data = find_pending_activation (self, device);
	if (data && data->autoactivate_id)
		activate_data_free (data);

	if (g_hash_table_remove (priv->devices, device))
		devices_list_unregister (self, device);

	/* Don't update routing and DNS here as we've already handled that
	 * for devices that need it when the device's state changed to UNMANAGED.
	 */
}

/*****************************************************************************/

static void
vpn_connection_activated (NMPolicy *self, ByxService *vpn)
{
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);
	NMIP4Config *ip4_config;
	NMIP6Config *ip6_config;

	nm_dns_manager_begin_updates (priv->dns_manager, __func__);

	ip4_config = nm_vpn_connection_get_ip4_config (vpn);
	if (ip4_config)
		nm_dns_manager_set_ip_config (priv->dns_manager, NM_IP_CONFIG_CAST (ip4_config), NM_DNS_IP_CONFIG_TYPE_VPN);

	ip6_config = nm_vpn_connection_get_ip6_config (vpn);
	if (ip6_config)
		nm_dns_manager_set_ip_config (priv->dns_manager, NM_IP_CONFIG_CAST (ip6_config), NM_DNS_IP_CONFIG_TYPE_VPN);

	update_routing_and_dns (self, TRUE);

	nm_dns_manager_end_updates (priv->dns_manager, __func__);
}

static void
vpn_connection_deactivated (NMPolicy *self, ByxService *vpn)
{
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);
	NMIP4Config *ip4_config;
	NMIP6Config *ip6_config;

	nm_dns_manager_begin_updates (priv->dns_manager, __func__);

	ip4_config = nm_vpn_connection_get_ip4_config (vpn);
	if (ip4_config)
		nm_dns_manager_set_ip_config (priv->dns_manager, NM_IP_CONFIG_CAST (ip4_config), NM_DNS_IP_CONFIG_TYPE_REMOVED);

	ip6_config = nm_vpn_connection_get_ip6_config (vpn);
	if (ip6_config)
		nm_dns_manager_set_ip_config (priv->dns_manager, NM_IP_CONFIG_CAST (ip6_config), NM_DNS_IP_CONFIG_TYPE_REMOVED);

	update_routing_and_dns (self, TRUE);

	nm_dns_manager_end_updates (priv->dns_manager, __func__);
}

static void
vpn_connection_state_changed (ByxService *vpn,
                              NMVpnConnectionState new_state,
                              NMVpnConnectionState old_state,
                              NMActiveConnectionStateReason reason,
                              NMPolicy *self)
{
	if (new_state == NM_VPN_CONNECTION_STATE_ACTIVATED)
		vpn_connection_activated (self, vpn);
	else if (new_state >= NM_VPN_CONNECTION_STATE_FAILED) {
		/* Only clean up IP/DNS if the connection ever got past IP_CONFIG */
		if (old_state >= NM_VPN_CONNECTION_STATE_IP_CONFIG_GET &&
		    old_state <= NM_VPN_CONNECTION_STATE_ACTIVATED)
			vpn_connection_deactivated (self, vpn);
	}
}

static void
vpn_connection_retry_after_failure (ByxService *vpn, NMPolicy *self)
{
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);
	NMActiveConnection *ac = NM_ACTIVE_CONNECTION (vpn);
	NMSettingsConnection *connection = nm_active_connection_get_settings_connection (ac);
	GError *error = NULL;

	/* Attempt to reconnect VPN connections that failed after being connected */
	if (!byx_manager_activate_connection (priv->manager,
	                                     connection,
	                                     NULL,
	                                     NULL,
	                                     NULL,
	                                     nm_active_connection_get_subject (ac),
	                                     NM_ACTIVATION_TYPE_MANAGED,
	                                     nm_active_connection_get_activation_reason (ac),
	                                     &error)) {
		_LOGW (LOGD_DEVICE, "VPN '%s' reconnect failed: %s",
		       nm_settings_connection_get_id (connection),
		       error->message ?: "unknown");
		g_clear_error (&error);
	}
}

static void
active_connection_state_changed (NMActiveConnection *active,
                                 GParamSpec *pspec,
                                 NMPolicy *self)
{
	NMActiveConnectionState state = nm_active_connection_get_state (active);

	if (state == NM_ACTIVE_CONNECTION_STATE_ACTIVATED)
		process_secondaries (self, active, TRUE);
	else if (state == NM_ACTIVE_CONNECTION_STATE_DEACTIVATED)
		process_secondaries (self, active, FALSE);
}

static void
active_connection_added (ByxManager *manager,
                         NMActiveConnection *active,
                         gpointer user_data)
{
	NMPolicyPrivate *priv = user_data;
	NMPolicy *self = _PRIV_TO_SELF (priv);

	if (NM_IS_VPN_CONNECTION (active)) {
		g_signal_connect (active, NM_VPN_CONNECTION_INTERNAL_STATE_CHANGED,
		                  G_CALLBACK (vpn_connection_state_changed),
		                  self);
		g_signal_connect (active, NM_VPN_CONNECTION_INTERNAL_RETRY_AFTER_FAILURE,
		                  G_CALLBACK (vpn_connection_retry_after_failure),
		                  self);
	}

	g_signal_connect (active, "notify::" NM_ACTIVE_CONNECTION_STATE,
	                  G_CALLBACK (active_connection_state_changed),
	                  self);
}

static void
active_connection_removed (ByxManager *manager,
                           NMActiveConnection *active,
                           gpointer user_data)
{
	NMPolicyPrivate *priv = user_data;
	NMPolicy *self = _PRIV_TO_SELF (priv);

	g_signal_handlers_disconnect_by_func (active,
	                                      vpn_connection_state_changed,
	                                      self);
	g_signal_handlers_disconnect_by_func (active,
	                                      vpn_connection_retry_after_failure,
	                                      self);
	g_signal_handlers_disconnect_by_func (active,
	                                      active_connection_state_changed,
	                                      self);
}

/*****************************************************************************/

static gboolean
schedule_activate_all_cb (gpointer user_data)
{
	NMPolicy *self = user_data;
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);
	const CList *tmp_lst;
	NMDevice *device;

	priv->schedule_activate_all_id = 0;

	byx_manager_for_each_device (priv->manager, device, tmp_lst)
		schedule_activate_check (self, device);

	return G_SOURCE_REMOVE;
}

static void
schedule_activate_all (NMPolicy *self)
{
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);

	/* always restart the idle handler. That way, we settle
	 * all other events before restarting to activate them. */
	nm_clear_g_source (&priv->schedule_activate_all_id);
	priv->schedule_activate_all_id = g_idle_add (schedule_activate_all_cb, self);
}

static void
connection_added (NMSettings *settings,
                  NMSettingsConnection *connection,
                  gpointer user_data)
{
	NMPolicyPrivate *priv = user_data;
	NMPolicy *self = _PRIV_TO_SELF (priv);

	schedule_activate_all (self);
}

static void
firewall_state_changed (NMFirewallManager *manager,
                        gboolean initialized_now,
                        gpointer user_data)
{
	NMPolicy *self = (NMPolicy *) user_data;
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);
	const CList *tmp_lst;
	NMDevice *device;

	if (initialized_now) {
		/* the firewall manager was initializing, but all requests
		 * so fare were queued and are already sent. No need to
		 * re-update the firewall zone of the devices. */
		return;
	}

	if (!nm_firewall_manager_get_running (manager))
		return;

	/* add interface of each device to correct zone */
	byx_manager_for_each_device (priv->manager, device, tmp_lst)
		nm_device_update_firewall_zone (device);
}

static void
dns_config_changed (NMDnsManager *dns_manager, gpointer user_data)
{
	NMPolicy *self = (NMPolicy *) user_data;
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);
}

static void
connection_updated (NMSettings *settings,
                    NMSettingsConnection *connection,
                    gboolean by_user,
                    gpointer user_data)
{
	NMPolicyPrivate *priv = user_data;
	NMPolicy *self = _PRIV_TO_SELF (priv);
	const CList *tmp_lst;
	NMDevice *device = NULL;
	NMDevice *dev;

	if (by_user) {
		/* find device with given connection */
		byx_manager_for_each_device (priv->manager, dev, tmp_lst) {
			if (nm_device_get_settings_connection (dev) == connection) {
				device = dev;
				break;
			}
		}

		if (device)
			nm_device_reapply_settings_immediately (device);

		/* Reset auto retries back to default since connection was updated */
		nm_settings_connection_autoconnect_retries_reset (connection);
	}

	schedule_activate_all (self);
}

static void
_deactivate_if_active (NMPolicy *self, NMSettingsConnection *connection)
{
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);
	NMActiveConnection *ac;
	const CList *tmp_list;
	GError *error = NULL;

	nm_assert (NM_IS_SETTINGS_CONNECTION (connection));

	byx_manager_for_each_active_connection (priv->manager, ac, tmp_list) {

		if (   nm_active_connection_get_settings_connection (ac) == connection
		    && (nm_active_connection_get_state (ac) <= NM_ACTIVE_CONNECTION_STATE_ACTIVATED)) {
			if (!byx_manager_deactivate_connection (priv->manager,
			                                       ac,
			                                       NM_DEVICE_STATE_REASON_CONNECTION_REMOVED,
			                                       &error)) {
				_LOGW (LOGD_DEVICE, "connection '%s' disappeared, but error deactivating it: (%d) %s",
				       nm_settings_connection_get_id (connection),
				       error ? error->code : -1,
				       error ? error->message : "(unknown)");
				g_clear_error (&error);
			}
		}
	}
}

static void
connection_removed (NMSettings *settings,
                    NMSettingsConnection *connection,
                    gpointer user_data)
{
	NMPolicyPrivate *priv = user_data;
	NMPolicy *self = _PRIV_TO_SELF (priv);

	_deactivate_if_active (self, connection);
}

static void
connection_flags_changed (NMSettings *settings,
                          NMSettingsConnection *connection,
                          gpointer user_data)
{
	NMPolicyPrivate *priv = user_data;
	NMPolicy *self = _PRIV_TO_SELF (priv);

	if (NM_FLAGS_HAS (nm_settings_connection_get_flags (connection),
	                  NM_SETTINGS_CONNECTION_INT_FLAGS_VISIBLE)) {
		if (!nm_settings_connection_autoconnect_is_blocked (connection))
			schedule_activate_all (self);
	} else
		_deactivate_if_active (self, connection);
}

static void
secret_agent_registered (NMSettings *settings,
                         NMSecretAgent *agent,
                         gpointer user_data)
{
	NMPolicy *self = NM_POLICY (user_data);

	/* The registered secret agent may provide some missing secrets. Thus we
	 * reset retries count here and schedule activation, so that the
	 * connections failed due to missing secrets may re-try auto-connection.
	 */
	if (reset_autoconnect_all (self, NULL, TRUE))
		schedule_activate_all (self);
}

NMDevice *
nm_policy_get_default_ip4_device (NMPolicy *self)
{
	return NM_POLICY_GET_PRIVATE (self)->default_device4;
}

NMDevice *
nm_policy_get_default_ip6_device (NMPolicy *self)
{
	return NM_POLICY_GET_PRIVATE (self)->default_device6;
}

NMDevice *
nm_policy_get_activating_ip4_device (NMPolicy *self)
{
	return NM_POLICY_GET_PRIVATE (self)->activating_device4;
}

NMDevice *
nm_policy_get_activating_ip6_device (NMPolicy *self)
{
	return NM_POLICY_GET_PRIVATE (self)->activating_device6;
}

/*****************************************************************************/

static void
get_property (GObject *object, guint prop_id,
              GValue *value, GParamSpec *pspec)
{
	NMPolicy *self = NM_POLICY (object);
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);

	switch (prop_id) {
	case PROP_DEFAULT_IP4_DEVICE:
		g_value_set_object (value, priv->default_device4);
		break;
	case PROP_DEFAULT_IP6_DEVICE:
		g_value_set_object (value, priv->default_device6);
		break;
	case PROP_ACTIVATING_IP4_DEVICE:
		g_value_set_object (value, priv->activating_device4);
		break;
	case PROP_ACTIVATING_IP6_DEVICE:
		g_value_set_object (value, priv->activating_device6);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
set_property (GObject *object, guint prop_id,
              const GValue *value, GParamSpec *pspec)
{
	NMPolicy *self = NM_POLICY (object);
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);

	switch (prop_id) {
	case PROP_MANAGER:
		/* construct-only */
		priv->manager = g_value_get_object (value);
		g_return_if_fail (BYX_IS_MANAGER (priv->manager));
		break;
	case PROP_SETTINGS:
		/* construct-only */
		priv->settings = g_value_dup_object (value);
		g_return_if_fail (NM_IS_SETTINGS (priv->settings));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

/*****************************************************************************/

static void
nm_policy_init (NMPolicy *self)
{
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);

	c_list_init (&priv->pending_activation_checks);

	priv->netns = g_object_ref (nm_netns_get ());

	priv->devices = g_hash_table_new (nm_direct_hash, NULL);
	priv->pending_active_connections = g_hash_table_new (nm_direct_hash, NULL);
	priv->ip6_prefix_delegations = g_array_new (FALSE, FALSE, sizeof (IP6PrefixDelegation));
	g_array_set_clear_func (priv->ip6_prefix_delegations, clear_ip6_prefix_delegation);
}

static void
constructed (GObject *object)
{
	NMPolicy *self = NM_POLICY (object);
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);
	char *hostname = NULL;

	priv->agent_mgr = g_object_ref (nm_agent_manager_get ());

	priv->firewall_manager = g_object_ref (nm_firewall_manager_get ());
	g_signal_connect (priv->firewall_manager, NM_FIREWALL_MANAGER_STATE_CHANGED,
	                  G_CALLBACK (firewall_state_changed), self);

	priv->dns_manager = g_object_ref (nm_dns_manager_get ());
	nm_dns_manager_set_initial_hostname (priv->dns_manager, gethostname());
	priv->config_changed_id = g_signal_connect (priv->dns_manager, NM_DNS_MANAGER_CONFIG_CHANGED,
	                                            G_CALLBACK (dns_config_changed), self);

	g_signal_connect (priv->manager, "notify::" BYX_MANAGER_SLEEPING,           (GCallback) sleeping_changed, priv);
	g_signal_connect (priv->manager, "notify::" BYX_MANAGER_NETWORKING_ENABLED, (GCallback) sleeping_changed, priv);
	g_signal_connect (priv->manager, BYX_MANAGER_INTERNAL_DEVICE_ADDED,         (GCallback) device_added, priv);
	g_signal_connect (priv->manager, BYX_MANAGER_INTERNAL_DEVICE_REMOVED,       (GCallback) device_removed, priv);
	g_signal_connect (priv->manager, BYX_MANAGER_ACTIVE_CONNECTION_ADDED,       (GCallback) active_connection_added, priv);
	g_signal_connect (priv->manager, BYX_MANAGER_ACTIVE_CONNECTION_REMOVED,     (GCallback) active_connection_removed, priv);

	g_signal_connect (priv->settings, NM_SETTINGS_SIGNAL_CONNECTION_ADDED,         (GCallback) connection_added, priv);
	g_signal_connect (priv->settings, NM_SETTINGS_SIGNAL_CONNECTION_UPDATED,       (GCallback) connection_updated, priv);
	g_signal_connect (priv->settings, NM_SETTINGS_SIGNAL_CONNECTION_REMOVED,       (GCallback) connection_removed, priv);
	g_signal_connect (priv->settings, NM_SETTINGS_SIGNAL_CONNECTION_FLAGS_CHANGED, (GCallback) connection_flags_changed, priv);

	g_signal_connect (priv->agent_mgr, NM_AGENT_MANAGER_AGENT_REGISTERED, G_CALLBACK (secret_agent_registered), self);

	G_OBJECT_CLASS (nm_policy_parent_class)->constructed (object);
}

NMPolicy *
nm_policy_new (ByxManager *manager, NMSettings *settings)
{
	g_return_val_if_fail (BYX_IS_MANAGER (manager), NULL);
	g_return_val_if_fail (NM_IS_SETTINGS (settings), NULL);

	return g_object_new (NM_TYPE_POLICY,
	                     NM_POLICY_MANAGER, manager,
	                     NM_POLICY_SETTINGS, settings,
	                     NULL);
}

static void
dispose (GObject *object)
{
	NMPolicy *self = NM_POLICY (object);
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);
	GHashTableIter h_iter;
	NMDevice *device;
	ActivateData *data, *data_safe;

	nm_clear_g_object (&priv->default_device4);
	nm_clear_g_object (&priv->default_device6);
	nm_clear_g_object (&priv->activating_device4);
	nm_clear_g_object (&priv->activating_device6);
	g_clear_pointer (&priv->pending_active_connections, g_hash_table_unref);

	c_list_for_each_entry_safe (data, data_safe, &priv->pending_activation_checks, pending_lst)
		activate_data_free (data);

	g_slist_free_full (priv->pending_secondaries, (GDestroyNotify) pending_secondary_data_free);
	priv->pending_secondaries = NULL;

	if (priv->firewall_manager) {
		g_signal_handlers_disconnect_by_func (priv->firewall_manager, firewall_state_changed, self);
		g_clear_object (&priv->firewall_manager);
	}

	if (priv->agent_mgr) {
		g_signal_handlers_disconnect_by_func (priv->agent_mgr, secret_agent_registered, self);
		g_clear_object (&priv->agent_mgr);
	}

	if (priv->dns_manager) {
		nm_clear_g_signal_handler (priv->dns_manager, &priv->config_changed_id);
		g_clear_object (&priv->dns_manager);
	}

	g_hash_table_iter_init (&h_iter, priv->devices);
	if (g_hash_table_iter_next (&h_iter, (gpointer *) &device, NULL)) {
		g_hash_table_iter_remove (&h_iter);
		devices_list_unregister (self, device);
	}

	/* The manager should have disposed of ActiveConnections already, which
	 * will have called active_connection_removed() and thus we don't need
	 * to clean anything up.  Assert that this is TRUE.
	 */
	nm_assert (c_list_is_empty (byx_manager_get_active_connections (priv->manager)));

	nm_clear_g_source (&priv->reset_retries_id);
	nm_clear_g_source (&priv->schedule_activate_all_id);

	if (priv->settings) {
		g_signal_handlers_disconnect_by_data (priv->settings, priv);
		g_clear_object (&priv->settings);

		/* we don't clear priv->manager as we don't own a reference to it,
		 * that is, ByxManager must outlive NMPolicy anyway.
		 *
		 * Hence, we unsubscribe the signals here together with the signals
		 * for settings. */
		g_signal_handlers_disconnect_by_data (priv->manager, priv);
	}

	if (priv->ip6_prefix_delegations) {
		g_array_free (priv->ip6_prefix_delegations, TRUE);
		priv->ip6_prefix_delegations = NULL;
	}

	nm_assert (BYX_IS_MANAGER (priv->manager));

	G_OBJECT_CLASS (nm_policy_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
	NMPolicy *self = NM_POLICY (object);
	NMPolicyPrivate *priv = NM_POLICY_GET_PRIVATE (self);

	g_hash_table_unref (priv->devices);

	G_OBJECT_CLASS (nm_policy_parent_class)->finalize (object);

	g_object_unref (priv->netns);
}

static void
nm_policy_class_init (NMPolicyClass *policy_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (policy_class);

	object_class->get_property = get_property;
	object_class->set_property = set_property;
	object_class->constructed = constructed;
	object_class->dispose = dispose;
	object_class->finalize = finalize;

	obj_properties[PROP_MANAGER] =
	    g_param_spec_object (NM_POLICY_MANAGER, "", "",
	                         BYX_TYPE_MANAGER,
	                         G_PARAM_WRITABLE |
	                         G_PARAM_CONSTRUCT_ONLY |
	                         G_PARAM_STATIC_STRINGS);
	obj_properties[PROP_SETTINGS] =
	    g_param_spec_object (NM_POLICY_SETTINGS, "", "",
	                         NM_TYPE_SETTINGS,
	                         G_PARAM_WRITABLE |
	                         G_PARAM_CONSTRUCT_ONLY |
	                         G_PARAM_STATIC_STRINGS);
	obj_properties[PROP_DEFAULT_IP4_DEVICE] =
	    g_param_spec_object (NM_POLICY_DEFAULT_IP4_DEVICE, "", "",
	                         NM_TYPE_DEVICE,
	                         G_PARAM_READABLE |
	                         G_PARAM_STATIC_STRINGS);
	obj_properties[PROP_DEFAULT_IP6_DEVICE] =
	    g_param_spec_object (NM_POLICY_DEFAULT_IP6_DEVICE, "", "",
	                         NM_TYPE_DEVICE,
	                         G_PARAM_READABLE |
	                         G_PARAM_STATIC_STRINGS);
	obj_properties[PROP_ACTIVATING_IP4_DEVICE] =
	    g_param_spec_object (NM_POLICY_ACTIVATING_IP4_DEVICE, "", "",
	                         NM_TYPE_DEVICE,
	                         G_PARAM_READABLE |
	                         G_PARAM_STATIC_STRINGS);
	obj_properties[PROP_ACTIVATING_IP6_DEVICE] =
	    g_param_spec_object (NM_POLICY_ACTIVATING_IP6_DEVICE, "", "",
	                         NM_TYPE_DEVICE,
	                         G_PARAM_READABLE |
	                         G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties (object_class, _PROPERTY_ENUMS_LAST, obj_properties);
}
