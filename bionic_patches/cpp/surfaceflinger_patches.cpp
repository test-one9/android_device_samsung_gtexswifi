/*
 * SurfaceFlinger patches - C++ code
 */

#include <ui/DisplayInfo.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include <surfaceflinger/SurfaceFlinger.h>
#include <surfaceflinger/Layer.h>
#include <stdlib.h>
#include <string.h>

using namespace android;

// ============================================================================
// Global state for patches
// ============================================================================

static bool g_dropMissedFrames = false;
static bool g_daltonize = false;
static bool g_hasColorMatrix = false;
static float g_colorMatrix[16] = {0};
static int g_colorTransform = 0;

// ============================================================================
// SurfaceFlinger constructor with new flags
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
    
    mDaltonize = false;
    mHasColorMatrix = false;
}

// ============================================================================
// Get display configs with color transform
// ============================================================================

status_t SurfaceFlinger::getDisplayConfigs(const sp<IBinder>& display, 
                                           Vector<DisplayInfo>& configs) {
    // Get display info
    DisplayInfo info;
    
    // Set default values
    info.secure = false;
    info.appVsyncOffset = VSYNC_EVENT_PHASE_OFFSET_NS;
    info.presentationDeadline = 0;
    info.colorTransform = g_colorTransform;
    
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
    (void)display;
    return NO_ERROR;
}

// ============================================================================
// Handle refresh with drop missed frames
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
        // Latch buffers, but don't send anything to HWC
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
// Post composition with fence tracking
// ============================================================================

void SurfaceFlinger::postComposition(nsecs_t refreshStartTime) {
    ATRACE_CALL();
    
#ifdef ENABLE_FENCE_TRACKING
    sp<Fence> presentFence = mHwc->getRetireFence(HWC_DISPLAY_PRIMARY);
    mFenceTracker.addFrame(refreshStartTime, presentFence,
            hw->getVisibleLayersSortedByZ(), hw->getClientTargetAcquireFence());
#endif
    
    if (mAnimCompositionPending) {
        mAnimCompositionPending = false;
        dispatchInvalidate();
    }
}

// ============================================================================
// Set up HW composer with client composition flags
// ============================================================================

void SurfaceFlinger::setUpHWComposer() {
    for (size_t i = 0; i < mDisplays.size(); i++) {
        const sp<DisplayDevice>& displayDevice = mDisplays[i];
        
        for (size_t j = 0; j < mLayers.size(); j++) {
            const sp<Layer>& layer = mLayers[j];
            
            layer->setGeometry(displayDevice);
            
            if (mDebugDisableHWC || mDebugRegion || mDaltonize || mHasColorMatrix) {
                layer->forceClientComposition(displayDevice->getHwcDisplayId());
            }
        }
    }
}

// ============================================================================
// Display composition with color transforms
// ============================================================================

void SurfaceFlinger::doDisplayComposition(const sp<const DisplayDevice>& hw, 
                                          const Region& dirtyRegion) {
    Region dirty(dirtyRegion);
    
    hw->initComposition(dirty);
    
    if (CC_LIKELY(!mDaltonize && !mHasColorMatrix)) {
        if (!doComposeSurfaces(hw, dirtyRegion)) return;
    } else {
        RenderEngine& engine = getRenderEngine();
        mat4 colorMatrix = mColorMatrix;
        
        if (mDaltonize) {
            colorMatrix = colorMatrix * mDaltonizer();
        }
        
        mat4 oldMatrix = engine.setupColorTransform(colorMatrix);
        doComposeSurfaces(hw, dirtyRegion);
        engine.setupColorTransform(oldMatrix);
    }
    
    hw->swapRegion.orSelf(dirtyRegion);
    hw->finishComposition();
}

// ============================================================================
// Layer destroy handler
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

// ============================================================================
// Dump all with HWC disabled state
// ============================================================================

void SurfaceFlinger::dumpAllLocked(const Vector<String16>& args, 
                                   size_t& index, String8& result) {
    bool hwcDisabled = mDebugDisableHWC || mDebugRegion || mDaltonize || mHasColorMatrix;
    result.appendFormat("  h/w composer %s\n",
            hwcDisabled ? "disabled" : "enabled");
}

// ============================================================================
// Transact handler for daltonize and color matrix
// ============================================================================

status_t SurfaceFlinger::onTransact(uint32_t code, const Parcel& data, 
                                    Parcel* reply, uint32_t flags) {
    int n;
    
    switch (code) {
        case 1014: {  // Daltonize
            n = data.readInt32();
            if (n > 0) {
                mDaltonizer.setMode(ColorBlindnessMode::Daltonize);
            } else {
                mDaltonizer.setMode(ColorBlindnessMode::Simulation);
            }
            mDaltonize = n > 0;
            invalidateHwcGeometry();
            repaintEverything();
            return NO_ERROR;
        }
        case 1015: {  // Color matrix
            n = data.readInt32();
            mHasColorMatrix = n ? 1 : 0;
            if (n) {
                // Read matrix data
                // ...
            }
            return NO_ERROR;
        }
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

// ============================================================================
// Patch control functions
// ============================================================================

void SurfaceFlinger::setDaltonize(bool enabled) {
    mDaltonize = enabled;
    invalidateHwcGeometry();
    repaintEverything();
}

void SurfaceFlinger::setColorMatrix(const mat4& matrix) {
    mColorMatrix = matrix;
    mHasColorMatrix = true;
    invalidateHwcGeometry();
    repaintEverything();
}

void SurfaceFlinger::clearColorMatrix() {
    mColorMatrix = mat4();
    mHasColorMatrix = false;
    invalidateHwcGeometry();
    repaintEverything();
}

int SurfaceFlinger::getColorTransform() {
    return g_colorTransform;
}

void SurfaceFlinger::setColorTransform(int transform) {
    g_colorTransform = transform;
}
