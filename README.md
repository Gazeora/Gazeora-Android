# Gazeora for Android

The Android companion app for [Gazeora](https://github.com/Gazeora/Gazeora), a Windows game
launcher that captures and streams games. The app plays the PC's stream full screen on the
phone, over Wi-Fi or a USB cable, either flat or laid out for a Cardboard-style VR viewer. It
also sends input back to the PC: touches, a hand pointer, and head tracking.

The PC side lives in the separate [Gazeora](https://github.com/Gazeora/Gazeora) repository.
This app only views the stream and sends input; it does not capture or encode anything.

## Current capabilities

- **Finds PCs by itself** on the local network, or connects to a typed `host[:port]`.
- **Plays every Gazeora protocol:** RTSP over TCP, RTSP over UDP, UDP, HTTP (MPEG-TS), and USB
  (Android Open Accessory).
- **Decodes H.264 and H.265** with `MediaCodec`, requesting low-latency decoding where the
  decoder supports it.
- **Follows the PC** when its protocol changes, for example from Wi-Fi to USB.
- **Touch as a mouse** on a flat picture: a tap clicks, and a moving finger scrolls.
- **VR viewer layout** drawn on the phone with OpenGL ES, using the lens settings the PC sends
  with every frame.
- **Hand pointer** in a VR viewer over Gazeora's window: the back camera tracks your hand with
  MediaPipe's hand landmarker models on LiteRT, and a pinch clicks or scrolls.
- **Head tracking** in a VR viewer: the phone's rotation sensor turns the game's camera on the
  PC.

See the [user guide](docs/user-guide.md) for how each works, and
[Known limitations](#known-limitations).

## Requirements

To run the app:

- Android 10 (API 29) or newer.
- An ABI the APK is built for: `arm64-v8a`, `armeabi-v7a` or `x86_64`.
- A PC running Gazeora, on the same network or connected by USB.
- Optional: a back camera for the hand pointer, and a rotation sensor for head tracking.

To build it, see [Development](docs/development.md). In brief: Android Studio's JDK, Android
SDK platform 37, NDK `30.0.16248370` and CMake `4.1.2`, plus network access on the first build.

## Quick start

From the repository root, with a phone or emulator connected:

```bash
./gradlew installDebug
```

On Windows, use `gradlew.bat`. Then:

1. Start Gazeora on the PC.
2. Open the app. PCs on the same network appear under **Your PCs**. Tap one to watch.
3. Or use **Connect manually**: pick a **Protocol**, type the **PC address**, and tap
   **Watch**. The default port is `8554`.
4. For USB, plug the phone into the PC. When the PC switches the phone to accessory mode, the
   app opens the stream by itself. The PC needs a one-time USB driver setup; see Gazeora's
   [USB setup](https://github.com/Gazeora/Gazeora/blob/master/docs/streaming.md#usb-setup).

## Documentation

| Guide | Covers |
|---|---|
| [User guide](docs/user-guide.md) | Connecting, the player, touch, VR viewer, hand pointer, head tracking |
| [Permissions](docs/permissions.md) | Each permission, when it is asked for, and what happens without it |
| [Troubleshooting](docs/troubleshooting.md) | The app's error messages and what to do |
| [Architecture](docs/architecture.md) | Layers, the native client, project structure |
| [Development](docs/development.md) | Building, tests, versions, releases |
| [Stream format](https://github.com/Gazeora/Gazeora/blob/master/docs/stream-format.md) | The wire protocol between the PC and this app (canonical, in the Gazeora repository) |

## Known limitations

- The stream and the input sent back to the PC are not encrypted or authenticated.
- RTSP over UDP does not work from the Android emulator; use another protocol there.
- USB needs a physical phone with USB accessory support.
- H.265 needs an H.265 decoder on the phone.
- The hand pointer works only in a VR viewer over Gazeora's own window, needs the camera
  permission, and needs enough light. Games are never shown with it.
- Head tracking needs Gazeora on the PC to have VR mode on with the lens correction done by the
  phone. Roll is not used by the PC.
- Released APKs and CI builds leave out the camera status sent to the PC's stream overlay
  (`-Pgazeora.streamOverlay=false`).
- Compatibility is not verified per phone model.

## Contributing

Bug reports, phone reports and pull requests are welcome. See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

Copyright (C) 2026 Nithin Aashik. Gazeora for Android is free software under the
[GNU General Public License version 3](LICENSE).

[NOTICE](NOTICE) adds terms under section 7 of the GPL: modified versions keep the attribution
"Gazeora by Nithin Aashik" where they show information about themselves, are marked as
modified, and do not use the Gazeora name or logo as their own (see
[TRADEMARKS.md](TRADEMARKS.md)). The libraries and the hand model the app is built with are
listed in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

Gazeora for Android is also available under a commercial license. See
[COMMERCIAL-LICENSE.md](COMMERCIAL-LICENSE.md).
