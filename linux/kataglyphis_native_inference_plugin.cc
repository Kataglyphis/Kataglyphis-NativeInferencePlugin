#include "include/kataglyphis_native_inference/kataglyphis_native_inference_plugin.h"

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>
#include <sys/utsname.h>

#include <cstring>

#include "kataglyphis_native_inference_plugin_private.h"

#include "kataglyphis_c_api.h"

#define KATAGLYPHIS_NATIVE_INFERENCE_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), kataglyphis_native_inference_plugin_get_type(), \
                              KataglyphisNativeInferencePlugin))

struct _KataglyphisNativeInferencePlugin {
  GObject parent_instance;
};

G_DEFINE_TYPE(KataglyphisNativeInferencePlugin, kataglyphis_native_inference_plugin, g_object_get_type())

// Called when a method call is received from Flutter.
static void kataglyphis_native_inference_plugin_handle_method_call(
    KataglyphisNativeInferencePlugin* self,
    FlMethodCall* method_call) {
  g_autoptr(FlMethodResponse) response = nullptr;

  const gchar* method = fl_method_call_get_name(method_call);

  if (strcmp(method, "getPlatformVersion") == 0) {
    response = get_platform_version();
  } else if (strcmp(method, "add") == 0) {
    // Linux-style handling for an 'add' method expecting { "a": int, "b": int }
    FlValue* args = fl_method_call_get_args(method_call);
    if (args && fl_value_get_type(args) == FL_VALUE_TYPE_MAP) {
      FlValue* a_val = fl_value_lookup_string(args, "a");
      FlValue* b_val = fl_value_lookup_string(args, "b");

      if (a_val && b_val &&
          fl_value_get_type(a_val) == FL_VALUE_TYPE_INT &&
          fl_value_get_type(b_val) == FL_VALUE_TYPE_INT) {
        gint64 a = fl_value_get_int(a_val);
        gint64 b = fl_value_get_int(b_val);

        // Call the C API function from KataglyphisCppInference
        int result = kataglyphis_add((int)a, (int)b);
        
        g_autoptr(FlValue) result_value = fl_value_new_int(result);
        response = FL_METHOD_RESPONSE(fl_method_success_response_new(result_value));
      } else {
        response = FL_METHOD_RESPONSE(fl_method_error_response_new(
            "bad_args", "Expected integer 'a' and 'b' in the argument map", nullptr));
      }
    } else {
      response = FL_METHOD_RESPONSE(fl_method_error_response_new(
          "bad_args", "Expected argument map", nullptr));
    }
  } else {
    response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
  }

  fl_method_call_respond(method_call, response, nullptr);
}

FlMethodResponse* get_platform_version() {
  struct utsname uname_data = {};
  uname(&uname_data);
  g_autofree gchar *version = g_strdup_printf("Linux %s", uname_data.version);
  g_autoptr(FlValue) result = fl_value_new_string(version);
  return FL_METHOD_RESPONSE(fl_method_success_response_new(result));
}

static void kataglyphis_native_inference_plugin_dispose(GObject* object) {
  G_OBJECT_CLASS(kataglyphis_native_inference_plugin_parent_class)->dispose(object);
}

static void kataglyphis_native_inference_plugin_class_init(KataglyphisNativeInferencePluginClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = kataglyphis_native_inference_plugin_dispose;
}

static void kataglyphis_native_inference_plugin_init(KataglyphisNativeInferencePlugin* self) {}

static void method_call_cb(FlMethodChannel* channel, FlMethodCall* method_call,
                           gpointer user_data) {
  KataglyphisNativeInferencePlugin* plugin = KATAGLYPHIS_NATIVE_INFERENCE_PLUGIN(user_data);
  kataglyphis_native_inference_plugin_handle_method_call(plugin, method_call);
}

void kataglyphis_native_inference_plugin_register_with_registrar(FlPluginRegistrar* registrar) {
  KataglyphisNativeInferencePlugin* plugin = KATAGLYPHIS_NATIVE_INFERENCE_PLUGIN(
      g_object_new(kataglyphis_native_inference_plugin_get_type(), nullptr));

  g_autoptr(FlStandardMethodCodec) codec = fl_standard_method_codec_new();
  g_autoptr(FlMethodChannel) channel =
      fl_method_channel_new(fl_plugin_registrar_get_messenger(registrar),
                            "kataglyphis_native_inference",
                            FL_METHOD_CODEC(codec));
  fl_method_channel_set_method_call_handler(channel, method_call_cb,
                                            g_object_ref(plugin),
                                            g_object_unref);

  g_object_unref(plugin);
}
