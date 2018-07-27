gboolean
is_valid_hostname (const char *hostname)
{
	const char *p;
	gboolean dot = TRUE;

	if (!hostname || !hostname[0])
		return FALSE;

	for (p = hostname; *p; p++) {
		if (*p == '.') {
			if (dot)
				return FALSE;
			dot = TRUE;
		} else {
			if (!g_ascii_isalnum (*p) && (*p != '-') && (*p != '_'))
				return FALSE;
			dot = FALSE;
		}
	}

	if (dot)
		return FALSE;

	return (p - hostname <= HOST_NAME_MAX);
}


		g_free (priv->dhcp4.pac_url);
		priv->dhcp4.pac_url = g_strdup (g_hash_table_lookup (options, "wpad"));


