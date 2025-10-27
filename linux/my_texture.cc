#include "my_texture.h"
#include <string.h>

struct _MyTexture {
  FlPixelBufferTexture parent_instance;

  uint32_t width;
  uint32_t height;
  uint8_t* buffer;
  
  // GStreamer components
  GstElement* pipeline;
  GstElement* appsink;
  GstSample* last_sample;
  
  // Callback for texture updates
  FlTextureRegistrar* texture_registrar;
};

G_DEFINE_TYPE(MyTexture, my_texture, fl_pixel_buffer_texture_get_type())

// Forward declarations
static GstFlowReturn on_new_sample(GstAppSink* appsink, gpointer user_data);

static void my_texture_dispose(GObject* object) {
  MyTexture* self = MY_TEXTURE(object);

  if (self->pipeline) {
    gst_element_set_state(self->pipeline, GST_STATE_NULL);
    gst_object_unref(self->pipeline);
    self->pipeline = nullptr;
  }

  if (self->last_sample) {
    gst_sample_unref(self->last_sample);
    self->last_sample = nullptr;
  }

  if (self->buffer) {
    free(self->buffer);
    self->buffer = nullptr;
  }

  G_OBJECT_CLASS(my_texture_parent_class)->dispose(object);
}

static gboolean my_texture_copy_pixels(FlPixelBufferTexture* texture,
                                       const uint8_t** out_buffer,
                                       uint32_t* width, uint32_t* height,
                                       GError** error) {
  MyTexture* self = MY_TEXTURE(texture);
  
  if (self->last_sample) {
    GstBuffer* buffer = gst_sample_get_buffer(self->last_sample);
    GstMapInfo map;
    
    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
      // Kopieren Sie die Daten in den internen Buffer
      memcpy(self->buffer, map.data, map.size);
      gst_buffer_unmap(buffer, &map);
    }
  }
  
  *out_buffer = self->buffer;
  *width = self->width;
  *height = self->height;
  return TRUE;
}

void my_texture_set_color(MyTexture* self, uint8_t r, uint8_t g, uint8_t b) {
  g_return_if_fail(MY_IS_TEXTURE(self));
  if (!self->buffer) return;

  const uint32_t pixels = self->width * self->height;
  for (uint32_t i = 0; i < pixels; ++i) {
    uint8_t* p = self->buffer + i * 4;
    p[0] = r;
    p[1] = g;
    p[2] = b;
    p[3] = 255;
  }

  if (self->texture_registrar) {
    fl_texture_registrar_mark_texture_frame_available(
        self->texture_registrar, FL_TEXTURE(self));
  }
}

static void my_texture_class_init(MyTextureClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = my_texture_dispose;
  FL_PIXEL_BUFFER_TEXTURE_CLASS(klass)->copy_pixels = my_texture_copy_pixels;
}

static void my_texture_init(MyTexture* self) {
  self->pipeline = nullptr;
  self->appsink = nullptr;
  self->last_sample = nullptr;
  self->buffer = nullptr;
  self->texture_registrar = nullptr;
}

MyTexture* my_texture_new(uint32_t width, uint32_t height, uint8_t r, uint8_t g, uint8_t b) {
  MyTexture* self = MY_TEXTURE(g_object_new(my_texture_get_type(), nullptr));
  self->width = width;
  self->height = height;
  self->buffer = static_cast<uint8_t*>(malloc(width * height * 4));
  memset(self->buffer, 0, width * height * 4);

  gst_init(nullptr, nullptr);

  my_texture_set_color(self, r, g, b);
  return self;
}

// Callback wenn ein neues Frame verfügbar ist
static GstFlowReturn on_new_sample(GstAppSink* appsink, gpointer user_data) {
  MyTexture* self = MY_TEXTURE(user_data);
  
  GstSample* sample = gst_app_sink_pull_sample(appsink);
  if (!sample) {
    return GST_FLOW_ERROR;
  }
  
  // Altes Sample freigeben
  if (self->last_sample) {
    gst_sample_unref(self->last_sample);
  }
  
  self->last_sample = sample;
  
  // Flutter benachrichtigen, dass ein neues Frame verfügbar ist
  if (self->texture_registrar) {
    fl_texture_registrar_mark_texture_frame_available(
        self->texture_registrar, FL_TEXTURE(self));
  }
  
  return GST_FLOW_OK;
}

gboolean my_texture_set_pipeline(MyTexture* self, const gchar* pipeline_description, GError** error) {
  g_return_val_if_fail(MY_IS_TEXTURE(self), FALSE);
  
  // Alte Pipeline aufräumen
  if (self->pipeline) {
    gst_element_set_state(self->pipeline, GST_STATE_NULL);
    gst_object_unref(self->pipeline);
  }
  
  // Neue Pipeline erstellen
  self->pipeline = gst_parse_launch(pipeline_description, error);
  if (!self->pipeline) {
    return FALSE;
  }
  
  // AppSink finden
  self->appsink = gst_bin_get_by_name(GST_BIN(self->pipeline), "sink");
  if (!self->appsink) {
    g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                "Pipeline muss ein appsink Element mit name='sink' enthalten");
    gst_object_unref(self->pipeline);
    self->pipeline = nullptr;
    return FALSE;
  }
  
  // AppSink konfigurieren
  g_object_set(self->appsink,
               "emit-signals", TRUE,
               "sync", FALSE,
               nullptr);
  
  // Callback registrieren
  GstAppSinkCallbacks callbacks = {
    nullptr,  // eos
    nullptr,  // new_preroll
    on_new_sample,
    nullptr
  };
  gst_app_sink_set_callbacks(GST_APP_SINK(self->appsink), &callbacks, self, nullptr);
  
  return TRUE;
}

void my_texture_play(MyTexture* self) {
  g_return_if_fail(MY_IS_TEXTURE(self));
  
  if (self->pipeline) {
    gst_element_set_state(self->pipeline, GST_STATE_PLAYING);
  }
}

void my_texture_pause(MyTexture* self) {
  g_return_if_fail(MY_IS_TEXTURE(self));
  
  if (self->pipeline) {
    gst_element_set_state(self->pipeline, GST_STATE_PAUSED);
  }
}

void my_texture_stop(MyTexture* self) {
  g_return_if_fail(MY_IS_TEXTURE(self));
  
  if (self->pipeline) {
    gst_element_set_state(self->pipeline, GST_STATE_NULL);
  }
}

// Hilfsfunktion um den TextureRegistrar zu setzen
void my_texture_set_texture_registrar(MyTexture* self, FlTextureRegistrar* registrar) {
  g_return_if_fail(MY_IS_TEXTURE(self));
  self->texture_registrar = registrar;
}