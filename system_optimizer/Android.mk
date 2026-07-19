# Android.mk for System Optimizer
# Place this file and system_optimizer.cpp in the same folder

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# Main system optimizer executable
LOCAL_MODULE := system_optimizer
LOCAL_SRC_FILES := system_optimizer.cpp
LOCAL_C_INCLUDES := 
LOCAL_CFLAGS := -Wall -Werror -O2 -DANDROID
LOCAL_CPPFLAGS := -std=c++14
LOCAL_LDFLAGS := -llog -lz
LOCAL_SHARED_LIBRARIES := liblog libz
LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true
LOCAL_INIT_RC := system_optimizer.rc

include $(BUILD_EXECUTABLE)

# Init script for automatic optimization on boot
include $(CLEAR_VARS)
LOCAL_MODULE := system_optimizer.rc
LOCAL_SRC_FILES := system_optimizer.rc
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)
include $(BUILD_PREBUILT)

# Helper script for manual optimization
include $(CLEAR_VARS)
LOCAL_MODULE := optimize_system.sh
LOCAL_SRC_FILES := optimize_system.sh
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)
