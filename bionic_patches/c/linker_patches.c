/*
 * Linker patches - C code
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// Linker initialization with environment variable patches
// ============================================================================

typedef struct {
    const char* ld_library_path;
    const char* ld_preload;
    const char* ld_shim_libs;
    const char* ld_origin_path;
    const char* ld_profile;
} linker_env_vars_t;

void* linker_init_patched(void* args, void* dynamic_info) {
    // Always check environment variables regardless of AT_SECURE
    const char* ld_library_path = getenv("LD_LIBRARY_PATH");
    const char* ld_preload = getenv("LD_PRELOAD");
    const char* ld_shim_libs = getenv("LD_SHIM_LIBS");
    
    // Process environment variables
    if (ld_library_path != NULL) {
        // INFO("[ LD_LIBRARY_PATH set to \"%s\" ]", ld_library_path);
    }
    
    if (ld_preload != NULL) {
        // INFO("[ LD_PRELOAD set to \"%s\" ]", ld_preload);
    }
    
    if (ld_shim_libs != NULL) {
        // INFO("[ LD_SHIM_LIBS set to \"%s\" ]", ld_shim_libs);
    }
    
    (void)args;
    return dynamic_info;
}

// ============================================================================
// Environment variable processing
// ============================================================================

int linker_process_env_vars(linker_env_vars_t* vars) {
    if (vars == NULL) {
        return -1;
    }
    
    vars->ld_library_path = getenv("LD_LIBRARY_PATH");
    vars->ld_preload = getenv("LD_PRELOAD");
    vars->ld_shim_libs = getenv("LD_SHIM_LIBS");
    vars->ld_origin_path = getenv("LD_ORIGIN_PATH");
    vars->ld_profile = getenv("LD_PROFILE");
    
    return 0;
}
