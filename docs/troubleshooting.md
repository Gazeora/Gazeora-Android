# Troubleshooting

The messages the app shows, what they mean, and what to try. Problems with capture or with the
stream on the PC are covered by Gazeora's
[troubleshooting guide](https://github.com/Gazeora/Gazeora/blob/master/docs/troubleshooting.md).

When reporting a problem, include the app version (under **About** on the home screen), your
phone model and Android version, the protocol and the codec.

## Finding the PC

### "Looking for your PC…" and nothing appears

- Check that Gazeora is running on the PC and that the phone is on the same network.
- Windows Firewall on the PC must allow Gazeora on this network. Discovery uses UDP port 8556.
- Some Wi-Fi networks isolate devices from each other. Then the app cannot reach the PC on that
  network at all; try another network or USB.
- If the PC is on a network larger than /22, the app only finds it by broadcast. Use
  **Connect manually** if broadcasts are blocked.

### "Local network access is needed to find your PC."

On Android 17 and later, the local network permission was denied. Tap **Allow network access**,
or allow it in the app's Android settings. See [Permissions](permissions.md#local-network).

### "Enter an address like 192.168.1.20:8554"

The manual address could not be read. Enter `host` or `host:port`. IPv6 addresses are not
supported.

## Playing

### "No video over RTSP/UDP. Switch the PC to RTSP over TCP."

RTSP over UDP was set up, but no video arrived. A firewall or NAT between the PC and the phone
is blocking the UDP packets; this is always the case from the Android emulator. In Gazeora on
the PC, set **Settings → Frame stream → Protocol** to **RTSP over TCP**.

### "No picture from …:…. Is the PC streaming over UDP?"

The app is connected with the UDP protocol but no frames are arriving. Check that the PC's
protocol is UDP, and that its firewall allows Gazeora. If the PC changed protocol, go back and
pick it again from **Your PCs**.

### "No picture from …"

The connection is open but no frames have arrived for a while. Check that Gazeora is still
running on the PC and that the network is working.

### "Can't play the stream from …"

Playing stopped. When a detail is shown under the message, it says why. Tap **Retry**. If the PC changed its
codec or protocol, it disconnected every player; going back and connecting again picks up the
new settings.

### The picture stutters, smears or lags

These come from the network or the PC's stream settings. See Gazeora's
[Wi-Fi and packet loss](https://github.com/Gazeora/Gazeora/blob/master/docs/streaming.md#wi-fi-and-packet-loss).
H.265 needs an H.265 decoder on the phone; if it does not play, switch the PC to H.264.

## USB

| Message | Meaning | What to do |
|---|---|---|
| *Waiting for the PC on USB…* | No accessory connection yet, or the cable was unplugged | Plug in the cable. The PC must have Gazeora running with the USB protocol and its USB driver installed |
| *Allow USB access to connect* | Android is waiting for permission to use the accessory | Allow it in Android's dialog |
| *Couldn't open the USB connection* | Android gave access, but the connection could not be opened | Unplug and plug in again |

USB does not work with the Android emulator. See Gazeora's
[USB setup](https://github.com/Gazeora/Gazeora/blob/master/docs/streaming.md#usb-setup) for the
PC side.

## VR viewer

### The hand pointer does not appear

- It appears only in a VR viewer over Gazeora's window, never over a game.
- The camera permission must be granted. See [Permissions](permissions.md#camera).
- Another app may be using the camera.
- The room may be too dark: the camera turns off after 3 seconds of darkness and checks again
  every 10 seconds.
- The models may still be loading after the app starts.

### Head tracking does nothing

- Head tracking runs only while the PC sends frames laid out for a VR viewer: VR mode on, with
  **Lens correction done by → The phone**.
- On the PC, check **Controls → Head tracking**. Its status line says whether poses are arriving
  and whether they reach a game.
