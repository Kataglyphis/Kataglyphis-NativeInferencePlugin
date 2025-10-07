#include "kataglyphis_native_inference_plugin.h"

// This must be included before many other Windows headers.
#include <windows.h>

// For getPlatformVersion; remove unless needed for your plugin implementation.
#include <VersionHelpers.h>

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <memory>
#include <sstream>

namespace kataglyphis_native_inference {

// static
void KataglyphisNativeInferencePlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarWindows *registrar) {
  auto channel =
      std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
          registrar->messenger(), "kataglyphis_native_inference",
          &flutter::StandardMethodCodec::GetInstance());

  auto plugin = std::make_unique<KataglyphisNativeInferencePlugin>();

  channel->SetMethodCallHandler(
      [plugin_pointer = plugin.get()](const auto &call, auto result) {
        plugin_pointer->HandleMethodCall(call, std::move(result));
      });

  registrar->AddPlugin(std::move(plugin));
}

KataglyphisNativeInferencePlugin::KataglyphisNativeInferencePlugin() {}

KataglyphisNativeInferencePlugin::~KataglyphisNativeInferencePlugin() {}

void KataglyphisNativeInferencePlugin::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue> &method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  if (method_call.method_name().compare("getPlatformVersion") == 0) {
    std::ostringstream version_stream;
    version_stream << "Windows ";
    if (IsWindows10OrGreater()) {
      version_stream << "10+";
    } else if (IsWindows8OrGreater()) {
      version_stream << "8";
    } else if (IsWindows7OrGreater()) {
      version_stream << "7";
    }
    result->Success(flutter::EncodableValue(version_stream.str()));
  } else if(method_call.method_name().compare("add") == 0) {
      const auto* args = std::get_if<flutter::EncodableMap>(method_call.arguments());
      if (args) {
        int a = 0, b = 0;
        auto itA = args->find(flutter::EncodableValue("a"));
        auto itB = args->find(flutter::EncodableValue("b"));
        if (itA != args->end()) a = std::get<int>(itA->second);
        if (itB != args->end()) b = std::get<int>(itB->second);

        int sum = a + b;
        result->Success(flutter::EncodableValue(sum));
      } else {
        result->Error("bad_args", "Expected map with a and b");
      }
  } else {
    result->NotImplemented();
  }
}

}  // namespace kataglyphis_native_inference
