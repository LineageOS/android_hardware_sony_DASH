LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(SOMC_CFG_SENSORS_COMPASS_AK8963),yes)

LOCAL_MODULE := libsensors_akm8963
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES += ak8963_dummy.c
include $(BUILD_SHARED_LIBRARY)

endif
