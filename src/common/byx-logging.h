/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __BYX_LOGGING_H__
#define __BYX_LOGGING_H__

/* Log domains */
typedef enum  { /*< skip >*/
	LOGD_NONE       = 0LL,
	LOGD_PLATFORM   = (1LL << 0), /* Platform services */
	LOGD_RFKILL     = (1LL << 1),
	LOGD_ETHER      = (1LL << 2),
	LOGD_WIFI       = (1LL << 3),
	LOGD_BT         = (1LL << 4),
	LOGD_MB         = (1LL << 5), /* mobile broadband */
	LOGD_DHCP4      = (1LL << 6),
	LOGD_DHCP6      = (1LL << 7),
	LOGD_PPP        = (1LL << 8),
	LOGD_WIFI_SCAN  = (1LL << 9),
	LOGD_IP4        = (1LL << 10),
	LOGD_IP6        = (1LL << 11),
	LOGD_AUTOIP4    = (1LL << 12),
	LOGD_DNS        = (1LL << 13),
	LOGD_VPN        = (1LL << 14),
	LOGD_SHARING    = (1LL << 15), /* Connection sharing/dnsmasq */
	LOGD_SUPPLICANT = (1LL << 16), /* WiFi and 802.1x */
	LOGD_AGENTS     = (1LL << 17), /* Secret agents */
	LOGD_SETTINGS   = (1LL << 18), /* Settings */
	LOGD_SUSPEND    = (1LL << 19), /* Suspend/Resume */
	LOGD_CORE       = (1LL << 20), /* Core daemon and policy stuff */
	LOGD_DEVICE     = (1LL << 21), /* Device state and activation */
	LOGD_OLPC       = (1LL << 22),
	LOGD_INFINIBAND = (1LL << 23),
	LOGD_FIREWALL   = (1LL << 24),
	LOGD_ADSL       = (1LL << 25),
	LOGD_BOND       = (1LL << 26),
	LOGD_VLAN       = (1LL << 27),
	LOGD_BRIDGE     = (1LL << 28),
	LOGD_DBUS_PROPS = (1LL << 29),
	LOGD_TEAM       = (1LL << 30),
	LOGD_CONCHECK   = (1LL << 31),
	LOGD_DCB        = (1LL << 32), /* Data Center Bridging */
	LOGD_DISPATCH   = (1LL << 33),
	LOGD_AUDIT      = (1LL << 34),
	LOGD_SYSTEMD    = (1LL << 35),
	LOGD_VPN_PLUGIN = (1LL << 36),
	LOGD_PROXY      = (1LL << 37),

	__LOGD_MAX,
	LOGD_ALL       = (((__LOGD_MAX - 1LL) << 1) - 1LL),
	LOGD_DEFAULT   = LOGD_ALL & ~(
	                              LOGD_DBUS_PROPS |
	                              LOGD_WIFI_SCAN |
	                              LOGD_VPN_PLUGIN |
	                              0),

	/* aliases: */
	LOGD_DHCP       = LOGD_DHCP4 | LOGD_DHCP6,
	LOGD_IP         = LOGD_IP4 | LOGD_IP6,
} NMLogDomain;

static inline NMLogDomain LOGD_IP_from_af (int addr_family)
{
	return LOGD_IP4;
}

/* Log levels */
typedef enum  { /*< skip >*/
	LOGL_TRACE,
	LOGL_DEBUG,
	LOGL_INFO,
	LOGL_WARN,
	LOGL_ERR,

	_LOGL_N_REAL, /* the number of actual logging levels */

	_LOGL_OFF = _LOGL_N_REAL, /* special logging level that is always disabled. */
	_LOGL_KEEP,               /* special logging level to indicate that the logging level should not be changed. */

	_LOGL_N, /* the number of logging levels including "OFF" */
} ByxLogLevel;

#define byx_log_err(domain, ...)
#define byx_log_warn(domain, ...)
#define byx_log_info(domain, ...)
#define byx_log_dbg(domain, ...)
#define byx_log_trace(domain, ...)

#define _byx_log(level, domain, error, ifname, con_uuid, ...)

#define byx_log(level, domain, ifname, con_uuid, ...)

static inline gboolean byx_logging_enabled (ByxLogLevel level, NMLogDomain domain)
{
	return FALSE;
}

const char *byx_logging_all_levels_to_string (void);
const char *byx_logging_all_domains_to_string (void);

gboolean inline byx_logging_setup (const char  *level,
                           const char  *domains,
                           char       **bad_domains,
                           GError     **error)
{
	return TRUE;
}

/*****************************************************************************/

#define _LOGT(...)
#define _LOGD(...)
#define _LOGI(...)
#define _LOGW(...)
#define _LOGE(...)

#define _LOGT_err(errsv, ...)
#define _LOGD_err(errsv, ...)
#define _LOGI_err(errsv, ...)
#define _LOGW_err(errsv, ...)
#define _LOGE_err(errsv, ...)

#define _LOGt(...)
#define _LOGt_err(errsv, ...)

/*****************************************************************************/

#define __NMLOG_DEFAULT(level, domain, prefix, ...)

#define __NMLOG_DEFAULT_WITH_ADDR(level, domain, prefix, ...)

#endif /* __BYX_LOGGING_H__ */
