
import 'kataglyphis_native_inference_platform_interface.dart';

class KataglyphisNativeInference {
  Future<String?> getPlatformVersion() {
    return KataglyphisNativeInferencePlatform.instance.getPlatformVersion();
  }
}
