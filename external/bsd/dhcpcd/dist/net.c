#include <sys/cdefs.h>
 __RCSID("$NetBSD: net.c,v 1.3 2013/09/20 10:56:32 roy Exp $");

/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2013 Roy Marples <roy@marples.name>
 * All rights reserved

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#ifdef __FreeBSD__ /* Needed so that including netinet6/in6_var.h works */
#  include <net/if_var.h>
#endif
#ifdef AF_LINK
#  include <net/if_dl.h>
#  include <net/if_types.h>
#  include <netinet/in_var.h>
#endif
#ifdef AF_PACKET
#  include <netpacket/packet.h>
#endif
#ifdef SIOCGIFMEDIA
#  include <net/if_media.h>
#endif

#include <net/route.h>
#ifdef __linux__
#  include <asm/types.h> /* for systems with broken headers */
#  include <linux/rtnetlink.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <ifaddrs.h>
#include <fnmatch.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "config.h"
#include "common.h"
#include "dev.h"
#include "dhcp.h"
#include "dhcp6.h"
#include "if-options.h"
#include "ipv4.h"
#include "ipv6nd.h"
#include "net.h"

int socket_afnet = -1;

static char hwaddr_buffer[(HWADDR_LEN * 3) + 1 + 1024];

char *
hwaddr_ntoa(const unsigned char *hwaddr, size_t hwlen)
{
	char *p = hwaddr_buffer;
	size_t i;

	for (i = 0; i < hwlen; i++) {
		if (i > 0)
			*p ++= ':';
		p += snprintf(p, 3, "%.2x", hwaddr[i]);
	}

	*p ++= '\0';

	return hwaddr_buffer;
}

size_t
hwaddr_aton(unsigned char *buffer, const char *addr)
{
	char c[3];
	const char *p = addr;
	unsigned char *bp = buffer;
	size_t len = 0;

	c[2] = '\0';
	while (*p) {
		c[0] = *p++;
		c[1] = *p++;
		/* Ensure that digits are hex */
		if (isxdigit((unsigned char)c[0]) == 0 ||
		    isxdigit((unsigned char)c[1]) == 0)
		{
			errno = EINVAL;
			return 0;
		}
		/* We should have at least two entries 00:01 */
		if (len == 0 && *p == '\0') {
			errno = EINVAL;
			return 0;
		}
		/* Ensure that next data is EOL or a seperator with data */
		if (!(*p == '\0' || (*p == ':' && *(p + 1) != '\0'))) {
			errno = EINVAL;
			return 0;
		}
		if (*p)
			p++;
		if (bp)
			*bp++ = (unsigned char)strtol(c, NULL, 16);
		len++;
	}
	return len;
}

void
free_interface(struct interface *ifp)
{

	if (ifp == NULL)
		return;
	ipv4_free(ifp);
	dhcp_free(ifp);
	ipv6_free(ifp);
	dhcp6_free(ifp);
	ipv6nd_free(ifp);
	free_options(ifp->options);
	free(ifp);
}

int
carrier_status(struct interface *iface)
{
	int ret;
	struct ifreq ifr;
#ifdef SIOCGIFMEDIA
	struct ifmediareq ifmr;
#endif
#ifdef __linux__
	char *p;
#endif

	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, iface->name, sizeof(ifr.ifr_name));
#ifdef __linux__
	/* We can only test the real interface up */
	if ((p = strchr(ifr.ifr_name, ':')))
		*p = '\0';
#endif

	if (ioctl(socket_afnet, SIOCGIFFLAGS, &ifr) == -1)
		return LINK_UNKNOWN;
	iface->flags = ifr.ifr_flags;

	ret = LINK_UNKNOWN;
#ifdef SIOCGIFMEDIA
	memset(&ifmr, 0, sizeof(ifmr));
	strlcpy(ifmr.ifm_name, iface->name, sizeof(ifmr.ifm_name));
	if (ioctl(socket_afnet, SIOCGIFMEDIA, &ifmr) != -1 &&
	    ifmr.ifm_status & IFM_AVALID)
		ret = (ifmr.ifm_status & IFM_ACTIVE) ? LINK_UP : LINK_DOWN;
#endif
	if (ret == LINK_UNKNOWN)
		ret = (ifr.ifr_flags & IFF_RUNNING) ? LINK_UP : LINK_DOWN;
	return ret;
}

int
up_interface(struct interface *iface)
{
	struct ifreq ifr;
	int retval = -1;
#ifdef __linux__
	char *p;
#endif

	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, iface->name, sizeof(ifr.ifr_name));
#ifdef __linux__
	/* We can only bring the real interface up */
	if ((p = strchr(ifr.ifr_name, ':')))
		*p = '\0';
