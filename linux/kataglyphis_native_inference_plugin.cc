#include "include/kataglyphis_native_inference/kataglyphis_native_inference_plugin.h"

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>
#include <sys/utsname.h>

#include <cstring>

#include "kataglyphis_native_inference_plugin_private.h"

#include "kataglyphis_c_api.h"
#include "my_texture.h"

#define KATAGLYPHIS_NATIVE_INFERENCE_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), kataglyphis_native_inference_plugin_get_type(), \
                              KataglyphisNativeInferencePlugin))

struct _KataglyphisNativeInferencePlugin {
  GObject parent_instance;
  char** dart_entrypoint_arguments;

  /* Channel to receive general method calls from Flutter. */
  FlMethodChannel* channel;

  /* Channel to receive texture requests from Flutter (optional, only when a view is present). */
  FlMethodChannel* texture_channel;

  /* Texture we've created (optional). */
  MyTexture* texture;

  /* The FlView associated with the registrar (may be NULL). */
  FlView* view;
};

G_DEFINE_TYPE(KataglyphisNativeInferencePlugin, kataglyphis_native_inference_plugin, g_object_get_type())

/* Forward declarations */
static FlMethodResponse* handle_create(KataglyphisNativeInferencePlugin* self,
                                      FlMethodCall* method_call);
static FlMethodResponse* handle_set_color(KataglyphisNativeInferencePlugin* self,
                                          FlMethodCall* method_call);
FlMethodResponse* get_platform_version(void);

static FlMethodResponse* handle_set_pipeline(KataglyphisNativeInferencePlugin* self,
                                             FlMethodCall* method_call) {
  if (!self->texture) {
    return FL_METHOD_RESPONSE(fl_method_error_response_new(
        "Error", "No texture created. Call 'create' first.", nullptr));
  }

  FlValue* args = fl_method_call_get_args(method_call);
  if (fl_value_get_type(args) != FL_VALUE_TYPE_STRING) {
    return FL_METHOD_RESPONSE(fl_method_error_response_new(
        "Invalid args", "Expected pipeline string", nullptr));
  }

  const gchar* pipeline_desc = fl_value_get_string(args);
  GError* error = nullptr;
  
  if (!my_texture_set_pipeline(self->texture, pipeline_desc, &error)) {
    g_autofree gchar* error_msg = g_strdup_printf(
        "Failed to set pipeline: %s", error ? error->message : "Unknown error");
    if (error) g_error_free(error);
    return FL_METHOD_RESPONSE(fl_method_error_response_new(
        "Pipeline Error", error_msg, nullptr));
  }

  return FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
}

static FlMethodResponse* handle_play(KataglyphisNativeInferencePlugin* self,
                                     FlMethodCall* method_call) {
  if (!self->texture) {
    return FL_METHOD_RESPONSE(fl_method_error_response_new(
        "Error", "No texture created", nullptr));
  }

  my_texture_play(self->texture);
  return FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
}

static FlMethodResponse* handle_pause(KataglyphisNativeInferencePlugin* self,
                                      FlMethodCall* method_call) {
  if (!self->texture) {
    return FL_METHOD_RESPONSE(fl_method_error_response_new(
        "Error", "No texture created", nullptr));
  }

  my_texture_pause(self->texture);
  return FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
}

static FlMethodResponse* handle_stop(KataglyphisNativeInferencePlugin* self,
                                     FlMethodCall* method_call) {
  if (!self->texture) {
    return FL_METHOD_RESPONSE(fl_method_error_response_new(
        "Error", "No texture created", nullptr));
  }

  my_texture_stop(self->texture);
  return FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
}

