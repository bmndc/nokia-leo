AM_CFLAGS = \
	$(WARN_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/rockchip \
	$(PTHREADSTUBS_CFLAGS) \
	-I$(top_srcdir)/include/drm

libdrm_rockchip_la_LTLIBRARIES = libdrm_rockchip.la
libdrm_rockchip_ladir = $(libdir)
libdrm_rockchip_la_LDFLAGS = -version-number 1:0:0 -no-undefined
libdrm_rockchip_la_LIBADD = ../libdrm.la @PTHREADSTUBS_LIBS@

libdrm_rockchip_la_SOURCES = \
	rockchip_drm.c

libdrm_rockchipincludedir = ${includedir}/libdrm
libdrm_rockchipinclude_HEADERS = rockchip_drmif.h rockchip_drm.h

pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = libdrm_rockchip.pc