#endif
	if (ioctl(socket_afnet, SIOCGIFFLAGS, &ifr) == 0) {
		if ((ifr.ifr_flags & IFF_UP))
			retval = 0;
		else {
			ifr.ifr_flags |= IFF_UP;
			if (ioctl(socket_afnet, SIOCSIFFLAGS, &ifr) == 0)
				retval = 0;
		}
		iface->flags = ifr.ifr_flags;
	}
	return retval;
}

struct if_head *
discover_interfaces(int argc, char * const *argv)
{
	struct ifaddrs *ifaddrs, *ifa;
	char *p;
	int i, sdl_type;
	struct if_head *ifs;
	struct interface *ifp;
#ifdef __linux__
	char ifn[IF_NAMESIZE];
#endif
#ifdef INET
	const struct sockaddr_in *addr;
	const struct sockaddr_in *net;
	const struct sockaddr_in *dst;
#ifdef INET6
	const struct sockaddr_in6 *sin6;
	int ifa_flags;
#endif
#ifdef AF_LINK
	const struct sockaddr_dl *sdl;
#ifdef IFLR_ACTIVE
	struct if_laddrreq iflr;
	int socket_aflink;

	socket_aflink = socket(AF_LINK, SOCK_DGRAM, 0);
	if (socket_aflink == -1)
		return NULL;
	memset(&iflr, 0, sizeof(iflr));
#endif
#elif AF_PACKET
	const struct sockaddr_ll *sll;
#endif

	if (getifaddrs(&ifaddrs) == -1)
		return NULL;

	ifs = malloc(sizeof(*ifs));
	if (ifs == NULL)
		return NULL;
	TAILQ_INIT(ifs);

	for (ifa = ifaddrs; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr != NULL) {
#ifdef AF_LINK
			if (ifa->ifa_addr->sa_family != AF_LINK)
				continue;
#elif AF_PACKET
			if (ifa->ifa_addr->sa_family != AF_PACKET)
				continue;
#endif
		}

		/* Ensure that the interface name has settled */
		if (!dev_initialized(ifa->ifa_name))
			continue;

		/* It's possible for an interface to have >1 AF_LINK.
		 * For our purposes, we use the first one. */
		TAILQ_FOREACH(ifp, ifs, next) {
			if (strcmp(ifp->name, ifa->ifa_name) == 0)
				break;
		}
		if (ifp)
			continue;
		if (argc > 0) {
			for (i = 0; i < argc; i++) {
#ifdef __linux__
				/* Check the real interface name */
				strlcpy(ifn, argv[i], sizeof(ifn));
				p = strchr(ifn, ':');
				if (p)
					*p = '\0';
				if (strcmp(ifn, ifa->ifa_name) == 0)
					break;
#else
				if (strcmp(argv[i], ifa->ifa_name) == 0)
					break;
#endif
			}
			if (i == argc)
				continue;
			p = argv[i];
		} else {
			p = ifa->ifa_name;
			/* -1 means we're discovering against a specific
			 * interface, but we still need the below rules
			 * to apply. */
			if (argc == -1 && strcmp(argv[0], ifa->ifa_name) != 0)
				continue;
		}
		for (i = 0; i < ifdc; i++)
			if (!fnmatch(ifdv[i], p, 0))
				break;
		if (i < ifdc)
			continue;
		for (i = 0; i < ifac; i++)
			if (!fnmatch(ifav[i], p, 0))
				break;
		if (ifac && i == ifac)
			continue;

		ifp = calloc(1, sizeof(*ifp));
		if (ifp == NULL)
			return NULL;
		strlcpy(ifp->name, p, sizeof(ifp->name));
		ifp->flags = ifa->ifa_flags;

		/* Bring the interface up if not already */
		if (!(ifp->flags & IFF_UP)
#ifdef SIOCGIFMEDIA
		    && carrier_status(ifp) != LINK_UNKNOWN
#endif
		   )
		{
			if (up_interface(ifp) == 0)
				options |= DHCPCD_WAITUP;
			else
				syslog(LOG_ERR, "%s: up_interface: %m",
				    ifp->name);
		}

		sdl_type = 0;
		/* Don't allow loopback unless explicit */
		if (ifp->flags & IFF_LOOPBACK) {
			if (argc == 0 && ifac == 0) {
				free_interface(ifp);
				continue;
			}
		} else if (ifa->ifa_addr != NULL) {
#ifdef AF_LINK
			sdl = (const struct sockaddr_dl *)(void *)ifa->ifa_addr;

#ifdef IFLR_ACTIVE
			/* We need to check for active address */
			strlcpy(iflr.iflr_name, ifp->name,
			    sizeof(iflr.iflr_name));
			memcpy(&iflr.addr, ifa->ifa_addr,
			    MIN(ifa->ifa_addr->sa_len, sizeof(iflr.addr)));
			iflr.flags = IFLR_PREFIX;
			iflr.prefixlen = sdl->sdl_alen * NBBY;
			if (ioctl(socket_aflink, SIOCGLIFADDR, &iflr) == -1 ||
			    !(iflr.flags & IFLR_ACTIVE))
			{
				free_interface(ifp);
				continue;
			}
#endif

			ifp->index = sdl->sdl_index;
			sdl_type = sdl->sdl_type;
			switch(sdl->sdl_type) {
			case IFT_BRIDGE: /* FALLTHROUGH */
			case IFT_L2VLAN: /* FALLTHOUGH */
			case IFT_L3IPVLAN: /* FALLTHROUGH */
			case IFT_ETHER:
				ifp->family = ARPHRD_ETHER;
				break;
			case IFT_IEEE1394:
				ifp->family = ARPHRD_IEEE1394;
				break;
#ifdef IFT_INFINIBAND
			case IFT_INFINIBAND:
				ifp->family = ARPHRD_INFINIBAND;
				break;
#endif
			}
			ifp->hwlen = sdl->sdl_alen;
#ifndef CLLADDR
#  define CLLADDR(s) ((const char *)((s)->sdl_data + (s)->sdl_nlen))
#endif
			memcpy(ifp->hwaddr, CLLADDR(sdl), ifp->hwlen);
#elif AF_PACKET
			sll = (const struct sockaddr_ll *)(void *)ifa->ifa_addr;
			ifp->index = sll->sll_ifindex;
			ifp->family = sdl_type = sll->sll_hatype;
			ifp->hwlen = sll->sll_halen;
			if (ifp->hwlen != 0)
				memcpy(ifp->hwaddr, sll->sll_addr, ifp->hwlen);
#endif
		}
#ifdef __linux__
		/* PPP addresses on Linux don't have hardware addresses */
		else
			ifp->index = if_nametoindex(ifp->name);
#endif

		/* We only work on ethernet by default */
		if (!(ifp->flags & IFF_POINTOPOINT) &&
		    ifp->family != ARPHRD_ETHER)
		{
			if (argc == 0 && ifac == 0) {
				free_interface(ifp);
				continue;
			}
			switch (ifp->family) {
			case ARPHRD_IEEE1394: /* FALLTHROUGH */
			case ARPHRD_INFINIBAND:
				/* We don't warn for supported families */
				break;
			default:
				syslog(LOG_WARNING,
				    "%s: unsupported interface type %.2x"
				    ", falling back to ethernet",
				    ifp->name, sdl_type);
				ifp->family = ARPHRD_ETHER;
				break;
			}
		}

		/* Handle any platform init for the interface */
		if (if_init(ifp) == -1) {
			syslog(LOG_ERR, "%s: if_init: %m", p);
			free_interface(ifp);
			continue;
		}

		/* Ensure that the MTU is big enough for DHCP */
		if (get_mtu(ifp->name) < MTU_MIN &&
		    set_mtu(ifp->name, MTU_MIN) == -1)
		{
			syslog(LOG_ERR, "%s: set_mtu: %m", p);
			free_interface(ifp);
			continue;
		}

		/* We reserve the 100 range for virtual interfaces, if and when
		 * we can work them out. */
		ifp->metric = 200 + ifp->index;
		if (getifssid(ifp->name, ifp->ssid) != -1) {
			ifp->wireless = 1;
			ifp->metric += 100;
		}

		TAILQ_INSERT_TAIL(ifs, ifp, next);
	}

	for (ifa = ifaddrs; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL)
			continue;
		switch(ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			addr = (const struct sockaddr_in *)
			    (void *)ifa->ifa_addr;
			net = (const struct sockaddr_in *)
			    (void *)ifa->ifa_netmask;
			if (ifa->ifa_flags & IFF_POINTOPOINT)
				dst = (const struct sockaddr_in *)
				    (void *)ifa->ifa_dstaddr;
			else
				dst = NULL;
			ipv4_handleifa(RTM_NEWADDR, ifs, ifa->ifa_name,
				&addr->sin_addr,
				&net->sin_addr,
				dst ? &dst->sin_addr : NULL);
			break;
#endif
#ifdef INET6
		case AF_INET6:
			sin6 = (const struct sockaddr_in6 *)
			    (void *)ifa->ifa_addr;
			ifa_flags = in6_addr_flags(ifa->ifa_name,
			    &sin6->sin6_addr);
			if (ifa_flags != -1)
				ipv6_handleifa(RTM_NEWADDR, ifs,
				    ifa->ifa_name,
				    &sin6->sin6_addr, ifa_flags);
			break;
#endif
		}
	}
#endif

	freeifaddrs(ifaddrs);

#ifdef IFLR_ACTIVE
	close(socket_aflink);
#endif

	return ifs;
}

int
do_mtu(const char *ifname, short int mtu)
{
	struct ifreq ifr;
	int r;

	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	ifr.ifr_mtu = mtu;
	r = ioctl(socket_afnet, mtu ? SIOCSIFMTU : SIOCGIFMTU, &ifr);
	if (r == -1)
		return -1;
	return ifr.ifr_mtu;
}
