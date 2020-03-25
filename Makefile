KMOD=	pf_obsd
SRCS=	device_if.h \
	bus_if.h \
	pf.c

#.if defined(NO_LINEAR_HOOK_LOOKUP)
#CFLAGS+=	-DNO_LINEAR_HOOK_LOOKUP
#.endif

.include <bsd.kmod.mk>
