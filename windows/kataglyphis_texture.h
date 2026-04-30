#ifndef KATAGLYPHIS_TEXTURE_H_
#define KATAGLYPHIS_TEXTURE_H_

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>
#include <flutter/texture_registrar.h>
#include <flutter_texture_registrar.h>

#include <gst/gst.h>

#include <cstdint>
#include <memory>

namespace kataglyphis_native_inference {

class KataglyphisTexture {
 public:
  KataglyphisTexture(uint32_t width, uint32_t height, uint8_t r, uint8_t g,
                     uint8_t b);
  ~KataglyphisTexture();

  bool SetPipeline(const gchar* pipeline_description, GError** error);
  void Play();
  void Pause();
  void Stop();
  void SetColor(uint8_t r, uint8_t g, uint8_t b);

  int64_t texture_id() const { return texture_id_; }
  void set_texture_id(int64_t id) { texture_id_ = id; }

  void SetTextureRegistrar(flutter::TextureRegistrar* registrar);
  flutter::TextureVariant GetTextureVariant();

 private:
  static constexpr size_t kBytesPerPixel = 4;

  int64_t texture_id_;
  uint32_t width_;
  uint32_t height_;
  std::unique_ptr<uint8_t[]> buffer_;

  flutter::TextureRegistrar* texture_registrar_;

  const FlutterDesktopPixelBuffer* CopyPixelBufferCallback(size_t width,
                                                           size_t height);
};

}  // namespace kataglyphis_native_inference

#endif  // KATAGLYPHIS_TEXTURE_H_