// Handle request to create the texture.
static FlMethodResponse* handle_create(KataglyphisNativeInferencePlugin* self,
                                       FlMethodCall* method_call) {
  if (self->texture != nullptr) {
    g_autoptr(FlValue) id =
        fl_value_new_int(fl_texture_get_id(FL_TEXTURE(self->texture)));
    return FL_METHOD_RESPONSE(fl_method_success_response_new(id));
  }

  FlValue* args = fl_method_call_get_args(method_call);
  if (fl_value_get_type(args) != FL_VALUE_TYPE_LIST ||
      fl_value_get_length(args) != 2) {
    return FL_METHOD_RESPONSE(fl_method_error_response_new(
        "Invalid args", "Invalid create args", nullptr));
  }
  FlValue* width_value = fl_value_get_list_value(args, 0);
  FlValue* height_value = fl_value_get_list_value(args, 1);
  if (fl_value_get_type(width_value) != FL_VALUE_TYPE_INT ||
      fl_value_get_type(height_value) != FL_VALUE_TYPE_INT) {
    return FL_METHOD_RESPONSE(fl_method_error_response_new(
        "Invalid args", "Invalid create args", nullptr));
  }

  FlEngine* engine = fl_view_get_engine(self->view);
  FlTextureRegistrar* texture_registrar =
      fl_engine_get_texture_registrar(engine);

  self->texture =
      my_texture_new(fl_value_get_int(width_value),
                     fl_value_get_int(height_value), 0x05, 0x53, 0xb1);

  // WICHTIG: TextureRegistrar setzen für GStreamer-Updates
  my_texture_set_texture_registrar(self->texture, texture_registrar);
  
  if (!fl_texture_registrar_register_texture(texture_registrar,
                                             FL_TEXTURE(self->texture))) {
    return FL_METHOD_RESPONSE(fl_method_error_response_new(
        "Error", "Failed to register texture", nullptr));
  }

  // Return the texture ID to Flutter so it can use this texture.
  g_autoptr(FlValue) id =
      fl_value_new_int(fl_texture_get_id(FL_TEXTURE(self->texture)));
  return FL_METHOD_RESPONSE(fl_method_success_response_new(id));
}

// Handle request to set the texture color.
static FlMethodResponse* handle_set_color(KataglyphisNativeInferencePlugin* self,
                                          FlMethodCall* method_call) {
  FlValue* args = fl_method_call_get_args(method_call);
  if (fl_value_get_type(args) != FL_VALUE_TYPE_LIST ||
      fl_value_get_length(args) != 3) {
    return FL_METHOD_RESPONSE(fl_method_error_response_new(
        "Invalid args", "Invalid setColor args", nullptr));
  }
  FlValue* r_value = fl_value_get_list_value(args, 0);
  FlValue* g_value = fl_value_get_list_value(args, 1);
  FlValue* b_value = fl_value_get_list_value(args, 2);
  if (fl_value_get_type(r_value) != FL_VALUE_TYPE_INT ||
      fl_value_get_type(g_value) != FL_VALUE_TYPE_INT ||
      fl_value_get_type(b_value) != FL_VALUE_TYPE_INT) {
    return FL_METHOD_RESPONSE(fl_method_error_response_new(
        "Invalid args", "Invalid setColor args", nullptr));
  }

  FlEngine* engine = fl_view_get_engine(self->view);
  FlTextureRegistrar* texture_registrar =
      fl_engine_get_texture_registrar(engine);

  // Redraw in requested color.
  my_texture_set_color(self->texture, fl_value_get_int(r_value),
                       fl_value_get_int(g_value), fl_value_get_int(b_value));

  // Notify Flutter the texture has changed.
  fl_texture_registrar_mark_texture_frame_available(texture_registrar,
                                                    FL_TEXTURE(self->texture));

  return FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
}

// Called when first Flutter frame received.
static void first_frame_cb(KataglyphisNativeInferencePlugin* self, FlView* view) {
  gtk_widget_show(gtk_widget_get_toplevel(GTK_WIDGET(view)));
}

