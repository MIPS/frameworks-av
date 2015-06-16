LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/deblock.cpp \
 	src/dpb.cpp \
 	src/fmo.cpp \
 	src/mb_access.cpp \
 	src/reflist.cpp

ifeq ($(TARGET_ARCH),$(filter $(TARGET_ARCH),mips32 mips64))
    ifeq ($(ARCH_MIPS_HAVE_MSA),true)
        LOCAL_SRC_FILES  += src/mips/msa/deblock_msa.cpp
    endif
endif

LOCAL_MODULE := libstagefright_avc_common

LOCAL_CFLAGS := -DOSCL_EXPORT_REF= -DOSCL_IMPORT_REF=

ifeq ($(TARGET_ARCH),$(filter $(TARGET_ARCH),mips32 mips64))
    ifeq ($(ARCH_MIPS_HAVE_MSA),true)
        LOCAL_CFLAGS += -DH264ENC_MSA
        LOCAL_CFLAGS += -mmsa -msched-weight -funroll-loops
    endif
endif

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/src \
 	$(LOCAL_PATH)/include

LOCAL_CFLAGS += -Werror

ifeq ($(TARGET_ARCH),$(filter $(TARGET_ARCH),mips32 mips64))
    ifeq ($(ARCH_MIPS_HAVE_MSA),true)
        LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/mips/msa
    endif
endif

include $(BUILD_SHARED_LIBRARY)
