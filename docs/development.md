# Development

Building, testing, versions and releases. For how the code is organized, see
[Architecture](architecture.md). For the contribution workflow, see
[CONTRIBUTING.md](../CONTRIBUTING.md).

## Prerequisites

| What | Version | Source |
|---|---|---|
| JDK | Android Studio's bundled JDK | The app compiles as Java 11 |
| Android SDK platform | 37 (`compileSdk` and `targetSdk`) | SDK manager |
| Android Gradle Plugin | 9.4.1 | `gradle/libs.versions.toml`, fetched by Gradle |
| NDK | `30.0.16248370` | SDK manager |
| CMake | `4.1.2` | SDK manager |

The minimum Android version is API 29 (Android 10). The APK is built for `arm64-v8a`,
`armeabi-v7a` and `x86_64`.

CI installs exactly these SDK packages:

```text
platforms;android-37.0 build-tools;36.0.0 ndk;30.0.16248370 cmake;4.1.2
```

If `java` is not on your `PATH`, set `JAVA_HOME` to Android Studio's bundled JDK, for example
`C:\Program Files\Android\Android Studio\jbr` on Windows.

The first build needs network access:

- `fetchHandModel` downloads MediaPipe's hand landmarker bundle, checks it against its known
  SHA-256, and extracts `hands/hand_detector.tflite` and `hands/hand_landmarks_detector.tflite`
  as generated assets. The models are not kept in git.
- `extractLiteRt` takes LiteRT's C headers and native libraries out of its AARs for the native
  build.

## Building

From the repository root (use `gradlew.bat` on Windows):

```bash
./gradlew assembleDebug    # debug APK in app/build/outputs/apk/debug/
./gradlew installDebug     # install on a connected device or emulator
```

To build without the camera status for the PC's stream overlay, as CI does:

```bash
./gradlew assembleDebug -Pgazeora.streamOverlay=false
```

### Release signing

`assembleRelease` signs the APK only when these environment variables are set; otherwise the
release APK is unsigned:

| Variable | Contents |
|---|---|
| `GAZEORA_KEYSTORE_FILE` | Path to the keystore |
| `GAZEORA_KEYSTORE_PASSWORD` | Keystore password |
| `GAZEORA_KEY_ALIAS` | Key alias |
| `GAZEORA_KEY_PASSWORD` | Key password |

Release builds have code optimization enabled.

## Tests

### Java unit tests

```bash
./gradlew testDebugUnitTest
```

They live in `app/src/test/java`, in the same package layout as the code.

### Native host tests

The protocol code, the hand geometry and the head orientation have GoogleTest tests that run on
a desktop, with any C++17 compiler and CMake 3.22 or newer. The configure step downloads
GoogleTest 1.15.2.

```bash
cmake -S app/src/test/cpp -B build/host-tests
cmake --build build/host-tests
build/host-tests/protocols_test
build/host-tests/hands_test
build/host-tests/head_test
```

With a single-config generator the executables are in `build/host-tests/`; on Windows they end
in `.exe`. To build without downloading GoogleTest, pass
`-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=<path to a googletest source tree>`.

### Checks

`tools/license_headers.py` checks that every source file has the copyright and SPDX license
header. CI runs it:

```bash
python3 tools/license_headers.py
```

## Versions

The version is in the `VERSION` file at the root, as `MAJOR.MINOR.PATCH` (`1.1.0`) or
`MAJOR.MINOR.PATCH-rc.N` for a release candidate (`1.1.0-rc.1`). `app/build.gradle.kts` turns
it into:

- `versionName`, with a `v` prefix: `v1.1.0`, `v1.1.0-rc.1`. It is shown in **About** on the
  home screen.
- `versionCode`, which always grows: `((major × 100 + minor) × 100 + patch) × 100`, plus the rc
  number for a candidate or 99 for a stable release. So `1.1.0-rc.1` is 1010001 and `1.1.0` is
  1010099. Minor and patch go up to 99, rc up to 98; anything else fails the build.

## CI and releases

`.github/workflows/ci.yml` runs on every pull request. It checks license headers, runs the Java
unit tests and all three native host tests, and builds the debug APK with
`-Pgazeora.streamOverlay=false`. It also runs by hand (`workflow_dispatch`).

To release by hand, set `VERSION`, then publish a GitHub release with the tag `v<version>`, as a
pre-release for a release candidate. CI checks that the tag matches `VERSION` and that release
candidates are pre-releases, builds the signed release APK
(`assembleRelease -Pgazeora.streamOverlay=false`), verifies its signature, and attaches it as
`Gazeora-v<version>.apk`. CI then sends the release to [gazeora.github.io](https://gazeora.github.io).
Editing, unpublishing or deleting the release later updates the site through
`.github/workflows/website.yml`.

### Weekly pre-release

Every Monday at 00:00 UTC, or by hand from **Actions → Weekly pre-release → Run workflow**,
`.github/workflows/weekly-release.yml` compares `master` with the highest release so far. If
anything other than `VERSION` has changed, it releases the next patch as a candidate: after
`v1.0.1-rc.1` comes `v1.0.2-rc.1`, and after a stable `v1.0.3` comes `v1.0.4-rc.1`. If nothing
has changed, there is no release that week.

`master` only accepts pull requests, so the workflow tags its own commit on top of `master` that
sets `VERSION`. CI builds the signed APK from that tag and publishes the pre-release with notes
from the pull requests merged since the last release. `VERSION` on `master` keeps the last
version set by hand. Patch numbers stop at 99, so release the next minor version by hand before
then.
