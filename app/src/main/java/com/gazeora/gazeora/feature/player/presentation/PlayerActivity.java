package com.gazeora.gazeora.feature.player.presentation;

import android.os.Bundle;
import android.view.SurfaceView;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.lifecycle.ViewModelProvider;

import com.gazeora.gazeora.R;
import com.gazeora.gazeora.core.ui.AspectRatioLayout;
import com.gazeora.gazeora.feature.player.presentation.components.ImmersiveWindow;
import com.gazeora.gazeora.feature.player.presentation.components.PictureTouchListener;
import com.gazeora.gazeora.feature.player.presentation.components.StatusOverlay;
import com.gazeora.gazeora.feature.player.presentation.components.SurfaceCallback;

import dagger.hilt.android.AndroidEntryPoint;

/**
 * Plays the PC's stream full screen; frames are decoded straight onto the surface, and touches
 * on it go to the PC.
 */
@AndroidEntryPoint
public final class PlayerActivity extends AppCompatActivity {
    private PlayerViewModel viewModel;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_player);
        viewModel = new ViewModelProvider(this).get(PlayerViewModel.class);
        AspectRatioLayout frameLayout = findViewById(R.id.frame_layout);
        SurfaceView frameSurface = findViewById(R.id.frame_surface);
        StatusOverlay overlay = new StatusOverlay(this, viewModel::onRetry);
        frameSurface.getHolder().addCallback(
                new SurfaceCallback(viewModel::onSurfaceCreated, viewModel::onSurfaceDestroyed));
        PictureTouchListener touches = new PictureTouchListener(viewModel::onTouch);
        frameSurface.setOnTouchListener(touches);
        ImmersiveWindow.apply(getWindow());
        viewModel.getUiState().observe(this, state -> state.render(overlay));
        viewModel.getAspectRatio().observe(this, frameLayout::setAspectRatio);
        viewModel.getPicture().observe(this, touches::setPicture);
    }

    @Override
    protected void onStart() {
        super.onStart();
        viewModel.onStart();
    }

    @Override
    protected void onStop() {
        super.onStop();
        viewModel.onStop();
    }
}
