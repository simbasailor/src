#	$NetBSD: Makefile,v 1.1 2009/12/13 17:13:47 kardel Exp $

LIBISPRIVATE=yes

LIB=parse

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/libparse

.PATH.c:	${DIST}

SRCS= parse.c          \
      parse_conf.c     \
      clk_meinberg.c   \
      clk_schmid.c     \
      clk_rawdcf.c     \
      clk_trimtsip.c   \
      clk_dcf7000.c    \
      clk_trimtaip.c   \
      clk_rcc8000.c    \
      clk_hopf6021.c   \
      clk_computime.c  \
      clk_wharton.c    \
      clk_varitext.c   \
      data_mbg.c       \
      info_trimble.c   \
      trim_info.c      \
      binio.c          \
      ieee754io.c      \
      mfp_mul.c        \
      gpstolfp.c

.include <bsd.lib.mk>
