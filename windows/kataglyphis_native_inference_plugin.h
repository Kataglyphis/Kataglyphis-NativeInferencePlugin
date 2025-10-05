#ifndef FLUTTER_PLUGIN_KATAGLYPHIS_NATIVE_INFERENCE_PLUGIN_H_
#define FLUTTER_PLUGIN_KATAGLYPHIS_NATIVE_INFERENCE_PLUGIN_H_

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>

#include <memory>

namespace kataglyphis_native_inference {

class KataglyphisNativeInferencePlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

  KataglyphisNativeInferencePlugin();

  virtual ~KataglyphisNativeInferencePlugin();

  // Disallow copy and assign.
  KataglyphisNativeInferencePlugin(const KataglyphisNativeInferencePlugin&) = delete;
  KataglyphisNativeInferencePlugin& operator=(const KataglyphisNativeInferencePlugin&) = delete;

  // Called when a method is called on this plugin's channel from Dart.
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
};

}  // namespace kataglyphis_native_inference

#endif  // FLUTTER_PLUGIN_KATAGLYPHIS_NATIVE_INFERENCE_PLUGIN_H_
