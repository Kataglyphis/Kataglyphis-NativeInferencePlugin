#include "kataglyphis_texture.h"

#include <windows.h>

#include <algorithm>

namespace kataglyphis_native_inference {

namespace {

constexpr size_t kBytesPerPixel = 4;

}  // namespace

KataglyphisTexture::KataglyphisTexture(uint32_t width, uint32_t height, uint8_t r,
                                       uint8_t g, uint8_t b)
    : texture_id_(-1),
      width_(width),
      height_(height),
      buffer_(new uint8_t[width * height * kBytesPerPixel]),
      texture_registrar_(nullptr) {
  OutputDebugStringA("[kataglyphis_texture] Constructor called\n");
  const uint32_t pixels = width * height;
  for (uint32_t i = 0; i < pixels; ++i) {
    uint8_t* p = buffer_.get() + i * 4;
    p[0] = r;
    p[1] = g;
    p[2] = b;
    p[3] = 255;
  }
}

KataglyphisTexture::~KataglyphisTexture() {
  OutputDebugStringA("[kataglyphis_texture] Destructor called\n");
  buffer_.reset();
}

void KataglyphisTexture::SetTextureRegistrar(
    flutter::TextureRegistrar* registrar) {
  texture_registrar_ = registrar;
}

const FlutterDesktopPixelBuffer* KataglyphisTexture::CopyPixelBufferCallback(
    size_t /*width*/, size_t /*height*/) {
  static FlutterDesktopPixelBuffer pixel_buffer;
  pixel_buffer.buffer = buffer_.get();
  pixel_buffer.width = width_;
  pixel_buffer.height = height_;
  pixel_buffer.release_callback = nullptr;
  pixel_buffer.release_context = nullptr;
  return &pixel_buffer;
}

flutter::TextureVariant KataglyphisTexture::GetTextureVariant() {
  return flutter::TextureVariant(flutter::PixelBufferTexture(
      [this](size_t width, size_t height) -> const FlutterDesktopPixelBuffer* {
        return this->CopyPixelBufferCallback(width, height);
      }));
}

bool KataglyphisTexture::SetPipeline(const gchar* pipeline_description,
                                     GError** error) {
  OutputDebugStringA("[kataglyphis_texture] SetPipeline called (noop)\n");
  return true;
}

void KataglyphisTexture::Play() {
  OutputDebugStringA("[kataglyphis_texture] Play called (noop)\n");
}

void KataglyphisTexture::Pause() {
  OutputDebugStringA("[kataglyphis_texture] Pause called (noop)\n");
}

void KataglyphisTexture::Stop() {
  OutputDebugStringA("[kataglyphis_texture] Stop called (noop)\n");
}

void KataglyphisTexture::SetColor(uint8_t r, uint8_t g, uint8_t b) {
  OutputDebugStringA("[kataglyphis_texture] SetColor called\n");
  const uint32_t pixels = width_ * height_;
  for (uint32_t i = 0; i < pixels; ++i) {
    uint8_t* p = buffer_.get() + i * 4;
    p[0] = r;
    p[1] = g;
    p[2] = b;
    p[3] = 255;
  }
  if (texture_registrar_) {
    texture_registrar_->MarkTextureFrameAvailable(texture_id_);
  }
}

}  // namespace kataglyphis_native_inference
