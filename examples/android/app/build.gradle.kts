plugins {
    id("com.android.application")
}

android {
    namespace = "io.github.libuvc.example"
    compileSdk = 36
    // Pin the NDK so that every machine, and CI, builds the native code with
    // the same compiler.
    ndkVersion = "30.0.16248370"

    defaultConfig {
        applicationId = "io.github.libuvc.example"
        // Android 11, for AImageDecoder, which decodes MJPEG.
        minSdk = 30
        targetSdk = 36
        versionCode = 1
        versionName = "1.0"
    }

    buildTypes {
        release {
            // Debug builds compile the native code without optimisation. Sign
            // release builds with the debug key too, so that they install as
            // they are: an app of its own would use its own key.
            signingConfig = signingConfigs.getByName("debug")
        }
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
        }
    }
}
