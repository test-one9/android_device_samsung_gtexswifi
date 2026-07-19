/*
 * Sparse image patches - C code
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// ============================================================================
// Sparse format constants
// ============================================================================

#define SPARSE_HEADER_MAGIC     0xed26ff3a
#define SAMSUNG_CHUNK_MAGIC     0xf7776f58

// ============================================================================
// Sparse header structure
// ============================================================================

typedef struct sparse_header_patched {
    uint32_t magic;
    uint16_t major_version;
    uint16_t minor_version;
    uint16_t file_hdr_sz;
    uint16_t chunk_hdr_sz;
    uint32_t blk_sz;
    uint32_t total_blks;
    uint32_t total_chunks;
    uint32_t image_checksum;
    uint32_t samsung_reserved;  // Added Samsung reserved field
} sparse_header_patched_t;

// ============================================================================
// Chunk header structure
// ============================================================================

typedef struct chunk_header_patched {
    uint16_t chunk_type;
    uint16_t reserved1;
    uint32_t chunk_sz;
    uint32_t total_sz;
    uint32_t samsung_magic;     // Added Samsung magic field
} chunk_header_patched_t;

// ============================================================================
// Initialize sparse header
// ============================================================================

void sparse_header_init_patched(sparse_header_patched_t* header) {
    if (header == NULL) {
        return;
    }
    
    header->magic = SPARSE_HEADER_MAGIC;
    header->major_version = 1;
    header->minor_version = 0;
    header->file_hdr_sz = sizeof(sparse_header_patched_t);
    header->chunk_hdr_sz = sizeof(chunk_header_patched_t);
    header->blk_sz = 4096;
    header->total_blks = 0;
    header->total_chunks = 0;
    header->image_checksum = 0;
    header->samsung_reserved = 0;
}

// ============================================================================
// Write sparse chunks with Samsung magic
// ============================================================================

int sparse_write_chunk_patched(void* out, int chunk_type, uint32_t chunk_sz, 
                                uint32_t total_sz, const void* data) {
    if (out == NULL) {
        return -1;
    }
    
    chunk_header_patched_t chunk_header;
    chunk_header.chunk_type = (uint16_t)chunk_type;
    chunk_header.reserved1 = 0;
    chunk_header.chunk_sz = chunk_sz;
    chunk_header.total_sz = total_sz;
    chunk_header.samsung_magic = SAMSUNG_CHUNK_MAGIC;
    
    (void)data;
    // Write chunk header
    // return write(out, &chunk_header, sizeof(chunk_header));
    return 0;
}

// ============================================================================
// Get Samsung magic value
// ============================================================================

uint32_t get_samsung_chunk_magic(void) {
    return SAMSUNG_CHUNK_MAGIC;
}

// ============================================================================
// Add Samsung magic to chunk
// ============================================================================

void add_samsung_magic_to_chunk(chunk_header_patched_t* chunk) {
    if (chunk != NULL) {
        chunk->samsung_magic = SAMSUNG_CHUNK_MAGIC;
    }
}
