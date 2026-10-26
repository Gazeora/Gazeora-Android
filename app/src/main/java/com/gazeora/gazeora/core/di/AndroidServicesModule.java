package com.gazeora.gazeora.core.di;

import android.content.Context;
import android.hardware.usb.UsbManager;
import android.net.wifi.WifiManager;
import android.os.Handler;
import android.os.Looper;

import androidx.annotation.NonNull;

import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

import javax.inject.Singleton;

import dagger.Module;
import dagger.Provides;
import dagger.hilt.InstallIn;
import dagger.hilt.android.qualifiers.ApplicationContext;
import dagger.hilt.components.SingletonComponent;

/** Android system services, the main thread's Handler and a thread for slow work. */
@Module
@InstallIn(SingletonComponent.class)
public final class AndroidServicesModule {
    private AndroidServicesModule() {
    }

    @Provides
    @NonNull
    static UsbManager provideUsbManager(@NonNull @ApplicationContext Context context) {
        return (UsbManager) context.getSystemService(Context.USB_SERVICE);
    }

    @Provides
    @NonNull
    static WifiManager provideWifiManager(@NonNull @ApplicationContext Context context) {
        return (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
    }

    // Unscoped on purpose: each owner cancels only its own posts.
    @Provides
    @NonNull
    static Handler provideMainHandler() {
        return new Handler(Looper.getMainLooper());
    }

    // One thread, for decoding and other work too slow for the main thread.
    @Provides
    @Singleton
    @NonNull
    static Executor provideBackgroundExecutor() {
        return Executors.newSingleThreadExecutor();
    }
}
