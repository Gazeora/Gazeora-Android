# User guide

How to connect to a PC, and how the player, touch input, VR viewer, hand pointer and head
tracking behave. Settings for the stream itself (protocol, codec, quality, VR lens settings)
are chosen in Gazeora on the PC; see Gazeora's
[streaming guide](https://github.com/Gazeora/Gazeora/blob/master/docs/streaming.md).

## Connecting

The home screen shows **Your PCs**, the PCs that answered discovery, while it shows
*Looking for your PC…*.

### Found PCs

The app looks for PCs running Gazeora by itself:

- Once a second it broadcasts a discovery request on each of the phone's IPv4 networks. On the
  emulator it also asks the host PC at `10.0.2.2`.
- Every 3 seconds it also asks each address on networks of up to 1022 addresses (/22 or
  smaller), because many Wi-Fi networks drop broadcasts.
- A PC that has not answered for 7 seconds is removed from the list.

Each entry shows the PC's name, protocol, address and codec. Tap it to watch. A PC that streams
over USB shows *USB · plug in to watch*.

On Android 17 and later, discovery needs the local network permission. Without it the screen
shows *Local network access is needed to find your PC.* and an **Allow network access** button.
See [Permissions](permissions.md#local-network).

### Manual connection

Under **Connect manually**, pick a **Protocol**, type the **PC address** as `host` or
`host:port`, and tap **Watch**. Without a port, `8554` is used. IPv6 addresses are not
supported. The app remembers the last protocol and address.

### USB

1. Set up the USB driver on the PC once (see Gazeora's
   [USB setup](https://github.com/Gazeora/Gazeora/blob/master/docs/streaming.md#usb-setup)).
2. Plug the phone in. The PC switches it to USB accessory mode, and Android opens the app.
3. Android asks whether to allow the app to use the accessory. Allow it.

**PC on the USB cable** appears on the home screen while the phone is in accessory mode. The app
checks the cable twice a second, so it reconnects by itself when the cable is plugged in again.

### When the PC changes protocol

If the PC's protocol changes while you watch (for example from a network protocol to USB),
the PC restarts its stream and announces the new protocol in discovery. The player follows it to
the new protocol.

## The player

The player fills the screen, including any display cutout, hides the system bars, and keeps
the screen on. It runs in landscape and turns with the phone between the two landscape
orientations. While it plays, it holds a low-latency Wi-Fi lock, because Wi-Fi power saving
delays and drops packets.

Frames are decoded with `MediaCodec`. For each codec, the app picks a decoder and turns on
low-latency decoding when that decoder supports it.

While connecting, the player shows *Connecting to …*. If the stream cannot play, it shows an
error with **Retry**. See [Troubleshooting](troubleshooting.md).

## Touch on a flat picture

While the picture is flat (not laid out for a VR viewer), touches on it control Gazeora's
window on the PC like a touch screen:

- A tap clicks at that spot. The press and release are sent as one message, so a lost packet
  cannot leave a button held down.
- A finger that moves, or a second finger, scrolls.

Touches are sent to the PC over the USB cable, or as UDP datagrams to the PC's port 8557. The
PC uses them only for its own window, and ignores them while a game is launching or running.

## VR viewer

When Gazeora on the PC has VR mode on with **Lens correction done by → The phone**, each frame
carries the lens settings, and the app draws the picture into both eyes with OpenGL ES, applying
the lens distortion and color-fringe correction itself. With 3D from depth on the PC, each frame
holds a view per eye side by side, and the app stretches each to its eye.

With **Lens correction done by → This PC**, frames arrive already split and warped, and the app
shows them as a flat picture.

While the picture is laid out for a viewer, the screen stays in the landscape orientation it
was in, so tilting your head does not flip it. A flat picture turns with the phone again.

Play full screen, then put the phone in the viewer.

## Hand pointer

In a VR viewer, while the PC shows Gazeora's own window (no game runs), the back camera tracks
your hand so you can point at the window. The camera's picture is never shown.

- The middle of your palm moves a ring cursor. The middle of the camera's view, well inside its
  edges, maps to the whole window, so the corners are easy to reach.
- A pinch (thumb and index finger) clicks where the cursor was when the pinch began. The ring
  closes in as a pinch nears and fills when it pinches.
- A pinch that moves scrolls.
- The cursor is steadied against tremor, and stays still while your hand is still.
- Your right hand points; without it, your left. A hand already pointing keeps pointing.
- Only your own hands count: a hand is taken as yours when it is within arm's reach and its arm
  comes from outside the camera's view. Other people's hands are ignored.
- While no hand points, the app draws your tracked hands over the window.

Requirements and behavior:

- The camera permission. The player asks for it when it opens, before the phone goes into the
  viewer. See [Permissions](permissions.md#camera).
- Enough light. Dim pictures are brightened, and the camera may drop from 30 to 15 frames a
  second to gather more light. If the picture stays dark for 3 seconds, the camera turns off to
  save power, then turns on briefly every 10 seconds to check whether the light is back.
- The models load when the app starts, first on the CPU and then on the GPU where that is
  faster. Tracking switches to the faster one when it is ready. The first GPU load after an
  install can take a while.
- Games are never shown with the hand pointer. Only Gazeora's window is.

In builds with the stream overlay enabled (not the released APKs), the app also sends the
camera's state and brightness to the PC, for its stream overlay, when it changes and every
second.

Debuggable builds also show the hand pointer over a flat picture of Gazeora's window, so it can
be tried without a viewer. Release builds show it only in a VR viewer.

## Head tracking

While the PC sends frames laid out for a VR viewer (VR mode on, lens correction done by the
phone), the app reads the phone's game rotation vector sensor (gyroscope and accelerometer,
without the compass) at 200 Hz. Phones without that sensor fall back to the rotation vector
sensor. The sensor runs only while the picture is laid out for a viewer, including over
Gazeora's window.

The app sends the head's orientation to the PC as numbered quaternions, over the USB cable or to
UDP port 8557. Gazeora turns each change into mouse or controller-stick movement for the game in
front. How the PC uses them, and its settings, are described in Gazeora's
[head tracking guide](https://github.com/Gazeora/Gazeora/blob/master/docs/vr-viewer.md#head-tracking).

The orientation's coordinate frame is defined in the
[stream format](https://github.com/Gazeora/Gazeora/blob/master/docs/stream-format.md#headpose-type-7).
It accounts for which landscape orientation the phone is in.
