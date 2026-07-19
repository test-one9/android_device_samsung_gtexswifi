#
# Bionic Patches for LineageOS 16
# Combined build file for all patches
#

LOCAL_PATH := $(call my-dir)

# ============================================================================
# Include directories
# ============================================================================

BIONIC_PATCHES_INCLUDE := $(LOCAL_PATH)/include

# ============================================================================
# Common C library patches
# ============================================================================

include $(CLEAR_VARS)

LOCAL_MODULE := libbionic_patches_c
LOCAL_MODULE_TAGS := optional
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

LOCAL_SRC_FILES := \
    c/libc_patches.c \
    c/threads_patches.c

LOCAL_CFLAGS := \
    -Wall \
    -Wextra \
    -Werror \
    -Wno-unused-parameter \
    -DHAVE_GETTID \
    -std=c99

LOCAL_C_INCLUDES := \
    bionic/libc/include \
    bionic/libc/private

include $(BUILD_STATIC_LIBRARY)

# ============================================================================
# Linker patches
# ============================================================================

include $(CLEAR_VARS)

LOCAL_MODULE := libbionic_patches_linker
LOCAL_MODULE_TAGS := optional
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

LOCAL_SRC_FILES := \
    c/linker_patches.c

LOCAL_CFLAGS := \
    -Wall \
    -Wextra \
    -Werror \
    -Wno-unused-parameter \
    -std=c99 \
    -D__LINKER_MODIFICATIONS__

LOCAL_C_INCLUDES := \
    bionic/linker \
    bionic/linker/include

include $(BUILD_STATIC_LIBRARY)

# ============================================================================
# Sparse image patches
# ============================================================================

include $(CLEAR_VARS)

LOCAL_MODULE := libbionic_patches_sparse
LOCAL_MODULE_TAGS := optional
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

LOCAL_SRC_FILES := \
    c/sparse_patches.c

LOCAL_CFLAGS := \
    -Wall \
    -Wextra \
    -Werror \
    -std=c99 \
    -DSPARSE_FORMAT_PATCHES

LOCAL_C_INCLUDES := \
    system/core/libsparse/include

include $(BUILD_STATIC_LIBRARY)

# ============================================================================
# SurfaceFlinger patches (C++)
# ============================================================================

include $(CLEAR_VARS)

LOCAL_MODULE := libbionic_patches_surfaceflinger
LOCAL_MODULE_TAGS := optional
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

LOCAL_SRC_FILES := \
    cpp/surfaceflinger_patches.cpp \
    cpp/surfaceflinger_hwc1_patches.cpp \
    cpp/client_patches.cpp \
    cpp/displaydevice_patches.cpp

LOCAL_CFLAGS := \
    -Wall \
    -Wextra \
    -Werror \
    -std=c++14 \
    -fvisibility=hidden \
    -DLOG_TAG=\"SurfaceFlinger\" \
    -DENABLE_FENCE_TRACKING

LOCAL_CPPFLAGS := \
    -std=c++14 \
    -fno-rtti \
    -fno-exceptions

LOCAL_C_INCLUDES := \
    frameworks/native/include \
    frameworks/native/services/surfaceflinger \
    frameworks/native/services/surfaceflinger/DisplayHardware \
    system/core/include \
    system/core/include/utils \
    hardware/libhardware/include \
    hardware/libhardware/include/hardware

LOCAL_SHARED_LIBRARIES := \
    libbinder \
    libcutils \
    libEGL \
    libGLESv2 \
    libgui \
    libhardware \
    liblog \
    libutils \
    libui

LOCAL_STATIC_LIBRARIES := \
    libbionic_patches_c \
    libbionic_patches_sparse

include $(BUILD_STATIC_LIBRARY)

# ============================================================================
# Binder patches (C++)
# ============================================================================

include $(CLEAR_VARS)

LOCAL_MODULE := libbionic_patches_binder
LOCAL_MODULE_TAGS := optional
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

LOCAL_SRC_FILES := \
    cpp/binder_patches.cpp

LOCAL_CFLAGS := \
    -Wall \
    -Wextra \
    -Werror \
    -std=c++14

LOCAL_C_INCLUDES := \
    frameworks/native/include \
    frameworks/native/libs/binder \
    system/core/include

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils \
    libutils

include $(BUILD_STATIC_LIBRARY)

# ============================================================================
# Combined library (includes all patches)
# ============================================================================

include $(CLEAR_VARS)

LOCAL_MODULE := libbionic_patches
LOCAL_MODULE_TAGS := optional
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

LOCAL_WHOLE_STATIC_LIBRARIES := \
    libbionic_patches_c \
    libbionic_patches_linker \
    libbionic_patches_sparse \
    libbionic_patches_surfaceflinger \
    libbionic_patches_binder

LOCAL_CFLAGS := -DUSE_BIONIC_PATCHES

include $(BUILD_SHARED_LIBRARY)

# ============================================================================
# Install patches to system
# ============================================================================

PRODUCT_PACKAGES += \
    libbionic_patches

# ============================================================================
# Default build flags
# ============================================================================

VSYNC_EVENT_PHASE_OFFSET_NS ?= 0
SF_VSYNC_EVENT_PHASE_OFFSET_NS ?= 0

# SPRD-specific flags
SPRD_ENABLE_FRAMEBUFFER_AFBC ?= 0
