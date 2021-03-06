/*	$NetBSD */

/*
 * Copyright (c) 2011, Lourival Neto <lneto@NetBSD.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the Author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * This file is a placeholder only, to allow Lua to be compiled from
 * unchanged sources.
 */

#include <sys/systm.h>

#ifndef _LUA_INCLUDE_CTYPE_
#define _LUA_INCLUDE_CTYPE_

LIBKERN_INLINE int
isalnum(int ch)
{
	return (isalpha(ch) || isdigit(ch));
}

LIBKERN_INLINE int
iscntrl(int ch)
{
	return ((ch >= 0x00 && ch <= 0x1F) || ch == 0x7F);
}

LIBKERN_INLINE int
isprint(int ch)
{
	return (ch >= 0x20 && ch <= 0x7E);
}

LIBKERN_INLINE int
ispunct(int ch)
{
	return (isprint(ch) && ch != ' ' && !isalnum(ch));
}

#endif

