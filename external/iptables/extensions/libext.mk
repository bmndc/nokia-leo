include $(CLEAR_VARS)

LOCAL_MODULE_TAGS:=
LOCAL_MODULE:=libext$(libext_suffix)

# LOCAL_MODULE_CLASS must be defined before calling $(local-generated-sources-dir)
#
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
MY_gen := $(call local-generated-sources-dir)

# LOCAL_PATH needed because of dirty #include "blabla.c"
LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/../include/ \
	$(LOCAL_PATH)/.. \
	$(MY_gen) \
	$(LOCAL_PATH)

LOCAL_CFLAGS:=-DNO_SHARED_LIBS=1
# The $* does not work as expected. It ends up empty. Even with SECONDEXPANSION.
# LOCAL_CFLAGS+=-D_INIT=lib$*_init
LOCAL_CFLAGS+=-DXTABLES_INTERNAL
LOCAL_CFLAGS+=-D_LARGEFILE_SOURCE=1 -D_LARGE_FILES -D_FILE_OFFSET_BITS=64 -D_REENTRANT -DENABLE_IPV4 -DENABLE_IPV6
# Accommodate arm-eabi-4.4.3 tools that don't set __ANDROID__
LOCAL_CFLAGS+=-D__ANDROID__
LOCAL_CFLAGS += $(MY_warnings)

MY_GEN_INITEXT:= $(MY_gen)/initext.c
$(MY_GEN_INITEXT): MY_initext_func := $(addprefix $(libext_prefix)_,$(libext_build_mod))
$(MY_GEN_INITEXT): MY_suffix := $(libext_suffix)
$(MY_GEN_INITEXT):
	@mkdir -p $(dir $@)
	@( \
	echo "" >$@; \
	for i in $(MY_initext_func); do \
		echo "extern void lib$${i}_init(void);" >>$@; \
	done; \
	echo "void init_extensions$(MY_suffix)(void);" >>$@; \
	echo "void init_extensions$(MY_suffix)(void)" >>$@; \
	echo "{" >>$@; \
	for i in $(MY_initext_func); do \
		echo " ""lib$${i}_init();" >>$@; \
	done; \
	echo "}" >>$@; \
	);

MY_lib_sources:= \
	$(patsubst %,$(LOCAL_PATH)/lib$(libext_prefix)_%.c,$(libext_build_mod))

MY_gen_lib_sources:= $(patsubst $(LOCAL_PATH)/%,${MY_gen}/%,${MY_lib_sources})

${MY_gen_lib_sources}: PRIVATE_PATH := $(LOCAL_PATH)
${MY_gen_lib_sources}: PRIVATE_CUSTOM_TOOL = $(PRIVATE_PATH)/filter_init $(PRIVATE_PATH)/$(notdir $@) > $@
${MY_gen_lib_sources}: PRIVATE_MODULE := $(LOCAL_MODULE)
${MY_gen_lib_sources}: PRIVATE_C_INCLUDES := $(LOCAL_C_INCLUDES)
${MY_gen_lib_sources}: ${MY_gen}/% : $(LOCAL_PATH)/%
	$(transform-generated-source)

$(MY_GEN_INITEXT): $(MY_gen_lib_sources)

LOCAL_GENERATED_SOURCES:= $(MY_GEN_INITEXT) $(MY_gen_lib_sources)

include $(BUILD_STATIC_LIBRARY)
