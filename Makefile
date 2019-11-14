KMOD=	pf_obsd
SRCS=	pf.c

#.if defined(NO_LINEAR_HOOK_LOOKUP)
#CFLAGS+=	-DNO_LINEAR_HOOK_LOOKUP
#.endif

.include <bsd.kmod.mk>
