LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
 	src/adaptive_smooth_no_mmx.cpp \
 	src/bitstream.cpp \
 	src/block_idct.cpp \
 	src/cal_dc_scaler.cpp \
 	src/chvr_filter.cpp \
 	src/chv_filter.cpp \
 	src/combined_decode.cpp \
 	src/conceal.cpp \
 	src/datapart_decode.cpp \
 	src/dcac_prediction.cpp \
 	src/dec_pred_intra_dc.cpp \
 	src/deringing_chroma.cpp \
 	src/deringing_luma.cpp \
 	src/find_min_max.cpp \
 	src/get_pred_adv_b_add.cpp \
 	src/get_pred_outside.cpp \
 	src/idct.cpp \
 	src/idct_vca.cpp \
 	src/mb_motion_comp.cpp \
 	src/mb_utils.cpp \
 	src/packet_util.cpp \
 	src/post_filter.cpp \
 	src/post_proc_semaphore.cpp \
 	src/pp_semaphore_chroma_inter.cpp \
 	src/pp_semaphore_luma.cpp \
 	src/pvdec_api.cpp \
 	src/scaling_tab.cpp \
 	src/vlc_decode.cpp \
 	src/vlc_dequant.cpp \
 	src/vlc_tab.cpp \
 	src/vop.cpp \
 	src/zigzag_tab.cpp


ifeq ($(TARGET_ARCH),$(filter $(TARGET_ARCH),mips32 mips64))
    ifeq ($(ARCH_MIPS_HAVE_MSA),true)
        LOCAL_SRC_FILES  += src/mips/msa/adaptive_smooth_msa.cpp \
            src/mips/msa/chv_filter_msa.cpp \
            src/mips/msa/chvr_filter_msa.cpp \
            src/mips/msa/find_min_max_msa.cpp \
            src/mips/msa/get_pred_adv_b_add_msa.cpp \
            src/mips/msa/idct_msa.cpp \
            src/mips/msa/post_filter_msa.cpp
    endif
endif

LOCAL_MODULE := libstagefright_m4vh263dec

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/src \
	$(LOCAL_PATH)/include \
	$(TOP)/frameworks/av/media/libstagefright/include \
	$(TOP)/frameworks/native/include/media/openmax

ifeq ($(TARGET_ARCH),$(filter $(TARGET_ARCH),mips32 mips64))
    ifeq ($(ARCH_MIPS_HAVE_MSA),true)
        LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/mips/msa
    endif
endif

LOCAL_CFLAGS := -DOSCL_EXPORT_REF= -DOSCL_IMPORT_REF=

LOCAL_CFLAGS += -Werror

include $(BUILD_STATIC_LIBRARY)

################################################################################

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
        SoftMPEG4.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/src \
	$(LOCAL_PATH)/include \
        frameworks/av/media/libstagefright/include \
        frameworks/native/include/media/openmax

ifeq ($(TARGET_ARCH),$(filter $(TARGET_ARCH),mips32 mips64))
    ifeq ($(ARCH_MIPS_HAVE_MSA),true)
        LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/mips/msa
    endif
endif

LOCAL_CFLAGS := -DOSCL_EXPORT_REF= -DOSCL_IMPORT_REF=

ifeq ($(TARGET_ARCH),$(filter $(TARGET_ARCH),mips32 mips64))
    ifeq ($(ARCH_MIPS_HAVE_MSA),true)
        LOCAL_CFLAGS += -DM4VH263DEC_MSA
        LOCAL_CFLAGS += -mmsa -msched-weight -funroll-loops
    endif
endif

LOCAL_STATIC_LIBRARIES := \
        libstagefright_m4vh263dec

LOCAL_SHARED_LIBRARIES := \
        libstagefright libstagefright_omx libstagefright_foundation libutils liblog

LOCAL_MODULE := libstagefright_soft_mpeg4dec
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS += -Werror

include $(BUILD_SHARED_LIBRARY)
