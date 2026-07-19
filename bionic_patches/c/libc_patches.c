/*
 * libc patches - C code
 */

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

// ============================================================================
// Environment variable handling
// ============================================================================

int __is_unsafe_environment_variable_patched(const char* name) {
    // Modified to allow LD_SHIM_LIBS
    const char* unsafe_vars[] = {
        "LD_ORIGIN_PATH",
        "LD_PRELOAD",
        "LD_PROFILE",
        //"LD_SHIM_LIBS",  // Commented out to allow this variable
        "LD_SHOW_AUXV",
        "LD_USE_LOAD_BIAS",
        "LOCALDOMAIN",
        NULL
    };
    
    for (int i = 0; unsafe_vars[i] != NULL; i++) {
        if (strcmp(name, unsafe_vars[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

// ============================================================================
// stdio patches - vsnprintx implementation
// ============================================================================

int vsnprintx(char * __restrict buffer, size_t size, 
              const char * __restrict format, va_list args) {
    // Stub implementation that always returns 0
    // In practice, this would be a secure version of vsnprintf
    (void)buffer;
    (void)size;
    (void)format;
    (void)args;
    return 0;
}
