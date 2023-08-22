LOCAL_PATH:= $(call my-dir)

#----------------------------------------------------------------
# libip4tc

include $(CLEAR_VARS)

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/../include/

# Accommodate arm-eabi-4.4.3 tools that don't set __ANDROID__
LOCAL_CFLAGS:=-D__ANDROID__
LOCAL_CFLAGS+=-D_LARGEFILE_SOURCE=1 -D_LARGE_FILES -D_FILE_OFFSET_BITS=64 -D_REENTRANT -DENABLE_IPV4 -DENABLE_IPV6
LOCAL_CFLAGS += -Wno-pointer-arith -Wno-unused-parameter -Wno-sign-compare -Wno-pointer-sign

LOCAL_SRC_FILES:= \
	libip4tc.c \


LOCAL_LDFLAGS:=-version-info 1:0:1
LOCAL_MODULE_TAGS:=
LOCAL_MODULE:=libip4tc

include $(BUILD_STATIC_LIBRARY)


#----------------------------------------------------------------
# libip6tc

include $(CLEAR_VARS)

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/../include/

# Accommodate arm-eabi-4.4.3 tools that don't set __ANDROID__
LOCAL_CFLAGS:=-D__ANDROID__
LOCAL_CFLAGS+=-D_LARGEFILE_SOURCE=1 -D_LARGE_FILES -D_FILE_OFFSET_BITS=64 -D_REENTRANT -DENABLE_IPV4 -DENABLE_IPV6
LOCAL_CFLAGS += -Wno-pointer-arith -Wno-unused-parameter -Wno-sign-compare -Wno-pointer-sign

LOCAL_SRC_FILES:= \
	libip6tc.c \


LOCAL_LDFLAGS:=-version-info 1:0:1
LOCAL_MODULE_TAGS:=
LOCAL_MODULE:=libip6tc

include $(BUILD_STATIC_LIBRARY)

#----------------------------------------------------------------
