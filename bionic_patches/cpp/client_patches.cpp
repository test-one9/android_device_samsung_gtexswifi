/*
 * Client patches for SurfaceFlinger - C++ code
 */

#include <surfaceflinger/Client.h>
#include <surfaceflinger/Layer.h>
#include <utils/RefBase.h>
#include <utils/Log.h>

using namespace android;

// ============================================================================
// Client destructor with safe layer removal
// ============================================================================

Client::~Client() {
    const size_t count = mLayers.size();
    for (size_t i = 0; i < count; i++) {
        sp<Layer> layer(mLayers.valueAt(i).promote());
        if (layer != 0) {
            mFlinger->removeLayer(layer);
        }
    }
}
