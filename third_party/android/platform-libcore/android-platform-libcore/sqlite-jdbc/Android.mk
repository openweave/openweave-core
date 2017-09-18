
sqlite_jdbc_src_files := \
    src/main/native/sqlite_jni.c
sqlite_jdbc_local_c_includes := \
    $(JNI_H_INCLUDE) \
    external/sqlite/dist


LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(sqlite_jdbc_src_files)
LOCAL_C_INCLUDES += $(sqlite_jdbc_local_c_includes)
LOCAL_SHARED_LIBRARIES += libsqlite
LOCAL_MODULE_TAGS := optional
# This name is dictated by the fact that the SQLite code calls loadLibrary("sqlite_jni").
LOCAL_MODULE := libsqlite_jni
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)

ifeq ($(WITH_HOST_DALVIK),true)
    include $(CLEAR_VARS)
    LOCAL_SRC_FILES := $(sqlite_jdbc_src_files)
    LOCAL_C_INCLUDES += $(sqlite_jdbc_local_c_includes)
    LOCAL_SHARED_LIBRARIES += libsqlite
    LOCAL_MODULE_TAGS := optional
    LOCAL_MODULE := libsqlite_jni
    LOCAL_PRELINK_MODULE := false
    include $(BUILD_HOST_SHARED_LIBRARY)
endif
