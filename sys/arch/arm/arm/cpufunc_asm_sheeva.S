/*-
 * Copyright (C) 2008 MARVELL INTERNATIONAL LTD.
 * All rights reserved.
 *
 * Developed by Semihalf.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of MARVELL nor the names of contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <arm/asm.h>
#include <arm/locore.h>

#include "assym.h"

.Lsheeva_cache_line_size:
	.word	_C_LABEL(arm_pcache) + DCACHE_LINE_SIZE
.Lsheeva_asm_page_mask:
	.word	_C_LABEL(PAGE_MASK)

ENTRY(sheeva_dcache_wbinv_range)
	str	lr, [sp, #-4]!
	mrs	lr, cpsr
	/* Start with cache line aligned address */
	ldr	ip, .Lsheeva_cache_line_size
	ldr	ip, [ip]
	sub	ip, ip, #1
	and	r2, r0, ip
	add	r1, r1, r2
	add	r1, r1, ip
	bics	r1, r1, ip
	bics	r0, r0, ip

	ldr	ip, .Lsheeva_asm_page_mask
	and	r2, r0, ip
	rsb	r2, r2, #PAGE_SIZE
	cmp	r1, r2
	movcc	ip, r1
	movcs	ip, r2
1:
	add	r3, r0, ip
	sub	r2, r3, #1
	/* Disable irqs */
	orr	r3, lr, #I32_bit | F32_bit
	msr	cpsr_c, r3
	mcr	p15, 5, r0, c15, c15, 0	/* Clean and inv zone start address */
	mcr	p15, 5, r2, c15, c15, 1	/* Clean and inv zone end address */
	/* Enable irqs */
	msr	cpsr_c, lr

	add	r0, r0, ip
	sub	r1, r1, ip
	cmp	r1, #PAGE_SIZE
	movcc	ip, r1
	movcs	ip, #PAGE_SIZE
	cmp	r1, #0
	bne	1b
	mov	r0, #0
	mcr	p15, 0, r0, c7, c10, 4	/* drain the write buffer */
	ldr	lr, [sp], #4
	RET

ENTRY(sheeva_dcache_inv_range)
	str	lr, [sp, #-4]!
	mrs	lr, cpsr
	/* Start with cache line aligned address */
	ldr	ip, .Lsheeva_cache_line_size
	ldr	ip, [ip]
	sub	ip, ip, #1
	and	r2, r0, ip
	add	r1, r1, r2
	add	r1, r1, ip
	bics	r1, r1, ip
	bics	r0, r0, ip

	ldr	ip, .Lsheeva_asm_page_mask
	and	r2, r0, ip
	rsb	r2, r2, #PAGE_SIZE
	cmp	r1, r2
	movcc	ip, r1
	movcs	ip, r2
1:
	add	r3, r0, ip
	sub	r2, r3, #1
	/* Disable irqs */
	orr	r3, lr, #I32_bit | F32_bit
	msr	cpsr_c, r3
	mcr	p15, 5, r0, c15, c14, 0	/* Inv zone start address */
	mcr	p15, 5, r2, c15, c14, 1	/* Inv zone end address */
	/* Enable irqs */
	msr	cpsr_c, lr

	add	r0, r0, ip
	sub	r1, r1, ip
	cmp	r1, #PAGE_SIZE
	movcc	ip, r1
	movcs	ip, #PAGE_SIZE
	cmp	r1, #0
	bne	1b
	mov	r0, #0
	mcr	p15, 0, r0, c7, c10, 4	/* drain the write buffer */
	ldr	lr, [sp], #4
	RET

ENTRY(sheeva_dcache_wb_range)
	str	lr, [sp, #-4]!
	mrs	lr, cpsr
	/* Start with cache line aligned address */
	ldr	ip, .Lsheeva_cache_line_size
	ldr	ip, [ip]
	sub	ip, ip, #1
	and	r2, r0, ip
	add	r1, r1, r2
	add	r1, r1, ip
	bics	r1, r1, ip
	bics	r0, r0, ip

	ldr	ip, .Lsheeva_asm_page_mask
	and	r2, r0, ip
	rsb	r2, r2, #PAGE_SIZE
	cmp	r1, r2
	movcc	ip, r1
	movcs	ip, r2
1:
	add	r3, r0, ip
	sub	r2, r3, #1
	/* Disable irqs */
	orr	r3, lr, #I32_bit | F32_bit
	msr	cpsr_c, r3
	mcr	p15, 5, r0, c15, c13, 0	/* Clean zone start address */
	mcr	p15, 5, r2, c15, c13, 1	/* Clean zone end address */
	/* Enable irqs */
	msr	cpsr_c, lr

	add	r0, r0, ip
	sub	r1, r1, ip
	cmp	r1, #PAGE_SIZE
	movcc	ip, r1
	movcs	ip, #PAGE_SIZE
	cmp	r1, #0
	bne	1b
	mov	r0, #0
	mcr	p15, 0, r0, c7, c10, 4	/* drain the write buffer */
	ldr	lr, [sp], #4
	RET

ENTRY(sheeva_idcache_wbinv_range)
	str	lr, [sp, #-4]!
	mrs	lr, cpsr
	/* Start with cache line aligned address */
	ldr	ip, .Lsheeva_cache_line_size
	ldr	ip, [ip]
	sub	ip, ip, #1
	and	r2, r0, ip
	add	r1, r1, r2
	add	r1, r1, ip
	bics	r1, r1, ip
	bics	r0, r0, ip

	ldr	ip, .Lsheeva_asm_page_mask
	and	r2, r0, ip
	rsb	r2, r2, #PAGE_SIZE
	cmp	r1, r2
	movcc	ip, r1
	movcs	ip, r2
1:
	add	r3, r0, ip
	sub	r2, r3, #1
	/* Disable irqs */
	orr	r3, lr, #I32_bit | F32_bit
	msr	cpsr_c, r3
	mcr	p15, 5, r0, c15, c15, 0	/* Clean and inv zone start address */
	mcr	p15, 5, r2, c15, c15, 1	/* Clean and inv zone end address */
	/* Enable irqs */
	msr	cpsr_c, lr

	/* Invalidate and clean icache line by line */
	ldr	r3, .Lsheeva_cache_line_size
	ldr	r3, [r3]
2:
	mcr	p15, 0, r0, c7, c5, 1
	add	r0, r0, r3
	cmp	r2, r0
	bhi	2b

	add	r0, r2, #1
	sub	r1, r1, ip
	cmp	r1, #PAGE_SIZE
	movcc	ip, r1
	movcs	ip, #PAGE_SIZE
	cmp	r1, #0
	bne	1b
	mov	r0, #0
	mcr	p15, 0, r0, c7, c10, 4	/* drain the write buffer */
	ldr	lr, [sp], #4
	RET

/*
 * CPU sleep
 */
ENTRY_NP(sheeva_cpu_sleep)
	mov	r0, #0
	mcr	p15, 0, r0, c7, c0, 4	/* wait for interrupt */
	RET
