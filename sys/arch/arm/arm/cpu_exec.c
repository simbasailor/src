/*	$NetBSD: cpu_exec.c,v 1.5 2013/09/10 21:30:21 matt Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu_exec.c,v 1.5 2013/09/10 21:30:21 matt Exp $");

#include "opt_compat_netbsd.h"
#include "opt_compat_netbsd32.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/exec.h>

#include <uvm/uvm_extern.h>

#include <compat/common/compat_util.h>
#include <sys/exec_elf.h>			/* mandatory */

#ifdef COMPAT_NETBSD32
#include <compat/netbsd32/netbsd32_exec.h>
#endif

#if EXEC_ELF32
int
arm_netbsd_elf32_probe(struct lwp *l, struct exec_package *epp, void *eh0,
	char *itp, vaddr_t *start_p)
{
	const char *itp_suffix = NULL;
	const Elf_Ehdr * const eh = eh0;
	const bool elf_aapcs_p =
	    (eh->e_flags & EF_ARM_EABIMASK) >= EF_ARM_EABI_VER4;
#ifdef COMPAT_NETBSD32
	const bool netbsd32_p = (epp->ep_esch->es_emul == &emul_netbsd32);
#else
	const bool netbsd32_p = false;
#endif
#ifdef __ARM_EABI__
	const bool aapcs_p = true;
#else
	const bool aapcs_p = false;
#endif
#ifdef __ARMEB__
	const bool be8_p = (eh->e_flags & EF_ARM_BE8) != 0;

	/*
	 * If the BE-8 model is supported, CPSR[7] will be clear.
	 * If the BE-32 model is supported, CPSR[7] will be set.
	 */
	register_t cpsr;
	__asm("mrs\t%0, cpsr" : "=r"(cpsr));
	if ((cpsr & CPU_CONTROL_BEND_ENABLE) == be8_p)
		return ENOEXEC;
#endif /* __ARMEB__ */

	/*
	 * This is subtle.  If are netbsd32, then we don't want to match the
	 * same ABI as the kernel.  If we aren't (netbsd32 == false), then we
	 * don't want to be different from the kernel's ABI.
	 *    true   true   true  ENOEXEC
	 *    true   false  true  0
	 *    true   true   false 0
	 *    true   false  false ENOEXEC
	 *    false  true   true  0
	 *    false  false  true  ENOEXEC
	 *    false  true   false ENOEXEC
	 *    false  false  false 0
	 */
	if (netbsd32_p ^ elf_aapcs_p ^ aapcs_p)
		return ENOEXEC;

	if (netbsd32_p)
		itp_suffix = (elf_aapcs_p) ? "eabi" : "oabi";

	if (itp_suffix != NULL)
		(void)compat_elf_check_interp(epp, itp, itp_suffix);

	/*
	 * Copy (if any) the machine_arch of the executable to the proc.
	 */
	if (epp->ep_machine_arch[0] != 0) {
		strlcpy(l->l_proc->p_md.md_march, epp->ep_machine_arch,
		    sizeof(l->l_proc->p_md.md_march));
	}
	return 0;
}
#endif
