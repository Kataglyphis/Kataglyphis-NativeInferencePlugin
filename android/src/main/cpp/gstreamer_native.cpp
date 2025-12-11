#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/log.h>

#include <gst/gst.h>
#include <gst/video/videooverlay.h>

#include <mutex>
#include <string>

namespace {
constexpr const char *kTag = "KataglyphisGStreamer";

struct GstState {
    GstElement *pipeline = nullptr;
    ANativeWindow *window = nullptr;
    bool initialized = false;
};

std::mutex g_mutex;
GstState g_state;

void logError(const char *msg) {
    __android_log_print(ANDROID_LOG_ERROR, kTag, "%s", msg);
}

void logInfo(const char *msg) {
    __android_log_print(ANDROID_LOG_INFO, kTag, "%s", msg);
}

void ensure_gst_init_unlocked() {
    if (g_state.initialized) return;
    int argc = 0;
    char **argv = nullptr;
    gst_init(&argc, &argv);
    g_state.initialized = true;
    logInfo("GStreamer initialized");
}

void release_pipeline_unlocked() {
    if (g_state.pipeline) {
        gst_element_set_state(g_state.pipeline, GST_STATE_NULL);
        gst_object_unref(g_state.pipeline);
        g_state.pipeline = nullptr;
    }
}

void release_window_unlocked() {
    if (g_state.window) {
        ANativeWindow_release(g_state.window);
        g_state.window = nullptr;
    }
}

bool bind_overlay(GstElement *pipeline, ANativeWindow *window) {
    GstElement *overlay = nullptr;
    GstIterator *it = gst_bin_iterate_elements(GST_BIN(pipeline));
    GValue item = G_VALUE_INIT;
    while (gst_iterator_next(it, &item) == GST_ITERATOR_OK) {
        GstElement *element = GST_ELEMENT(g_value_get_object(&item));
        if (GST_IS_VIDEO_OVERLAY(element)) {
            overlay = element;
            g_value_reset(&item);
            break;
        }
        g_value_reset(&item);
    }
    gst_iterator_free(it);

    if (!overlay) return false;

    gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(overlay), reinterpret_cast<guintptr>(window));
    return true;
}

GstElement *find_factory(GstElement *pipeline, const char *factoryName) {
    GstElement *found = nullptr;
    GstIterator *it = gst_bin_iterate_elements(GST_BIN(pipeline));
    GValue item = G_VALUE_INIT;
    while (gst_iterator_next(it, &item) == GST_ITERATOR_OK) {
        GstElement *element = GST_ELEMENT(g_value_get_object(&item));
        const gchar *factory = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(gst_element_get_factory(element)));
        if (factory && std::string(factory) == factoryName) {
            found = GST_ELEMENT(gst_object_ref(element));
            g_value_reset(&item);
            break;
        }
        g_value_reset(&item);
    }
    gst_iterator_free(it);
    return found;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_kataglyphis_1native_1inference_GStreamerNative_init(
        JNIEnv *env,
        jclass /*clazz*/,
        jobject context) {
    std::lock_guard<std::mutex> lock(g_mutex);
    (void)env;
    (void)context;
    ensure_gst_init_unlocked();
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_kataglyphis_1native_1inference_GStreamerNative_create(
        JNIEnv *env,
        jclass /*clazz*/,
        jobject surface,
        jint /*width*/, jint /*height*/) {
    std::lock_guard<std::mutex> lock(g_mutex);
    ensure_gst_init_unlocked();

    release_pipeline_unlocked();
    release_window_unlocked();

    if (!surface) {
        logError("Surface is null");
        return JNI_FALSE;
    }

    g_state.window = ANativeWindow_fromSurface(env, surface);
    if (!g_state.window) {
        logError("Failed to acquire ANativeWindow");
        return JNI_FALSE;
    }
    return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_kataglyphis_1native_1inference_GStreamerNative_setPipeline(
        JNIEnv *env,
        jclass /*clazz*/,
        jstring pipelineStr) {
    std::lock_guard<std::mutex> lock(g_mutex);
    ensure_gst_init_unlocked();

    if (!g_state.window) {
        logError("Window not created before setPipeline");
        return JNI_FALSE;
    }

    const char *cStr = env->GetStringUTFChars(pipelineStr, nullptr);
    std::string pipelineDesc(cStr ? cStr : "");
    if (cStr) env->ReleaseStringUTFChars(pipelineStr, cStr);

    if (pipelineDesc.empty()) {
        logError("Pipeline string is empty");
        return JNI_FALSE;
    }

    release_pipeline_unlocked();

    GError *err = nullptr;
    GstElement *pipeline = gst_parse_launch(pipelineDesc.c_str(), &err);
    if (!pipeline) {
        logError("gst_parse_launch returned null");
        if (err) {
            __android_log_print(ANDROID_LOG_ERROR, kTag, "parse error: %s", err->message);
            g_error_free(err);
        }
        return JNI_FALSE;
    }

    if (!bind_overlay(pipeline, g_state.window)) {
        logError("No video overlay sink found in pipeline");
        gst_object_unref(pipeline);
        return JNI_FALSE;
    }

    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PAUSED);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        logError("Failed to preroll pipeline");
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        return JNI_FALSE;
    }

    g_state.pipeline = pipeline;
    return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_kataglyphis_1native_1inference_GStreamerNative_play(
        JNIEnv * /*env*/,
        jclass /*clazz*/) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_state.pipeline) return JNI_FALSE;
    return gst_element_set_state(g_state.pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_kataglyphis_1native_1inference_GStreamerNative_pause(
        JNIEnv * /*env*/,
        jclass /*clazz*/) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_state.pipeline) return JNI_FALSE;
    return gst_element_set_state(g_state.pipeline, GST_STATE_PAUSED) != GST_STATE_CHANGE_FAILURE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_kataglyphis_1native_1inference_GStreamerNative_stop(
        JNIEnv * /*env*/,
        jclass /*clazz*/) {
    std::lock_guard<std::mutex> lock(g_mutex);
    release_pipeline_unlocked();
    return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_kataglyphis_1native_1inference_GStreamerNative_setColor(
        JNIEnv * /*env*/,
        jclass /*clazz*/,
        jint r,
        jint g,
        jint b) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_state.pipeline) return JNI_FALSE;

    GstElement *src = find_factory(g_state.pipeline, "videotestsrc");
    if (!src) return JNI_FALSE;

    guint32 color = (0xFFu << 24) | ((static_cast<guint32>(r) & 0xFFu) << 16) |
                    ((static_cast<guint32>(g) & 0xFFu) << 8) |
                    (static_cast<guint32>(b) & 0xFFu);
    g_object_set(src, "foreground-color", color, NULL);
    gst_object_unref(src);
    return JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_kataglyphis_1native_1inference_GStreamerNative_dispose(
        JNIEnv * /*env*/,
        jclass /*clazz*/) {
    std::lock_guard<std::mutex> lock(g_mutex);
    release_pipeline_unlocked();
    release_window_unlocked();
    g_state.initialized = true; // keep gst initialized to avoid re-init issues
}

} // namespace
