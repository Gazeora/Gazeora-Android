#include "hands/camera_setup.h"

#include <media/NdkImage.h>

#include <vector>

namespace gazeora {

namespace {

// The hand tracker looks at a couple of hundred pixels; more only costs converting them.
constexpr int32_t kMaxStreamWidth = 640;
constexpr float kNoZoom = 1.0f;
// The hand tracker can't keep up with more frames a second than this on a phone; and in a dim
// room, the camera may slow to this, to let in light for longer.
constexpr int32_t kMostFramesPerSecond = 30;
constexpr int32_t kLeastFramesPerSecond = 15;
constexpr uint32_t kConfigFields = 4;  // format, width, height, input

const ACameraMetadata_const_entry* Entry(const ACameraMetadata* metadata, uint32_t tag,
                                         ACameraMetadata_const_entry& entry, uint32_t count) {
    return ACameraMetadata_getConstEntry(metadata, tag, &entry) == ACAMERA_OK &&
                   entry.count >= count
               ? &entry
               : nullptr;
}

}  // namespace

bool CameraSetup::Read(const ACameraMetadata* characteristics) {
    ACameraMetadata_const_entry facing, configs, orientation, focal, physical, pixels, active;
    if (!Entry(characteristics, ACAMERA_LENS_FACING, facing, 1) ||
        facing.data.u8[0] != ACAMERA_LENS_FACING_BACK ||
        !Entry(characteristics, ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, configs,
               kConfigFields)) {
        return false;
    }
    bool activeKnown = Entry(characteristics, ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE, active, 4);
    // (left, top, width, height)
    int32_t activeWidth = activeKnown ? active.data.i32[2] : 0;
    int32_t activeHeight = activeKnown ? active.data.i32[3] : 0;
    float activeAspect = activeHeight > 0 ? float(activeWidth) / activeHeight : 4.0f / 3;

    std::vector<camera_geometry::Size> sizes;
    for (uint32_t i = 0; i + kConfigFields <= configs.count; i += kConfigFields) {
        const int32_t* config = configs.data.i32 + i;
        if (config[3] == ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT &&
            config[0] == AIMAGE_FORMAT_YUV_420_888) {
            sizes.push_back({config[1], config[2]});
        }
    }
    size = camera_geometry::PickStreamSize(sizes, activeAspect, kMaxStreamWidth);
    if (size.width == 0) {
        return false;
    }

    if (Entry(characteristics, ACAMERA_SENSOR_ORIENTATION, orientation, 1)) {
        sensorOrientation = orientation.data.i32[0];
    }
    if (activeKnown &&
        Entry(characteristics, ACAMERA_LENS_INFO_AVAILABLE_FOCAL_LENGTHS, focal, 1) &&
        Entry(characteristics, ACAMERA_SENSOR_INFO_PHYSICAL_SIZE, physical, 2) &&
        Entry(characteristics, ACAMERA_SENSOR_INFO_PIXEL_ARRAY_SIZE, pixels, 2)) {
        camera_geometry::SetAngles(view, focal.data.f[0], physical.data.f[0], physical.data.f[1],
                                   pixels.data.i32[0], pixels.data.i32[1], activeWidth,
                                   activeHeight, size.width, size.height);
    }
    ACameraMetadata_const_entry fps;
    if (Entry(characteristics, ACAMERA_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES, fps, 2)) {
        std::vector<camera_geometry::FrameRates> ranges;
        for (uint32_t i = 0; i + 2 <= fps.count; i += 2) {
            ranges.push_back({fps.data.i32[i], fps.data.i32[i + 1]});
        }
        frameRates = camera_geometry::PickFrameRates(ranges, kLeastFramesPerSecond, kMostFramesPerSecond);
    }
    // Zooming out widens the view by the same ratio.
    ACameraMetadata_const_entry zoom;
    if (Entry(characteristics, ACAMERA_CONTROL_ZOOM_RATIO_RANGE, zoom, 2) && zoom.data.f[0] > 0 &&
        zoom.data.f[0] < kNoZoom) {
        zoomRatio = zoom.data.f[0];
        view.tanAcross /= zoomRatio;
        view.tanDown /= zoomRatio;
    }
    return true;
}

}  // namespace gazeora
