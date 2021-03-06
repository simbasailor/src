/*	$NetBSD: vnduncompress.c,v 1.2 2013/05/06 22:53:24 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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
__RCSID("$NetBSD: vnduncompress.c,v 1.2 2013/05/06 22:53:24 riastradh Exp $");

#include <sys/endian.h>

#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <zlib.h>

#include "common.h"

int
vnduncompress(int argc, char **argv, const struct options *O __unused)
{

	if (argc != 2)
		usage();

	const char *const cloop2_pathname = argv[0];
	const char *const image_pathname = argv[1];

	/* Open the cloop2 and image files.  */
	const int cloop2_fd = open(cloop2_pathname, O_RDONLY);
	if (cloop2_fd == -1)
		err(1, "open(%s)", cloop2_pathname);

	const int image_fd = open(image_pathname,
	    /* XXX O_EXCL, not O_TRUNC */
	    (O_WRONLY | O_CREAT | O_TRUNC), 0777);
	if (image_fd == -1)
		err(1, "open(%s)", image_pathname);

	/* Read the header.  */
	struct cloop2_header header;
	const ssize_t h_read = read(cloop2_fd, &header, sizeof(header));
	if (h_read == -1)
		err(1, "read header");
	assert(h_read >= 0);
	if ((size_t)h_read != sizeof(header))
		errx(1, "partial read of header: %zu != %zu",
		    (size_t)h_read, sizeof(header));

	const uint32_t blocksize = be32toh(header.cl2h_blocksize);
	const uint32_t n_blocks = be32toh(header.cl2h_n_blocks);

	/* Sanity-check the header parameters.  */
	__CTASSERT(MIN_BLOCKSIZE <= UINT32_MAX);
	if (blocksize < MIN_BLOCKSIZE)
		errx(1, "blocksize too small: %"PRIu32
		    " (must be at least %"PRIu32")",
		    blocksize, (uint32_t)MIN_BLOCKSIZE);
	__CTASSERT(MAX_BLOCKSIZE <= UINT32_MAX);
	if (MAX_BLOCKSIZE < blocksize)
		errx(1, "blocksize too large: %"PRIu32
		    " (must be at most %"PRIu32")",
		    blocksize, (uint32_t)MAX_BLOCKSIZE);
	__CTASSERT(DEV_BSIZE <= UINT32_MAX);
	if ((blocksize % DEV_BSIZE) != 0)
		errx(1, "bad blocksize: %"PRIu32
		    " (not a multiple of %"PRIu32")",
		    blocksize, (uint32_t)DEV_BSIZE);
	__CTASSERT(MAX_N_BLOCKS <= UINT32_MAX);
	if (MAX_N_BLOCKS < n_blocks)
		errx(1, "too many blocks: %"PRIu32" (max %"PRIu32")",
		    n_blocks, (uint32_t)MAX_N_BLOCKS);

	/* Allocate an offset table.  */
	__CTASSERT(MAX_N_BLOCKS <= (UINT32_MAX - 1));
	__CTASSERT((MAX_N_BLOCKS + 1) == MAX_N_OFFSETS);
	const uint32_t n_offsets = (n_blocks + 1);

	__CTASSERT(MAX_N_OFFSETS <= (SIZE_MAX / sizeof(uint64_t)));
	uint64_t *const offset_table = malloc(n_offsets * sizeof(uint64_t));

	/* Read the offset table in.  */
	const ssize_t ot_read = read(cloop2_fd, offset_table,
	    (n_offsets * sizeof(uint64_t)));
	if (ot_read == -1)
		err(1, "read offset table");
	assert(ot_read >= 0);
	if ((size_t)ot_read != (n_offsets * sizeof(uint64_t)))
		errx(1, "partial read of offset table: %zu != %zu",
		    (size_t)ot_read, (size_t)(n_offsets * sizeof(uint64_t)));

	/* Allocate compression buffers.  */
	/* XXX compression ratio bound */
	__CTASSERT(MAX_BLOCKSIZE <= (SIZE_MAX / 2));
	void *const compbuf = malloc(2 * (size_t)blocksize);
	if (compbuf == NULL)
		err(1, "malloc compressed buffer");

	__CTASSERT(MAX_BLOCKSIZE <= SIZE_MAX);
	void *const uncompbuf = malloc(blocksize);
	if (uncompbuf == NULL)
		err(1, "malloc uncompressed buffer");

	/*
	 * Uncompress the blocks.
	 */
	__CTASSERT(MAX_N_OFFSETS <= (OFF_MAX / sizeof(uint64_t)));
	__CTASSERT(sizeof(header) <=
	    (OFF_MAX - (MAX_N_OFFSETS * sizeof(uint64_t))));
	__CTASSERT(OFF_MAX <= UINT64_MAX);
	uint64_t offset = (sizeof(header) + (n_offsets * sizeof(uint64_t)));
	uint32_t blkno;
	for (blkno = 0; blkno < n_blocks; blkno++) {
		const uint64_t start = be64toh(offset_table[blkno]);
		const uint64_t end = be64toh(offset_table[blkno + 1]);

		/* Sanity-check the offsets.  */
		if (start != offset)
			errx(1, "strange offset for block %"PRIu32": %"PRIu64,
			    blkno, start);
		/* XXX compression ratio bound */
		__CTASSERT(MAX_BLOCKSIZE <= (SIZE_MAX / 2));
		if ((2 * (size_t)blocksize) <= (end - start))
			errx(1, "block %"PRIu32" too large: %"PRIu64" bytes",
			    blkno, (end - start));
		assert(offset <= MIN(OFF_MAX, UINT64_MAX));
		if ((MIN(OFF_MAX, UINT64_MAX) - offset) < (end - start))
			errx(1, "block %"PRIu32" overflows offset:"
			    " %"PRIu64" + %"PRIu64,
			    blkno, offset, (end - start));

		/* Read the compressed block.  */
		const ssize_t n_read = read(cloop2_fd, compbuf, (end - start));
		if (n_read == -1)
			err(1, "read block %"PRIu32, blkno);
		assert(n_read >= 0);
		if ((size_t)n_read != (end - start))
			errx(1, "partial read of block %"PRIu32": %zu != %zu",
			    blkno, (size_t)n_read, (size_t)(end - start));

		/* Uncompress the block.  */
		const unsigned long complen = (end - start);
		unsigned long uncomplen = blocksize;
		const int zerror = uncompress(uncompbuf, &uncomplen, compbuf,
		    complen);
		if (zerror != Z_OK)
			errx(1, "block %"PRIu32" decompression failure (%d)"
			    ": %s", blkno, zerror, zError(zerror));

		/* Sanity-check the uncompressed length.  */
		assert(uncomplen <= blocksize);
		if (((blkno + 1) < n_blocks) && (uncomplen != blocksize))
			errx(1, "truncated non-final block %"PRIu32
			    ": %lu bytes", blkno, uncomplen);

		/* Write the uncompressed block.  */
		const ssize_t n_written = write(image_fd, uncompbuf,
		    uncomplen);
		if (n_written == -1)
			err(1, "write block %"PRIu32, blkno);
		assert(n_written >= 0);
		if ((size_t)n_written != uncomplen)
			errx(1, "partial write of block %"PRIu32": %zu != %lu",
			    blkno, (size_t)n_written, uncomplen);

		/* Advance our position.  */
		assert((size_t)n_read <= (MIN(OFF_MAX, UINT64_MAX) - offset));
		offset += (size_t)n_read;
	}

	/* Free the offset table and compression buffers.  */
	free(offset_table);
	free(uncompbuf);
	free(compbuf);

	/* Close the files.  */
	if (close(image_fd) == -1)
		warn("close(image fd)");
	if (close(cloop2_fd) == -1)
		warn("close(cloop2 fd)");

	return 0;
}
