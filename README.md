# Gazeora for Android

Gazeora's Android viewer. It plays a PC's screen stream full screen on a phone, over Wi-Fi or a
USB cable. The phone can show the picture flat or for a VR viewer. While the picture is flat,
touches on it drive the PC's mouse.

The PC side is the separate **Gazeora** project. This app is only the viewer.

## Features

- **Finds PCs on its own.** Discovery asks the local network once a second and lists every PC
  running Gazeora. On the emulator it also asks the host PC (`10.0.2.2`).
- **Several stream protocols:** RTSP over TCP, RTSP over UDP, raw UDP, HTTP, and USB.
- **USB accessory mode.** When the PC switches the phone to accessory mode, the app opens
  straight into the stream. It reconnects when the cable is plugged back in.
- **Follows the PC.** If the PC changes protocol mid-stream (for example from Wi-Fi to USB),
  the player switches with it.
- **Touch as a mouse.** On a flat picture, works like a touch screen: a tap clicks there, and
  moving a finger scrolls. No button is ever left held down on the PC.
- **Hardware decoding.** H.264 and H.265 are decoded with `MediaCodec`, using low-latency decoders
  where the device has them.
- **Manual connection.** You can type a `host[:port]` instead. The app remembers the last one.

## Requirements

| What | Version |
| --- | --- |
| Android Studio | Recent, with its bundled JDK |
| Android SDK | compileSdk / targetSdk 37, minSdk 29 (Android 10) |
| NDK | 30.0.16248370 |
| CMake | 4.1.2 (from the SDK manager) |
| ABIs | `arm64-v8a`, `armeabi-v7a`, `x86_64` |

On Android 17 and later the app asks for the **local network** permission. Without it, Android
silently drops traffic to the PC. Loopback addresses (`127.*`, for `adb reverse`) don't need it.

## Build and run

```bash
./gradlew assembleDebug          # build the debug APK
./gradlew installDebug           # install on a connected device or emulator
```

On Windows, use `gradlew.bat`. If `java` isn't on your `PATH`, point `JAVA_HOME` at Android
Studio's bundled JDK (for example `C:\Program Files\Android\Android Studio\jbr`).

### Connecting to a PC

1. Start Gazeora on the PC.
2. Open the app. PCs on the same network show up in the list. Tap one to watch.
3. Or open **Connect manually**, pick a protocol and type the PC's address. The default port is
   `8554`.
4. For USB, plug in the cable and let the PC switch the phone to accessory mode. The app opens
   the stream by itself.

## Tests

```bash
./gradlew testDebugUnitTest      # Java unit tests (app/src/test/java)
```

The native protocol code has host-side GoogleTest tests. They build with any desktop C++17
compiler:

```bash
cmake -S app/src/test/cpp -B build/host-tests
cmake --build build/host-tests
build/host-tests/protocols_test
```

## Versions

The version lives in the `VERSION` file at the root, as `MAJOR.MINOR.PATCH` (`1.1.0`), or
`MAJOR.MINOR.PATCH-rc.N` for a release candidate (`1.1.0-rc.1`). It's always shown with a `v`:
`app/build.gradle.kts` makes it the `versionName` (`v1.1.0-rc.1`) and works out the
`versionCode` from it, which only grows: a release candidate counts just below its release
(`1.1.0-rc.1` is 1010001, `1.1.0` is 1010099). Minor and patch go up to 99, rc up to 98.

CI (`.github/workflows/ci.yml`) runs the Java unit tests and the native protocol tests and
builds the debug APK on every pull request.

## Architecture

The Java code follows **MVVM + Clean Architecture**. **Hilt** injects all dependencies.

```
Presentation  ──▶  Domain  ◀──  Data
(Activity, ViewModel,   (use cases, models,    (repositories, native
 UiState, views)         repository interfaces) adapters, mappers)
```

- **Presentation.** Activities only bind views and forward events. ViewModels expose
  `LiveData<UiState>`, and UI states draw themselves (State pattern). One-off events go
  through an `EventQueue`.
- **Domain.** Plain Java with no Android types. Use cases have a single `execute()` method.
- **Data.** Implements the domain interfaces. Native code sits behind adapters
  (`NativePlayer`, `NativeDiscovery`, `NativeUsbLink`), so the rest of the app never calls JNI.

### Native client (`app/src/main/cpp`)

The C++ client does the networking, the stream protocols and the decoding. Frames go
straight onto the player's `Surface`.

| Folder | What it does |
| --- | --- |
| `protocols/` | Pure protocol code with no Android dependencies: RTSP, RTP, MPEG-TS, GZF framing, discovery replies, pointer messages |
| `sources/` | Stream sources for each protocol, plus the shared USB link |
| `pointer/` | Sends touches back to the PC: over UDP (port `8557`) for network streams, or over the USB link |
| `player.*`, `video_*` | Ties a source to the decoder and renderer |
| `discovery.*` | Network search for PCs |
| `jni.cpp` | The JNI entry points for the Java adapters |

## Project structure

```
app/src/main/java/com/gazeora/gazeora/
├── core/
│   ├── device/        # emulator detection
│   ├── di/            # app-wide Hilt modules
│   ├── discovery/     # finding PCs on the network (data + domain)
│   ├── nativebridge/  # loads the native library
│   ├── network/       # local network permission
│   ├── stream/        # StreamProtocol, StreamTarget
│   ├── ui/            # shared views and UI helpers
│   └── usb/           # USB accessory connection
├── feature/
│   ├── connect/       # home screen: PC list and manual entry
│   │   ├── data/  domain/  presentation/  di/
│   └── player/        # full-screen player and touch pointer
│       ├── data/  domain/  presentation/  di/
└── GazeoraApplication.java
```

Unit tests in `app/src/test/java` use the same package layout.
