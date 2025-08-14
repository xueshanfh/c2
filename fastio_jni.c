
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <jni.h>

#define CHUNK_SIZE 262144  // 256KB chunks for better stability
#define MAX_MMAP_SIZE (100 * 1024 * 1024)  // Don't mmap files larger than 100MB

// Safe chunked reading with proper error handling
static jstring read_file_safe(JNIEnv *env, const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        return NULL;
    }
    
    // Get file size using fseek/ftell for compatibility
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }
    
    long file_size = ftell(file);
    if (file_size < 0) {
        fclose(file);
        return NULL;
    }
    
    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    
    if (file_size == 0) {
        fclose(file);
        return (*env)->NewStringUTF(env, "");
    }
    
    // For very large files, use streaming approach
    if (file_size > MAX_MMAP_SIZE) {
        fclose(file);
        return NULL; // Reject files that are too large
    }
    
    // Allocate buffer with extra space for null terminator
    char *buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }
    
    // Read file in chunks
    size_t total_read = 0;
    size_t remaining = (size_t)file_size;
    
    while (remaining > 0 && !feof(file)) {
        size_t to_read = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;
        size_t nread = fread(buffer + total_read, 1, to_read, file);
        
        if (nread == 0) {
            if (ferror(file)) {
                free(buffer);
                fclose(file);
                return NULL;
            }
            break; // EOF
        }
        
        total_read += nread;
        remaining -= nread;
    }
    
    fclose(file);
    
    // Ensure null termination
    buffer[total_read] = '\0';
    
    // Create Java string with explicit length
    jstring result = (*env)->NewStringUTF(env, buffer);
    free(buffer);
    
    // Check for JNI exceptions
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionClear(env);
        return NULL;
    }
    
    return result;
}

// Memory-mapped reading for smaller files only
static jstring read_file_mmap_safe(JNIEnv *env, const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        return NULL;
    }
    
    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        return NULL;
    }
    
    size_t file_size = (size_t)st.st_size;
    
    // Only use mmap for reasonably sized files
    if (file_size == 0 || file_size > MAX_MMAP_SIZE) {
        close(fd);
        return NULL;
    }
    
    // Memory map the file
    void *mapped = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        close(fd);
        return NULL;
    }
    
    // Advise kernel for sequential reading
    madvise(mapped, file_size, MADV_SEQUENTIAL);
    
    // Create a null-terminated copy for safety
    char *buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        munmap(mapped, file_size);
        close(fd);
        return NULL;
    }
    
    memcpy(buffer, mapped, file_size);
    buffer[file_size] = '\0';
    
    // Clean up mmap
    munmap(mapped, file_size);
    close(fd);
    
    // Create Java string
    jstring result = (*env)->NewStringUTF(env, buffer);
    free(buffer);
    
    // Check for JNI exceptions
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionClear(env);
        return NULL;
    }
    
    return result;
}

// Core file reading implementation with fallback strategy
static jstring read_file_native_impl(JNIEnv *env, jstring jpath) {
    if (!jpath) {
        return NULL;
    }
    
    // Convert Java string to C string
    const char *path = (*env)->GetStringUTFChars(env, jpath, NULL);
    if (!path) {
        return NULL;
    }
    
    jstring result = NULL;
    
    // Try memory mapping for smaller files first
    result = read_file_mmap_safe(env, path);
    
    // Fallback to safe file reading
    if (!result) {
        result = read_file_safe(env, path);
    }
    
    (*env)->ReleaseStringUTFChars(env, jpath, path);
    return result;
}

// EXACT JNI signature that matches your Smali code
JNIEXPORT jstring JNICALL
Java_luaj_lib_PackageLib_00024loadlib_00024FastIOFunction_readFileNative(JNIEnv *env, jclass clazz, jstring jpath) {
    return read_file_native_impl(env, jpath);
}

// Alternative JNI export for simpler class paths
JNIEXPORT jstring JNICALL
Java_FastIONative_readFileNative(JNIEnv *env, jclass clazz, jstring jpath) {
    return read_file_native_impl(env, jpath);
}
