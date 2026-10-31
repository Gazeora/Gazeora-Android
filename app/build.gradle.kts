// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

import java.net.URI
import java.security.MessageDigest
import java.util.zip.ZipFile
import javax.inject.Inject

plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.hilt)
}

/**
 * The app's version, from the VERSION file at the root: MAJOR.MINOR.PATCH, or
 * MAJOR.MINOR.PATCH-rc.N for a release candidate. Shown with a "v": v1.2.0, v1.2.0-rc.1.
 */
class AppVersion private constructor(
    private val major: Int,
    private val minor: Int,
    private val patch: Int,
    private val candidate: Int?,
) {
    val name: String = "v$major.$minor.$patch" + (candidate?.let { "-rc.$it" } ?: "")

    // Android only installs a higher code over a lower one, so a candidate counts below its release
    val code: Int = ((major * PART + minor) * PART + patch) * PART + (candidate ?: STABLE)

    companion object {
        private const val PART = 100
        private const val STABLE = PART - 1
        private const val MAX_CODE = 2_100_000_000  // Google Play's limit
        private val FORMAT = Regex("""(\d+)\.(\d+)\.(\d+)(?:-rc\.([1-9]\d*))?""")

        fun parse(text: String): AppVersion {
            val match = FORMAT.matchEntire(text.trim())
                ?: throw GradleException("VERSION must be like 1.2.0, or 1.2.0-rc.1 for a release candidate; it says '${text.trim()}'")
            val (major, minor, patch, candidate) = match.destructured
            val version = AppVersion(major.toInt(), minor.toInt(), patch.toInt(), candidate.toIntOrNull())
            if (minor.toInt() >= PART || patch.toInt() >= PART || (candidate.toIntOrNull() ?: 0) >= STABLE || version.code > MAX_CODE) {
                throw GradleException("VERSION ${text.trim()} is out of range: minor and patch up to 99, rc up to 98")
            }
            return version
        }
    }
}

val appVersion = AppVersion.parse(
    providers.fileContents(rootProject.layout.projectDirectory.file("VERSION")).asText.get()
)

/**
 * MediaPipe's hand landmarker models, for the hands over Gazeora's window (the native hand
 * tracker, src/main/cpp/hands/hand_tracker.h): fetched at build time rather than kept in git,
 * checked against their known SHA-256, and taken out of their bundle as the app's assets
 * hands/hand_detector.tflite and hands/hand_landmarks_detector.tflite.
 */
abstract class FetchHandModel : DefaultTask() {
    @get:Input
    abstract val url: Property<String>

    @get:Input
    abstract val sha256: Property<String>

    @get:OutputDirectory
    abstract val assetsDir: DirectoryProperty

    @TaskAction
    fun fetch() {
        assetsDir.get().asFile.deleteRecursively()  // nothing left from an older fetch
        val bundle = URI(url.get()).toURL().openStream().use { it.readBytes() }
        val digest = MessageDigest.getInstance("SHA-256").digest(bundle)
            .joinToString("") { "%02x".format(it) }
        if (digest != sha256.get()) {
            throw GradleException("${url.get()} has SHA-256 $digest, not ${sha256.get()}")
        }
        val hands = assetsDir.dir("hands").get().asFile
        hands.mkdirs()
        val saved = temporaryDir.resolve("hand_landmarker.task")
        saved.writeBytes(bundle)
        ZipFile(saved).use { zip ->
            for (name in listOf("hand_detector.tflite", "hand_landmarks_detector.tflite")) {
                val entry = zip.getEntry(name) ?: throw GradleException("${url.get()} lacks $name")
                zip.getInputStream(entry).use { input ->
                    hands.resolve(name).outputStream().use { input.copyTo(it) }
                }
            }
        }
        saved.delete()
    }
}

val fetchHandModel = tasks.register<FetchHandModel>("fetchHandModel") {
    url = "https://storage.googleapis.com/mediapipe-models/hand_landmarker/hand_landmarker/float16/1/hand_landmarker.task"
    sha256 = "fbc2a30080c3c557093b5ddfc334698132eb341044ccee322ccf8bcf3607cde1"
    assetsDir = layout.buildDirectory.dir("generated/handModel")
}

