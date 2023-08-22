LOCAL_PATH:= $(call my-dir)
#----------------------------------------------------------------
## extension

MY_srcdir:=$(LOCAL_PATH)
# Exclude some modules that are problematic to compile (types/header).
MY_excluded_modules:=TCPOPTSTRIP connlabel

MY_pfx_build_mod := $(patsubst ${MY_srcdir}/libxt_%.c,%,$(wildcard ${MY_srcdir}/libxt_*.c))
MY_pf4_build_mod := $(patsubst ${MY_srcdir}/libipt_%.c,%,$(wildcard ${MY_srcdir}/libipt_*.c))
MY_pf6_build_mod := $(patsubst ${MY_srcdir}/libip6t_%.c,%,$(wildcard ${MY_srcdir}/libip6t_*.c))
MY_pfx_build_mod := $(filter-out ${MY_excluded_modules} dccp ipvs,${MY_pfx_build_mod})
MY_pf4_build_mod := $(filter-out ${MY_excluded_modules} dccp ipvs,${MY_pf4_build_mod})
MY_pf6_build_mod := $(filter-out ${MY_excluded_modules} dccp ipvs,${MY_pf6_build_mod})
MY_pfx_objs      := $(patsubst %,libxt_%.o,${MY_pfx_build_mod})
MY_pf4_objs      := $(patsubst %,libipt_%.o,${MY_pf4_build_mod})
MY_pf6_objs      := $(patsubst %,libip6t_%.o,${MY_pf6_build_mod})
# libxt_recent.c:202:11: error: address of array 'info->name' will always evaluate to 'true' [-Werror,-Wpointer-bool-conversion]
MY_warnings      := \
    -Wno-unused-parameter -Wno-missing-field-initializers \
    -Wno-sign-compare -Wno-pointer-arith \
    -Wno-pointer-bool-conversion

libext_suffix :=
libext_prefix := xt
libext_build_mod := $(MY_pfx_build_mod)
include $(LOCAL_PATH)/libext.mk

libext_suffix := 4
libext_prefix := ipt
libext_build_mod := $(MY_pf4_build_mod)
include $(LOCAL_PATH)/libext.mk

libext_suffix := 6
libext_prefix := ip6t
libext_build_mod := $(MY_pf6_build_mod)
include $(LOCAL_PATH)/libext.mk


#----------------------------------------------------------------
