#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define CHUNK_SIZE (1024 * 1024)

// Direct C function exports - no Lua module system required
__attribute__((visibility("default"))) 
char* fastio_read_file_direct(const char* path, size_t* size_out) {
    if (!path) return NULL;
    
    FILE *fp = fopen(path, "rb");
    if (!fp) return NULL;
    
    // Get file size
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (file_size < 0) {
        fclose(fp);
        return NULL;
    }
    
    // Allocate buffer
    char *buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        fclose(fp);
        return NULL;
    }
    
    // Read file in chunks
    size_t total_read = 0;
    while (total_read < (size_t)file_size) {
        size_t to_read = (file_size - total_read > CHUNK_SIZE) ? 
                        CHUNK_SIZE : (file_size - total_read);
        size_t nread = fread(buffer + total_read, 1, to_read, fp);
        if (nread == 0) {
            if (ferror(fp)) {
                free(buffer);
                fclose(fp);
                return NULL;
            }
            break;
        }
        total_read += nread;
    }
    
    fclose(fp);
    buffer[total_read] = '\0';
    if (size_out) *size_out = total_read;
    return buffer;
}

__attribute__((visibility("default"))) 
void fastio_free_direct(char* buffer) {
    if (buffer) free(buffer);
}

// Export version info
__attribute__((visibility("default"))) 
const char* fastio_version() {
    return "FastIO ARM64 Direct 1.0";
}