/**
 * LiteRT's C headers and native libraries, out of its AARs, for the native client's CMake build
 * (src/main/cpp/CMakeLists.txt): the hand tracker runs its models through LiteRT's C API. The
 * app packages the same libraries through its dependencies.
 */
abstract class ExtractLiteRt : DefaultTask() {
    @get:InputFiles
    abstract val aars: ConfigurableFileCollection

    @get:OutputDirectory
    abstract val outputDir: DirectoryProperty

    @get:Inject
    abstract val archives: ArchiveOperations

    @get:Inject
    abstract val fileSystem: FileSystemOperations

    @TaskAction
    fun extract() {
        fileSystem.sync {
            aars.forEach { from(archives.zipTree(it)) }
            include("headers/**", "jni/**")
            into(outputDir)
        }
    }
}

val liteRtNative: Configuration by configurations.creating {
    isCanBeConsumed = false
    isTransitive = false
}

val extractLiteRt = tasks.register<ExtractLiteRt>("extractLiteRt") {
    aars.from(liteRtNative)
    outputDir = layout.buildDirectory.dir("litert")
}

// The release signing key, given by CI (see .github/workflows/ci.yml). Without it the release
// build is unsigned, which is fine for trying it locally.
val releaseKeystore = providers.environmentVariable("GAZEORA_KEYSTORE_FILE")

android {
    namespace = "com.gazeora.gazeora"
    compileSdk {
        version = release(37)
    }
    ndkVersion = "30.0.16248370"

    defaultConfig {
        applicationId = "com.gazeora.gazeora"
        minSdk = 29
        targetSdk = 37
        versionCode = appVersion.code
        versionName = appVersion.name

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
        ndk {
            // Phones, and the emulator on a PC.
            abiFilters += listOf("arm64-v8a", "armeabi-v7a", "x86_64")
        }
    }

    // The client: networking, protocols and decoding (src/main/cpp).
    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "4.1.2"
        }
    }
    defaultConfig {
        externalNativeBuild {
            cmake {
                arguments += "-DLITERT_DIR=" +
                    layout.buildDirectory.dir("litert").get().asFile.invariantSeparatorsPath
            }
        }
    }

    signingConfigs {
        if (releaseKeystore.isPresent) {
            create("release") {
                storeFile = file(releaseKeystore.get())
                storePassword = providers.environmentVariable("GAZEORA_KEYSTORE_PASSWORD").get()
                keyAlias = providers.environmentVariable("GAZEORA_KEY_ALIAS").get()
                keyPassword = providers.environmentVariable("GAZEORA_KEY_PASSWORD").get()
            }
        }
    }

    buildTypes {
        release {
            if (releaseKeystore.isPresent) signingConfig = signingConfigs.getByName("release")
            optimization {
                enable = true
                packageScope = setOf("androidx.**", "kotlin.**", "kotlinx.**")
            }
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
    androidResources {
        noCompress += "tflite"  // LiteRT reads the models straight from the APK
    }
}

androidComponents {
    onVariants { variant ->
        variant.sources.assets?.addGeneratedSourceDirectory(fetchHandModel, FetchHandModel::assetsDir)
    }
}

dependencies {
    implementation(libs.appcompat)
    implementation(libs.material)
    implementation(libs.core.splashscreen)
    implementation(libs.hilt.android)
    annotationProcessor(libs.hilt.compiler)
    implementation(libs.lifecycle.viewmodel)
    implementation(libs.lifecycle.livedata)
    implementation(libs.litert)
    implementation(libs.litert.gpu)
    liteRtNative(libs.litert)
    liteRtNative(libs.litert.gpu)
    testImplementation(libs.junit)
    androidTestImplementation(libs.espresso.core)
    androidTestImplementation(libs.ext.junit)
}

// CMake needs LiteRT's headers and libraries before it configures.
tasks.configureEach {
    if (name.startsWith("configureCMake") || name.startsWith("buildCMake")) {
        dependsOn(extractLiteRt)
    }
}
