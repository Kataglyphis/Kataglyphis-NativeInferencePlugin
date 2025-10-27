#ifndef FLUTTER_MY_TEXTURE_H_
#define FLUTTER_MY_TEXTURE_H_

#include <flutter_linux/flutter_linux.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>

G_DECLARE_FINAL_TYPE(MyTexture, my_texture, MY, TEXTURE, FlPixelBufferTexture)

MyTexture* my_texture_new(uint32_t width, uint32_t height, uint8_t r, uint8_t g, uint8_t b);

void my_texture_set_color(MyTexture* self, uint8_t r, uint8_t g, uint8_t b);

// Neue GStreamer-Funktionen
void my_texture_set_texture_registrar(MyTexture* self, FlTextureRegistrar* registrar);

gboolean my_texture_set_pipeline(MyTexture* self, const gchar* pipeline_description, GError** error);

void my_texture_play(MyTexture* self);
void my_texture_pause(MyTexture* self);
void my_texture_stop(MyTexture* self);

#endif  // FLUTTER_MY_TEXTURE_H_