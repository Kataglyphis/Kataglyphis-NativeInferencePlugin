LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := kataglyphis_native
LOCAL_SRC_FILES := native_entry_point.cpp
LOCAL_LDLIBS    := -llog
include $(BUILD_SHARED_LIBRARY)