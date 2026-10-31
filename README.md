# Gazeora for Android

Gazeora's Android viewer. It plays a PC's screen stream full screen on a phone, over Wi-Fi or a
USB cable. The phone can show the picture flat or for a VR viewer. While the picture is flat,
touches on it drive the PC's mouse; in a VR viewer, over Gazeora's own window, your hand does,
and in a game your head turns the camera.

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
- **Your hand as a pointer.** In a VR viewer, while Gazeora's own window shows (no game runs),
  your hand points at the window, as VR headsets let a hand point at a screen out of reach. The
  middle of your palm moves a ring cursor, and the middle of the camera's view, well inside its
  edges, covers the whole window, so its corners are in easy reach. A pinch clicks where the
  cursor was as it began, and a pinch that moves scrolls, as a finger does on the touch screen;
  no button is ever left held down. The ring closes in as a pinch nears and fills once it
  pinches. The cursor is steadied against tremor and the tracker's wobble, and a hand held
  still holds it perfectly still. The hand already pointing keeps pointing; otherwise your
  right hand points, or your left without it. It needs a Gazeora on the PC that takes pointer
  input in VR.
- **Head tracking.** In a VR viewer, the phone's rotation sensor (the game rotation vector:
  gyroscope and accelerometer, no compass) is read 200 times a second, and where your head points
  goes to the PC, on the USB cable or over UDP port 8557 like touches. Gazeora turns the camera of
  the game in front with it, captured or not. The sensor is off while the picture is flat. It needs a Gazeora on
  the PC that takes head tracking.
- **Hand tracking on the phone.** The back camera looks for your hands with MediaPipe's hand
  landmarker models, run with LiteRT straight on the camera's pictures, each on the phone's
  GPU or CPU, whichever runs it faster there. As a headset does, it searches the whole picture
  only to find hands, then follows each in a small square around it. A dim picture is
  brightened first, and the camera may slow to 15 frames a second to let in more light, so it
  still finds hands in a dim room. The models load as the app starts, behind the splash: on
  the CPU first, so hands are tracked at once, then on the GPU, which the first time after an
  install takes about 20 seconds in the background; tracking changes over when it's ready.
  Whether a hand is your right or left is settled when it's found, by where its arm comes in
  from, and kept: from behind, a right hand looks much like a left one from the front, which
  the tracker mixes up. Other people's hands in view are left out (a hand becomes yours only
  when it's within arm's reach and its arm comes from out of view, below or to the side), and
  one hand is never taken for two. The camera's picture never shows. Games show without any
  of this.
- **Steady in the viewer.** While the picture is for a VR viewer, the screen stays in the
  landscape it was in, so tilting the head doesn't turn it over.
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

The player asks for the **camera** as it opens, before the phone goes into the viewer, so your
hand can point at Gazeora's window. It's optional.

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

The native protocol code and the hands' geometry have host-side GoogleTest tests. They
build with any desktop C++17 compiler:

```bash
cmake -S app/src/test/cpp -B build/host-tests
cmake --build build/host-tests
build/host-tests/protocols_test
build/host-tests/hands_test
```

The build fetches MediaPipe's hand landmarker models from Google's storage and checks them
against their known SHA-256 (`fetchHandModel` in `app/build.gradle.kts`), so it needs the
network once. The native hand tracker uses LiteRT's C API, whose headers and libraries the build
takes out of LiteRT's AARs (`extractLiteRt`).

Debuggable builds also show the hand pointer over a flat picture of Gazeora's window, so it can
be tried without a viewer. Release builds show it only in a VR viewer.

## Versions

The version lives in the `VERSION` file at the root, as `MAJOR.MINOR.PATCH` (`1.1.0`), or
`MAJOR.MINOR.PATCH-rc.N` for a release candidate (`1.1.0-rc.1`). It's always shown with a `v`:
`app/build.gradle.kts` makes it the `versionName` (`v1.1.0-rc.1`) and works out the
`versionCode` from it, which only grows: a release candidate counts just below its release
(`1.1.0-rc.1` is 1010001, `1.1.0` is 1010099). Minor and patch go up to 99, rc up to 98.

CI (`.github/workflows/ci.yml`) runs the Java unit tests and the native protocol and hand
geometry tests and builds the debug APK on every pull request.

