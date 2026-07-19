/*
 * HWC1 patches for SurfaceFlinger - C++ code
 */

#include <surfaceflinger/SurfaceFlinger.h>
#include <surfaceflinger/DisplayDevice.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include <stdlib.h>

using namespace android;

// ============================================================================
// HWC1 init with drop missed frames
// ============================================================================

SurfaceFlinger::SurfaceFlinger() {
    char value[PROPERTY_VALUE_MAX];
    
    property_get("ro.bq.gpu_to_cpu_unsupported", value, "0");
    mGpuToCpuSupported = !atoi(value);
    
    property_get("debug.sf.drop_missed_frames", value, "0");
    mDropMissedFrames = atoi(value);
    
    property_get("debug.sf.showupdates", value, "0");
    mDebugRegion = atoi(value);
    
    property_get("debug.sf.disable_hwc", value, "0");
    mDebugDisableHWC = atoi(value);
}

// ============================================================================
// HWC1 getDisplayConfigs with color transform
// ============================================================================

status_t SurfaceFlinger::getDisplayConfigs(const sp<IBinder>& display, 
                                           Vector<DisplayInfo>& configs) {
    if (!display.get()) {
        return NAME_NOT_FOUND;
    }
    
    int32_t type = NAME_NOT_FOUND;
    for (int i = 0; i < DisplayDevice::NUM_BUILTIN_DISPLAY_TYPES; i++) {
        if (display == mBuiltinDisplays[i]) {
            type = i;
            break;
        }
    }
    
    if (type < 0) {
        return type;
    }
    
    // Get display info
    DisplayInfo info;
    info.secure = false;
    info.appVsyncOffset = VSYNC_EVENT_PHASE_OFFSET_NS;
    info.presentationDeadline = 0;
    info.colorTransform = 0;
    
    // Get DPI from properties
    char valuef[PROPERTY_VALUE_MAX];
    float xdpi = 0, ydpi = 0;
    property_get("ro.sf.xdpi", valuef, "0");
    xdpi = atof(valuef);
    property_get("ro.sf.ydpi", valuef, "0");
    ydpi = atof(valuef);
    
    if (xdpi == 0) xdpi = 160;
    if (ydpi == 0) ydpi = 160;
    
    configs.push_back(info);
    return NO_ERROR;
}

// ============================================================================
// HWC1 handle refresh with drop missed frames
// ============================================================================

void SurfaceFlinger::handleMessageRefresh() {
    ATRACE_CALL();
    
    nsecs_t refreshStartTime = systemTime(SYSTEM_TIME_MONOTONIC);
    
    static nsecs_t previousExpectedPresent = 0;
    nsecs_t expectedPresent = mPrimaryDispSync.computeNextRefresh(0);
    static bool previousFrameMissed = false;
    bool frameMissed = (expectedPresent == previousExpectedPresent);
    
    if (frameMissed != previousFrameMissed) {
        ATRACE_INT("FrameMissed", static_cast<int>(frameMissed));
    }
    previousFrameMissed = frameMissed;
    
    if (CC_UNLIKELY(mDropMissedFrames && frameMissed)) {
        preComposition();
        repaintEverything();
    } else {
        preComposition();
        rebuildLayerStacks();
        setUpHWComposer();
        doDebugFlashRegions();
        doComposition();
        postComposition(refreshStartTime);
    }
    
    previousExpectedPresent = mPrimaryDispSync.computeNextRefresh(0);
}

// ============================================================================
// HWC1 post composition with fence tracking
// ============================================================================

void SurfaceFlinger::postComposition(nsecs_t refreshStartTime) {
    const LayerVector& layers(mDrawingState.layersSortedByZ);
    const size_t count = layers.size();
    
    sp<Fence> presentFence;
    sp<const DisplayDevice> hw(getDefaultDisplayDevice());
    
    if (hw != NULL) {
        presentFence = hw->getDisplayFence();
    }
    
#ifdef ENABLE_FENCE_TRACKING
    mFenceTracker.addFrame(refreshStartTime, presentFence,
            hw->getVisibleLayersSortedByZ(), hw->getClientTargetAcquireFence());
#endif
    
    if (mAnimCompositionPending) {
        mAnimCompositionPending = false;
        postMessageInvalidate();
    }
}

// ============================================================================
// HWC1 rebuild layer stacks with layer stack filtering
// ============================================================================

void SurfaceFlinger::rebuildLayerStacks() {
    const LayerVector& layers(mDrawingState.layersSortedByZ);
    
    for (size_t i = 0; i < mDisplays.size(); i++) {
        const sp<DisplayDevice>& hw = mDisplays[i];
        const Transform& tr = hw->getTransform();
        const Region& bounds = hw->getBounds();
        
        for (size_t j = 0; j < layers.size(); j++) {
            const sp<Layer>& layer = layers[j];
            const Layer::State& s = layer->getDrawingState();
            
            if (s.layerStack == hw->getLayerStack()) {
                Region drawRegion(tr.transform(layer->visibleNonTransparentRegion));
                drawRegion.andSelf(bounds);
                // Add to display's layer stack
            }
        }
    }
}

// ============================================================================
// HWC1 layer destroy handler
// ============================================================================

status_t SurfaceFlinger::onLayerDestroyed(const wp<Layer>& layer) {
    status_t err = NO_ERROR;
    sp<Layer> l(layer.promote());
    if (l != NULL) {
        err = removeLayer(l);
        ALOGE_IF(err < 0 && err != NAME_NOT_FOUND,
                "error removing layer=%p (%s)", l.get(), strerror(-err));
    }
    return err;
}
