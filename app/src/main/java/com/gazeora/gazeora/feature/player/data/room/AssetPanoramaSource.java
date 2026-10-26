package com.gazeora.gazeora.feature.player.data.room;

import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.util.Consumer;

import java.io.IOException;
import java.io.InputStream;
import java.util.concurrent.Executor;

import javax.inject.Inject;

import dagger.hilt.android.qualifiers.ApplicationContext;

/**
 * Adapter pattern: the scene in the app's assets (tools/panorama/render_panorama.py makes
 * it), decoded on the background executor.
 */
public final class AssetPanoramaSource implements PanoramaSource {
    private static final String TAG = "GazeoraPanorama";
    private static final String ASSET = "room/panorama.jpg";

    private final AssetManager assets;
    private final Executor background;

    @Inject
    public AssetPanoramaSource(@NonNull @ApplicationContext Context context,
            @NonNull Executor background) {
        this.assets = context.getAssets();
        this.background = background;
    }

    @Override
    public void load(@NonNull Consumer<Bitmap> onLoaded) {
        background.execute(() -> {
            Bitmap scene = decode();
            if (scene != null) {
                onLoaded.accept(scene);
            }
        });
    }

    @Nullable
    private Bitmap decode() {
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inPreferredConfig = Bitmap.Config.ARGB_8888;  // RGBA in memory, as GL takes it
        options.inScaled = false;
        try (InputStream in = assets.open(ASSET)) {
            return BitmapFactory.decodeStream(in, null, options);
        } catch (IOException e) {
            Log.w(TAG, "can't read " + ASSET, e);
            return null;
        }
    }
}
