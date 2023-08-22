include Makefile.sources

AM_CFLAGS = $(filter-out -Wpointer-arith, $(WARN_CFLAGS))

AM_CFLAGS += \
	-I$(top_srcdir)/include/drm \
	-I$(top_srcdir)/libkms/ \
	-I$(top_srcdir)

PLANETEST_COMMON_LDADD = \
	$(top_builddir)/libdrm.la \
	$(top_builddir)/libkms/libkms.la \
	-lpthread

if HAVE_INSTALL_TESTS
bin_PROGRAMS = \
	atomictest \
	planetest
else
noinst_PROGRAMS = \
	atomictest \
	planetest
endif

atomictest_CFLAGS=-DUSE_ATOMIC_API ${AM_CFLAGS}
atomictest_SOURCES=${PLANETEST_COMMON_FILES} ${ATOMICTEST_FILES}
planetest_SOURCES=${PLANETEST_COMMON_FILES} ${PLANETEST_FILES}

atomictest_LDADD=${PLANETEST_COMMON_LDADD}
planetest_LDADD=${PLANETEST_COMMON_LDADD}
