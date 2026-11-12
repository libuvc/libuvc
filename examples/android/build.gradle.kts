buildscript {
    repositories {
        mavenCentral()
    }
    dependencies {
        // AGP 9 compiles Kotlin itself, with no org.jetbrains.kotlin.android
        // plugin, using the Kotlin Gradle plugin version it depends on
        // (2.2.10) unless a newer one is on the classpath.
        classpath("org.jetbrains.kotlin:kotlin-gradle-plugin:2.4.21")
    }
}

plugins {
    id("com.android.application") version "9.5.0" apply false
}
