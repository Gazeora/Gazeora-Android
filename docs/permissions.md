# Permissions

The permissions the app declares in
[`AndroidManifest.xml`](../app/src/main/AndroidManifest.xml), when it asks for them, and what
stops working without them.

| Permission | Kind | Asked for | Needed for |
|---|---|---|---|
| `INTERNET` | Install-time | Never | Every network protocol and discovery |
| `ACCESS_LOCAL_NETWORK` | Runtime, Android 17 (API 37) and later | When the home screen first starts, and on **Watch** in **Connect manually** | Reaching a PC on the local network |
| `WAKE_LOCK` | Install-time | Never | The low-latency Wi-Fi lock while a stream plays |
| `CAMERA` | Runtime | When the player opens | The hand pointer in a VR viewer |
| USB accessory access | Per-accessory dialog from Android | When the PC's accessory is attached | The USB protocol |

The manifest also declares the `android.hardware.camera` and `android.hardware.usb.accessory`
features as not required, so the app installs on phones without a camera or USB accessory
support.

## Local network

On Android 17 (API level 37) and later, Android drops the app's traffic to local network
addresses unless the local network permission is granted. On earlier versions there is no such
permission, and the app treats it as granted.

- The app asks for it once when the home screen first starts, for discovery, and again when
  you tap **Watch** under **Connect manually** while it is still missing.
- Loopback addresses (`127.*`, used with `adb reverse`) do not need it.
- USB does not need it.

If it is denied, the home screen shows *Local network access is needed to find your PC.* and an
**Allow network access** button. Tapping it asks again. If Android will no longer show the
request (denied for good), the button opens the app's page in Android Settings instead. If it is
denied after tapping **Watch**, the address field shows *Allow local network
access in Settings, or use USB.*

## Camera

The back camera finds your hands for the [hand pointer](user-guide.md#hand-pointer). Its
picture is never shown or sent to the PC; only pointer actions (and, in overlay builds, the
camera's state and brightness) are sent.

- The player asks for it each time it opens fresh (not when it is restored), if it is not
  granted yet. It asks then because the request cannot be answered once the phone is inside the
  viewer.
- If it is denied, everything else works. Gazeora's window shows in the viewer without a hand
  pointer. Whether the camera can be used is checked again by itself, so granting it later in
  Settings takes effect without restarting the stream.

## USB accessory

When the PC switches the phone to accessory mode, Android asks whether to let the app use the
accessory. While waiting for that answer, the player shows *Allow USB access to connect*.
Without it, the USB protocol cannot connect; the network protocols still work.
