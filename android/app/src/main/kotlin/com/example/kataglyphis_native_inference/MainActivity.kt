package com.example.kataglyphis_native_inference

import io.flutter.embedding.android.FlutterActivity

class MainActivity : FlutterActivity() {
    companion object {
        init {
            System.loadLibrary("kataglyphis_native")
        }
    }

    external fun nativeInference(input: FloatArray): FloatArray
}