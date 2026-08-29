LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := oxsimple
LOCAL_CPP_FEATURES := exceptions

# --- Sources (простой чит: рендер + ImGui + данные, без lua/сканов) ---
LOCAL_SRC_FILES := \
    src/main.cpp \
    src/proc.cpp \
    src/game.cpp \
    src/menu.cpp \
    src/Android_draw/draw.cpp \
    src/Android_touch/TouchHelperA.cpp \
    src/ImGui/imgui.cpp \
    src/ImGui/imgui_demo.cpp \
    src/ImGui/imgui_draw.cpp \
    src/ImGui/imgui_tables.cpp \
    src/ImGui/imgui_widgets.cpp \
    src/ImGui/backends/imgui_impl_android.cpp \
    src/ImGui/backends/imgui_impl_opengl3.cpp

# --- Include paths ---
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/src \
    $(LOCAL_PATH)/include/ImGui \
    $(LOCAL_PATH)/include/ImGui/backends \
    $(LOCAL_PATH)/include/ImGui/font \
    $(LOCAL_PATH)/include/native_surface \
    $(LOCAL_PATH)/include/Android_draw \
    $(LOCAL_PATH)/include/Android_touch

LOCAL_CFLAGS := \
    -DUSE_OPENGL \
    -Wno-unused-result \
    -Wno-deprecated-declarations \
    -Wno-format-security \
    -Wno-format \
    -Wno-format-pedantic

LOCAL_CPPFLAGS := -std=c++17 -fno-rtti -fexceptions

LOCAL_LDLIBS := \
    -llog \
    -landroid \
    -lEGL \
    -lGLESv3 \
    -ldl \
    -lm

# Standalone PIE executable, запускается под su на устройстве.
include $(BUILD_EXECUTABLE)
