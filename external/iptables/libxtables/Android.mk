LOCAL_PATH:= $(call my-dir)
#----------------------------------------------------------------
# libxtables

include $(CLEAR_VARS)

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/../include/ \
	$(LOCAL_PATH)/../iptables/ \
	$(LOCAL_PATH)/.. \

LOCAL_CFLAGS:=-DNO_SHARED_LIBS=1
LOCAL_CFLAGS+=-DXTABLES_INTERNAL
LOCAL_CFLAGS+=-DXTABLES_LIBDIR=\"xtables_libdir_not_used\"
LOCAL_CFLAGS+=-D_LARGEFILE_SOURCE=1 -D_LARGE_FILES -D_FILE_OFFSET_BITS=64 -D_REENTRANT -DENABLE_IPV4 -DENABLE_IPV6
# Accommodate arm-eabi-4.4.3 tools that don't set __ANDROID__
LOCAL_CFLAGS+=-D__ANDROID__
LOCAL_CFLAGS += -Wno-sign-compare -Wno-pointer-arith -Wno-type-limits -Wno-missing-field-initializers -Wno-unused-parameter -Wno-clobbered

LOCAL_LDFLAGS:=-version-info 10:0:0
LOCAL_SRC_FILES:= \
	xtables.c xtoptions.c

LOCAL_MODULE:=libxtables

include $(BUILD_STATIC_LIBRARY)

