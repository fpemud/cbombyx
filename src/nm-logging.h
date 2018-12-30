/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#ifndef __BYX_LOGGING_H__
#define __BYX_LOGGING_H__

#include "nm-core-types.h"

#define NM_LOG_CONFIG_BACKEND_DEFAULT   "default"
#define NM_LOG_CONFIG_BACKEND_TERMINAL  "terminal"
#define NM_LOG_CONFIG_BACKEND_SYSLOG    "syslog"
#define NM_LOG_CONFIG_BACKEND_JOURNAL   "journal"

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

static inline NMLogDomain
LOGD_IP_from_af (int addr_family)
{
	switch (addr_family) {
	case AF_INET:  return LOGD_IP4;
	case AF_INET6: return LOGD_IP6;
	}
	g_return_val_if_reached (LOGD_NONE);
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

#define byx_log_err(domain, ...)    byx_log (LOGL_ERR,   (domain),  NULL, NULL, __VA_ARGS__)
#define byx_log_warn(domain, ...)   byx_log (LOGL_WARN,  (domain),  NULL, NULL, __VA_ARGS__)
#define byx_log_info(domain, ...)   byx_log (LOGL_INFO,  (domain),  NULL, NULL, __VA_ARGS__)
#define byx_log_dbg(domain, ...)    byx_log (LOGL_DEBUG, (domain),  NULL, NULL, __VA_ARGS__)
#define byx_log_trace(domain, ...)  byx_log (LOGL_TRACE, (domain),  NULL, NULL, __VA_ARGS__)

//#define _NM_LOG_FUNC G_STRFUNC
#define _NM_LOG_FUNC NULL

/* A wrapper for the _byx_log_impl() function that adds call site information.
 * Contrary to byx_log(), it unconditionally calls the function without
 * checking whether logging for the given level and domain is enabled. */
#define _byx_log(level, domain, error, ifname, con_uuid, ...) \
    G_STMT_START { \
        _byx_log_impl (__FILE__, __LINE__, \
                      _NM_LOG_FUNC, \
                      (level), \
                      (domain), \
                      (error), \
                      (ifname), \
                      (con_uuid), \
                      ""__VA_ARGS__); \
    } G_STMT_END

/* byx_log() only evaluates its argument list after checking
 * whether logging for the given level/domain is enabled.  */
#define byx_log(level, domain, ifname, con_uuid, ...) \
    G_STMT_START { \
        if (byx_logging_enabled ((level), (domain))) { \
            _byx_log (level, domain, 0, ifname, con_uuid, __VA_ARGS__); \
        } \
    } G_STMT_END

#define _byx_log_ptr(level, domain, ifname, con_uuid, self, prefix, ...) \
   byx_log ((level), \
           (domain), \
           (ifname), \
           (con_uuid), \
           "%s[%p] " _NM_UTILS_MACRO_FIRST(__VA_ARGS__), \
           (prefix) ?: "", \
           self _NM_UTILS_MACRO_REST(__VA_ARGS__))

static inline gboolean
_byx_log_ptr_is_debug (ByxLogLevel level)
{
	return level <= LOGL_DEBUG;
}

/* log a message for an object (with providing a generic @self pointer) */
#define byx_log_ptr(level, domain, ifname, con_uuid, self, prefix, ...) \
    G_STMT_START { \
        if (_byx_log_ptr_is_debug (level)) { \
            _byx_log_ptr ((level), \
                         (domain), \
                         (ifname), \
                         (con_uuid), \
                         (self), \
                         (prefix), \
                         __VA_ARGS__); \
        } else { \
            const char *__prefix = (prefix); \
            \
            byx_log ((level), \
                    (domain), \
                    (ifname), \
                    (con_uuid), \
                    "%s%s" _NM_UTILS_MACRO_FIRST(__VA_ARGS__), \
                    __prefix ?: "", \
                    __prefix ? " " : "" _NM_UTILS_MACRO_REST(__VA_ARGS__)); \
        } \
    } G_STMT_END

#define _byx_log_obj(level, domain, ifname, con_uuid, self, prefix, ...) \
    _byx_log_ptr ((level), \
                 (domain), \
                 (ifname), \
                 (con_uuid), \
                 (self), \
                 prefix, \
                 __VA_ARGS__)

/* log a message for an object (with providing a @self pointer to a GObject).
 * Contrary to byx_log_ptr(), @self must be a GObject type (or %NULL).
 * As of now, byx_log_obj() is identical to byx_log_ptr(), but we might change that */
#define byx_log_obj(level, domain, ifname, con_uuid, self, prefix, ...) \
    byx_log_ptr ((level), \
                (domain), \
                (ifname), \
                (con_uuid), \
                (self), \
                prefix, \
                __VA_ARGS__)

void _byx_log_impl (const char *file,
                   guint line,
                   const char *func,
                   ByxLogLevel level,
                   NMLogDomain domain,
                   int error,
                   const char *ifname,
                   const char *con_uuid,
                   const char *fmt,
                   ...) _nm_printf (9, 10);

const char *byx_logging_level_to_string (void);
const char *byx_logging_domains_to_string (void);

extern NMLogDomain _byx_logging_enabled_state[_LOGL_N_REAL];
static inline gboolean
byx_logging_enabled (ByxLogLevel level, NMLogDomain domain)
{
	nm_assert (((guint) level) < G_N_ELEMENTS (_byx_logging_enabled_state));
	return    (((guint) level) < G_N_ELEMENTS (_byx_logging_enabled_state))
	       && !!(_byx_logging_enabled_state[level] & domain);
}

ByxLogLevel byx_logging_get_level (NMLogDomain domain);

const char *byx_logging_all_levels_to_string (void);
const char *byx_logging_all_domains_to_string (void);

gboolean byx_logging_setup (const char  *level,
                           const char  *domains,
                           char       **bad_domains,
                           GError     **error);

void byx_logging_set_syslog_identifier (const char *domain);
void byx_logging_set_prefix (const char *format, ...) _nm_printf (1, 2);

void     byx_logging_openlog (const char *logging_backend, gboolean debug);

/*****************************************************************************/

#define _LOGT(...)          _NMLOG (LOGL_TRACE, __VA_ARGS__)
#define _LOGD(...)          _NMLOG (LOGL_DEBUG, __VA_ARGS__)
#define _LOGI(...)          _NMLOG (LOGL_INFO , __VA_ARGS__)
#define _LOGW(...)          _NMLOG (LOGL_WARN , __VA_ARGS__)
#define _LOGE(...)          _NMLOG (LOGL_ERR  , __VA_ARGS__)

#define _LOGT_err(errsv, ...) _NMLOG_err (errsv, LOGL_TRACE, __VA_ARGS__)
#define _LOGD_err(errsv, ...) _NMLOG_err (errsv, LOGL_DEBUG, __VA_ARGS__)
#define _LOGI_err(errsv, ...) _NMLOG_err (errsv, LOGL_INFO , __VA_ARGS__)
#define _LOGW_err(errsv, ...) _NMLOG_err (errsv, LOGL_WARN , __VA_ARGS__)
#define _LOGE_err(errsv, ...) _NMLOG_err (errsv, LOGL_ERR  , __VA_ARGS__)

/* _LOGT() and _LOGt() both log with level TRACE, but the latter is disabled by default,
 * unless building with --with-more-logging. */
#ifdef NM_MORE_LOGGING
#define _LOGt(...)            _NMLOG (LOGL_TRACE, __VA_ARGS__)
#define _LOGt_err(errsv, ...) _NMLOG_err (errsv, LOGL_TRACE, __VA_ARGS__)
#else
/* still call the logging macros to get compile time checks, but they will be optimized out. */
#define _LOGt(...)            G_STMT_START { if (FALSE) { _NMLOG (LOGL_TRACE, __VA_ARGS__); } } G_STMT_END
#define _LOGt_err(errsv, ...) G_STMT_START { if (FALSE) { _NMLOG_err (errsv, LOGL_TRACE, __VA_ARGS__); } } G_STMT_END
#endif

/*****************************************************************************/

extern void (*_byx_logging_clear_platform_logging_cache) (void);

/*****************************************************************************/

#define __NMLOG_DEFAULT(level, domain, prefix, ...) \
	G_STMT_START { \
		byx_log ((level), (domain), NULL, NULL, \
		        "%s: " _NM_UTILS_MACRO_FIRST(__VA_ARGS__), \
		        (prefix) \
		        _NM_UTILS_MACRO_REST(__VA_ARGS__)); \
	} G_STMT_END

#define __NMLOG_DEFAULT_WITH_ADDR(level, domain, prefix, ...) \
	G_STMT_START { \
		byx_log ((level), (domain), NULL, NULL, \
		        "%s[%p]: " _NM_UTILS_MACRO_FIRST(__VA_ARGS__), \
		        (prefix), \
		        (self) \
		        _NM_UTILS_MACRO_REST(__VA_ARGS__)); \
	} G_STMT_END

#endif /* __BYX_LOGGING_H__ */
