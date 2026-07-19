/*
 * Thread patches - C code
 */

#include <unistd.h>
#include <sys/types.h>

// ============================================================================
// Get thread ID
// ============================================================================

pid_t androidGetTid(void) {
#ifdef HAVE_GETTID
    return gettid();
#else
    return getpid();
#endif
}
