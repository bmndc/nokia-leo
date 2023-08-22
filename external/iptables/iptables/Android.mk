LOCAL_PATH:= $(call my-dir)
#----------------------------------------------------------------
# iptables


include $(CLEAR_VARS)

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/../include/

LOCAL_CFLAGS:=-DNO_SHARED_LIBS=1
LOCAL_CFLAGS+=-DALL_INCLUSIVE
LOCAL_CFLAGS+=-DXTABLES_INTERNAL
LOCAL_CFLAGS+=-D_LARGEFILE_SOURCE=1 -D_LARGE_FILES -D_FILE_OFFSET_BITS=64 -D_REENTRANT -DENABLE_IPV4
# Accommodate arm-eabi-4.4.3 tools that don't set __ANDROID__
LOCAL_CFLAGS+=-D__ANDROID__
LOCAL_CFLAGS += -Wno-sign-compare -Wno-pointer-arith

LOCAL_SRC_FILES:= \
	xtables-multi.c iptables-xml.c xshared.c \
	iptables-save.c iptables-restore.c \
	iptables-standalone.c iptables.c

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE:=iptables

LOCAL_STATIC_LIBRARIES := \
	libext \
	libext4 \
	libip4tc \
	libxtables

include $(BUILD_EXECUTABLE)

#----------------------------------------------------------------
# ip6tables
include $(CLEAR_VARS)

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/../include/

LOCAL_CFLAGS:=-DNO_SHARED_LIBS=1
LOCAL_CFLAGS+=-DALL_INCLUSIVE
LOCAL_CFLAGS+=-DXTABLES_INTERNAL
LOCAL_CFLAGS+=-D_LARGEFILE_SOURCE=1 -D_LARGE_FILES -D_FILE_OFFSET_BITS=64 -D_REENTRANT -DENABLE_IPV6
# Accommodate arm-eabi-4.4.3 tools that don't set __ANDROID__
LOCAL_CFLAGS+=-D__ANDROID__
LOCAL_CFLAGS += -Wno-sign-compare -Wno-pointer-arith

LOCAL_SRC_FILES:= \
	xtables-multi.c iptables-xml.c xshared.c \
	ip6tables-save.c ip6tables-restore.c \
	ip6tables-standalone.c ip6tables.c

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE:=ip6tables

LOCAL_STATIC_LIBRARIES := \
	libext \
	libext6 \
	libip6tc \
	libxtables

include $(BUILD_EXECUTABLE)


#----------------------------------------------------------------
