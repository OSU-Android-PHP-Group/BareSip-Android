/**
 * @file ns.c  DNS Nameserver configuration
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <stdio.h>
#include <re_types.h>
#include <re_fmt.h>
#include <re_mbuf.h>
#include <re_list.h>
#include <re_sa.h>
#include <re_dns.h>
#include "dns.h"


#define DEBUG_MODULE "ns"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


#ifdef __SYMBIAN32__
extern int get_symbiandns(struct sa *nsv, uint32_t *n);
#endif


static int parse_resolv_conf(char *domain, size_t dsize,
			     struct sa *srvv, uint32_t *n)
{
	FILE *f;
	struct pl dom = pl_null;
	uint32_t i = 0;
	int err = 0;

	if (!srvv || !n || !*n)
		return EINVAL;

	f = fopen("/etc/resolv.conf", "r");
	if (!f)
		return errno;

	for (;;) {
		char line[128];
		struct pl srv;
		size_t len;

		if (1 != fscanf(f, "%127[^\n]\n", line))
			break;

		if ('#' == line[0])
			continue;

		len = str_len(line);

		/* Set domain if not already set */
		if (!pl_isset(&dom)) {
			if (0 == re_regex(line, len, "domain [^ ]+", &dom)) {
				(void)pl_strcpy(&dom, domain, dsize);
			}

			if (0 == re_regex(line, len, "search [^ ]+", &dom)) {
				(void)pl_strcpy(&dom, domain, dsize);
			}
		}

		/* Use the first entry */
		if (i < *n && 0 == re_regex(line, len, "nameserver [^\n]+",
					    &srv)) {
			err = sa_set(&srvv[i], &srv, DNS_PORT);
			if (err) {
				DEBUG_WARNING("sa_set: %r (%m)\n", &srv, err);
			}
			++i;
		}
	}

	*n = i;

	(void)fclose(f);

	return err;
}


int dns_srv_get(char *domain, size_t dsize, struct sa *srvv, uint32_t *n)
{
	int err;

	/* Try them all in prioritized order */

#ifdef HAVE_LIBRESOLV
	err = get_resolv_dns(domain, dsize, srvv, n);
	if (!err)
		return 0;
#endif

#ifdef DARWIN
	err = get_darwin_dns(domain, dsize, srvv, n);
	if (!err)
		return 0;
#endif

	err = parse_resolv_conf(domain, dsize, srvv, n);
	if (!err)
		return 0;

#ifdef WIN32
	err = get_windns(domain, dsize, srvv, n);
#endif

#ifdef __SYMBIAN32__
	err = get_symbiandns(srvv, n);
#endif

	return err;
}