/* Handles general plugin method calls on the single channel. */
static void kataglyphis_native_inference_plugin_handle_method_call(
    KataglyphisNativeInferencePlugin* self,
    FlMethodCall* method_call) {
  g_autoptr(FlMethodResponse) response = NULL;

  const gchar* method = fl_method_call_get_name(method_call);

  if (strcmp(method, "getPlatformVersion") == 0) {
    response = get_platform_version();
  } else if (strcmp(method, "add") == 0) {
    FlValue* args = fl_method_call_get_args(method_call);
    if (args && fl_value_get_type(args) == FL_VALUE_TYPE_MAP) {
      FlValue* a_val = fl_value_lookup_string(args, "a");
      FlValue* b_val = fl_value_lookup_string(args, "b");
      if (a_val && b_val &&
          fl_value_get_type(a_val) == FL_VALUE_TYPE_INT &&
          fl_value_get_type(b_val) == FL_VALUE_TYPE_INT) {
        gint64 a = fl_value_get_int(a_val);
        gint64 b = fl_value_get_int(b_val);
        int result = kataglyphis_add((int)a, (int)b);
        g_autoptr(FlValue) result_value = fl_value_new_int(result);
        response = FL_METHOD_RESPONSE(fl_method_success_response_new(result_value));
      } else {
        response = FL_METHOD_RESPONSE(fl_method_error_response_new(
            "bad_args", "Expected integer 'a' and 'b' in the argument map", NULL));
      }
    } else {
      response = FL_METHOD_RESPONSE(fl_method_error_response_new(
          "bad_args", "Expected argument map", NULL));
    }
  } else if (g_str_equal(method, "create")) {
    /* IMPORTANT: return the FlMethodResponse from the helper,
       do NOT fl_method_call_respond() here: let the single call below do it. */
    response = handle_create(self, method_call);
  } else if (g_str_equal(method, "setColor")) {
    response = handle_set_color(self, method_call);
  } else if (g_str_equal(method, "setPipeline")) {
    response = handle_set_pipeline(self, method_call);
  } else if (g_str_equal(method, "play")) {
    response = handle_play(self, method_call);
  } else if (g_str_equal(method, "pause")) {
    response = handle_pause(self, method_call);
  } else if (g_str_equal(method, "stop")) {
    response = handle_stop(self, method_call);
  } else {
    response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
  }

  /* Single call to respond. response must be a valid FlMethodResponse*. */
  fl_method_call_respond(method_call, response, NULL);
}


FlMethodResponse* get_platform_version() {
  struct utsname uname_data = {};
  uname(&uname_data);
  g_autofree gchar *version = g_strdup_printf("Linux %s", uname_data.version);
  g_autoptr(FlValue) result = fl_value_new_string(version);
  return FL_METHOD_RESPONSE(fl_method_success_response_new(result));
}

static void kataglyphis_native_inference_plugin_dispose(GObject *object) {
  KataglyphisNativeInferencePlugin* self = KATAGLYPHIS_NATIVE_INFERENCE_PLUGIN(object);

  g_clear_pointer(&self->dart_entrypoint_arguments, g_strfreev);
  g_clear_object(&self->channel);
  g_clear_object(&self->texture_channel);
  g_clear_object(&self->texture);
  if (self->view) {
    g_clear_object(&self->view);
  }

  G_OBJECT_CLASS(kataglyphis_native_inference_plugin_parent_class)->dispose(object);
}

static void kataglyphis_native_inference_plugin_class_init(
    KataglyphisNativeInferencePluginClass *klass) {
  G_OBJECT_CLASS(klass)->dispose = kataglyphis_native_inference_plugin_dispose;
}

static void kataglyphis_native_inference_plugin_init(
    KataglyphisNativeInferencePlugin *self) {
  self->dart_entrypoint_arguments = nullptr;
  self->channel = nullptr;
  self->texture_channel = nullptr;
  self->texture = nullptr;
  self->view = nullptr;
}

static void method_call_cb(FlMethodChannel* channel, FlMethodCall* method_call,
                           gpointer user_data) {
  KataglyphisNativeInferencePlugin* plugin = KATAGLYPHIS_NATIVE_INFERENCE_PLUGIN(user_data);
  kataglyphis_native_inference_plugin_handle_method_call(plugin, method_call);
}

void kataglyphis_native_inference_plugin_register_with_registrar(FlPluginRegistrar* registrar) {
  KataglyphisNativeInferencePlugin* plugin = KATAGLYPHIS_NATIVE_INFERENCE_PLUGIN(
      g_object_new(kataglyphis_native_inference_plugin_get_type(), nullptr));

  /* Save a reference to the FlView if present (texture features require a view). */
  FlView* view = fl_plugin_registrar_get_view(registrar);
  if (view) {
    plugin->view = FL_VIEW(g_object_ref(view));
  }
  
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
