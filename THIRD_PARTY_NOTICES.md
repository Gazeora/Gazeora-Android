# Third-party software

Gazeora for Android is built with the libraries below. Each is under its own license, all
compatible with the GNU GPL version 3. The native client in `app/src/main/cpp` is Gazeora's own
code and bundles no third-party sources.

| Component | Used for | License | In the app as |
|---|---|---|---|
| [AndroidX AppCompat](https://developer.android.com/jetpack/androidx/releases/appcompat) | Activities and themes on every Android version | Apache 2.0 | Compiled into the APK |
| [Material Components for Android](https://github.com/material-components/material-components-android) | Material Design components and theme | Apache 2.0 | Compiled into the APK |
| [AndroidX Core SplashScreen](https://developer.android.com/jetpack/androidx/releases/core) | The launch screen | Apache 2.0 | Compiled into the APK |
| [AndroidX Lifecycle](https://developer.android.com/jetpack/androidx/releases/lifecycle) | ViewModel and LiveData | Apache 2.0 | Compiled into the APK |
| [Dagger Hilt](https://dagger.dev/hilt/) | Dependency injection | Apache 2.0 | Compiled into the APK |

The versions are in [`gradle/libs.versions.toml`](gradle/libs.versions.toml). These libraries
also bring in other AndroidX and Kotlin libraries, all under Apache 2.0; the Apache License,
Version 2.0 is at <https://www.apache.org/licenses/LICENSE-2.0>.

[JUnit](https://junit.org/junit4/) (EPL 1.0), [AndroidX Test and Espresso](https://developer.android.com/training/testing)
(Apache 2.0) and [GoogleTest](https://github.com/google/googletest) (BSD 3-Clause) build and
run the tests only; they are not part of the app.
