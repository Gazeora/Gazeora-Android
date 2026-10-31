# Third-party software

Gazeora for Android is built with the libraries and the hand model below. Each is under its own
license, all compatible with the GNU GPL version 3. The native client in `app/src/main/cpp` is
Gazeora's own code and bundles no third-party sources, but for the hand model's data.

| Component | Used for | License | In the app as |
|---|---|---|---|
| [AndroidX AppCompat](https://developer.android.com/jetpack/androidx/releases/appcompat) | Activities and themes on every Android version | Apache 2.0 | Compiled into the APK |
| [Material Components for Android](https://github.com/material-components/material-components-android) | Material Design components and theme | Apache 2.0 | Compiled into the APK |
| [AndroidX Core SplashScreen](https://developer.android.com/jetpack/androidx/releases/core) | The launch screen | Apache 2.0 | Compiled into the APK |
| [AndroidX Lifecycle](https://developer.android.com/jetpack/androidx/releases/lifecycle) | ViewModel and LiveData | Apache 2.0 | Compiled into the APK |
| [Dagger Hilt](https://dagger.dev/hilt/) | Dependency injection | Apache 2.0 | Compiled into the APK |
| [LiteRT](https://ai.google.dev/edge/litert), with its GPU delegate | Running the hand tracker's models | Apache 2.0 | Its native libraries, which the native client links to |
| [MediaPipe hand landmarker models](https://storage.googleapis.com/mediapipe-models/hand_landmarker/hand_landmarker/float16/1/hand_landmarker.task) | Finding the viewer's hands in the camera's pictures | Apache 2.0 | Assets, fetched by the build and taken out of their bundle |
| [VR Hand](https://sketchfab.com/3d-models/vr-hand-a647cce524254c9a96d4244c12811d7a) by [RockDutyFree_models](https://sketchfab.com/RockDutyFree_models) | The viewer's hands in a VR viewer | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) | Its skin and rig, converted (`app/src/main/cpp/hands/hand_model_data.h`) |

The hands are based on "VR Hand" (https://sketchfab.com/3d-models/vr-hand-a647cce524254c9a96d4244c12811d7a)
by RockDutyFree_models (https://sketchfab.com/RockDutyFree_models), licensed under CC BY 4.0
(http://creativecommons.org/licenses/by/4.0/). It was changed: converted from glTF, its right
hand mirrored for the left, drawn in one color of the app's own rather than its texture, and
its wrist faded out. The hand shapes the app recognizes follow [Handy.js](https://github.com/stewdio/vr-hands)
by Stewart Smith (MIT); no code of it is included.

The versions are in [`gradle/libs.versions.toml`](gradle/libs.versions.toml). These libraries
also bring in other AndroidX, Kotlin and Google libraries (LiteRT's Java API, Guava's
ListenableFuture, JSR 305, Error Prone annotations, JSpecify, and javax and Jakarta Inject), all
under Apache 2.0. The Apache License, Version 2.0 is at
<https://www.apache.org/licenses/LICENSE-2.0>.

[JUnit](https://junit.org/junit4/) (EPL 1.0), [AndroidX Test and Espresso](https://developer.android.com/training/testing)
(Apache 2.0) and [GoogleTest](https://github.com/google/googletest) (BSD 3-Clause) build and
run the tests only; they are not part of the app.
