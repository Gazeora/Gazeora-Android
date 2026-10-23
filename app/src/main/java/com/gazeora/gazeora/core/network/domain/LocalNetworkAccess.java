package com.gazeora.gazeora.core.network.domain;

/** Whether the app may reach the local network (the PC on Wi-Fi, the emulator's host). */
public interface LocalNetworkAccess {
    boolean isGranted();
}
