/*
 * Binder patches - C++ code
 */

#include <binder/IServiceManager.h>
#include <utils/String8.h>
#include <utils/String16.h>
#include <unistd.h>
#include <string.h>

using namespace android;

// ============================================================================
// Binder service manager patches
// ============================================================================

class BpServiceManagerPatched : public BpServiceManager {
public:
    BpServiceManagerPatched(const sp<IBinder>& impl) : BpServiceManager(impl) {}
    
    virtual sp<IBinder> getService(const String16& name) const {
        // Special handling for atchannel service
        String8 name8(name);
        if (strcmp(name8.string(), "atchannel") == 0) {
            // Sleep for 1 second to allow service to initialize
            sleep(1);
            return NULL;
        }
        
        // Call original implementation
        return BpServiceManager::getService(name);
    }
};

// ============================================================================
// Factory function for patched service manager
// ============================================================================

sp<IServiceManager> getPatchedServiceManager(const sp<IBinder>& binder) {
    return new BpServiceManagerPatched(binder);
}