Every Monday at 00:00 UTC (or by hand: **Actions → Weekly pre-release → Run workflow**),
`.github/workflows/weekly-release.yml` compares `master` with the highest release so far. If
anything other than `VERSION` has changed, it releases the next patch as a candidate (after
`v1.0.1-rc.1` comes `v1.0.2-rc.1`, after a stable `v1.0.3`, `v1.0.4-rc.1`); if nothing has, there's
no release that week. `master` only takes pull requests, so the workflow tags its own commit on
top of `master` that sets `VERSION`, and CI builds the signed APK from that tag and publishes the
pre-release with notes from the pull requests merged since the last release. `VERSION` on
`master` keeps the last version set by hand. Patch numbers stop at 99, so before then, release
the next minor version by hand.

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
| `pointer/` | Sends touches and the hand pointer's moves, clicks and scrolls back to the PC: over UDP (port `8557`) for network streams, or over the USB link |
| `player.*`, `video_*` | Ties a source to the decoder and renderer |
| `hands/` | The viewer's hands over Gazeora's window: the back camera (for the hand tracker and the room's light only); the models (`hand_models.*`, LiteRT, loaded as the app starts) and the hand tracker (`hand_tracker.*`); telling the viewer's hands from others' and following them; the hand pointer (`hand_pointer.*`); the skeleton they're fitted to, the hand model rigged to it (`hand_model_data.h`, generated by `tools/hand_model/convert.py`), drawn only while no hand points, and the shapes the hands make; and the geometry (no Android in `view_geometry.*`, `hand_detection.*`, `hand_tracking.*`, `hand_geometry.*`, `hand_follower.*`, `hand_pointer.*`, `hand_skeleton.*`, `rigged_hand.*`, `hand_shapes.*`, `light_estimate.*`, `camera_frame.*`) |
| `gl/` | Decoder and camera buffers as textures, and handing them back once drawn |
| `shaders/` | The GLSL: `video.frag` for the stream as it is or for a VR viewer, with the hand pointer's cursor; `hand.vert` and `hand.frag` for the rigged hands over it, with their outline |
| `discovery.*` | Network search for PCs |
| `jni.cpp` | The JNI entry points for the Java adapters |

## Project structure

```
app/src/main/java/com/gazeora/gazeora/
├── core/
│   ├── device/        # emulator detection
│   ├── di/            # app-wide Hilt modules
│   ├── discovery/     # finding PCs on the network (data + domain)
│   ├── hands/         # loads the hand tracker's models as the app starts
│   ├── nativebridge/  # loads the native library
│   ├── network/       # local network permission
│   ├── stream/        # StreamProtocol, StreamTarget
│   ├── ui/            # shared views and UI helpers
│   └── usb/           # USB accessory connection
├── feature/
│   ├── connect/       # home screen: PC list and manual entry
│   │   ├── data/  domain/  presentation/  di/
│   └── player/        # full-screen player, touch and hand pointer
│       ├── data/  domain/  presentation/  di/
└── GazeoraApplication.java
```

Unit tests in `app/src/test/java` use the same package layout.

### The hand model

The rigged hands, drawn while no hand points, are the right hand of
[VR Hand](https://sketchfab.com/3d-models/vr-hand-a647cce524254c9a96d4244c12811d7a) by
RockDutyFree_models (CC BY 4.0), mirrored for the left. `hands/hand_model_data.h` is
generated from its glTF: its skin in its rest pose and its bones mapped onto the hand
skeleton's; not its texture, as the hands are drawn in one color of their own. The skeleton's
measures (`hand_skeleton.h`) are the model's, so the skin fits its bones; the script prints
them. To regenerate it (needs numpy):

```sh
python tools/hand_model/convert.py path/to/scene.gltf
```

## Contributing

Bug reports, phone reports and pull requests are welcome: see [CONTRIBUTING.md](CONTRIBUTING.md).

## License

Copyright (C) 2026 Nithin Aashik. Gazeora for Android is free software under the
[GNU GPL version 3](LICENSE): you can use it, study it, change it and share it. Versions you
share must stay under the same license, with their source.

[NOTICE](NOTICE) adds three terms, as the GPL's section 7 allows: modified versions keep the
attribution "Gazeora by Nithin Aashik" where they show information about themselves, are marked
as modified, and don't use the Gazeora name or logo as their own
([TRADEMARKS.md](TRADEMARKS.md)). The libraries and the hand model the app is built with are
listed in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

Gazeora for Android is also available under a commercial license, for products that can't
follow the GPL, such as closed-source apps or hardware that builds it in. Royalties and terms
are agreed with Nithin Aashik: see [COMMERCIAL-LICENSE.md](COMMERCIAL-LICENSE.md).
