/*	$NetBSD: net_stub.c,v 1.14 2013/06/26 17:10:23 pooka Exp $	*/

/*
 * Copyright (c) 2008 Antti Kantee.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: net_stub.c,v 1.14 2013/06/26 17:10:23 pooka Exp $");

#include <sys/param.h>
#include <sys/protosw.h>
#include <sys/socketvar.h>

#include <net/if.h>
#include <net/route.h>

#include <compat/sys/socket.h>
#include <compat/sys/sockio.h>

int __rumpnet_stub(void);
int
__rumpnet_stub(void)
{

	panic("component not available");
}
__weak_alias(rtioctl,__rumpnet_stub);
__weak_alias(rtrequest,__rumpnet_stub);
__weak_alias(ifioctl,__rumpnet_stub);
__weak_alias(ifunit,__rumpnet_stub);
__weak_alias(ifreq_setaddr,__rumpnet_stub);

/*
 * Weak symbols so that we can optionally leave components out.
 * (would be better to fix sys/net* to be more modular, though)
 */

/* bridge */
__weak_alias(bridge_ifdetach,__rumpnet_stub);
__weak_alias(bridge_output,__rumpnet_stub);
__weak_alias(bridge_input,__rumpnet_stub);

/* agr */
__weak_alias(agr_input,__rumpnet_stub);
__weak_alias(ieee8023ad_lacp_input,__rumpnet_stub);
__weak_alias(ieee8023ad_marker_input,__rumpnet_stub);

struct ifnet_head ifnet;

int
compat_ifconf(u_long cmd, void *data)
{

	return EOPNOTSUPP;
}
