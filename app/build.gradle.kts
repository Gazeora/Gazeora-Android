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
}

dependencies {
    implementation(libs.appcompat)
    implementation(libs.material)
    implementation(libs.core.splashscreen)
    implementation(libs.hilt.android)
    annotationProcessor(libs.hilt.compiler)
    implementation(libs.lifecycle.viewmodel)
    implementation(libs.lifecycle.livedata)
    testImplementation(libs.junit)
    androidTestImplementation(libs.espresso.core)
    androidTestImplementation(libs.ext.junit)
}