CFLAGS_SHARED_ATTR	+= -DMDSO_PRE_ALPHA -DMDSO_EXPORT
CFLAGS_STATIC_ATTR	+= -DMDSO_PRE_ALPHA -DMDSO_STATIC
CFLAGS_APP_ATTR		+= -DMDSO_APP

src/driver/mdso_driver_ctx.o:	version.tag
src/driver/mdso_driver_ctx.lo:	version.tag
