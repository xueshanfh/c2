
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <jni.h>

#define CHUNK_SIZE 1048576  // 1MB chunks for better performance

// Memory-mapped file reading for maximum speed
static jstring read_file_mmap(JNIEnv *env, const char *path) {
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
    if (file_size == 0) {
        close(fd);
        return (*env)->NewStringUTF(env, "");
    }
    
    // Memory map the entire file for fastest access
    void *mapped = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        close(fd);
        return NULL;
    }
    
    // Advise kernel for sequential reading
    madvise(mapped, file_size, MADV_SEQUENTIAL);
    
    // Create Java string directly from mapped memory
    jstring result = (*env)->NewStringUTF(env, (const char*)mapped);
    
    // Clean up
    munmap(mapped, file_size);
    close(fd);
    
    return result;
}

// Fallback chunked reading for when mmap fails
static jstring read_file_chunked(JNIEnv *env, const char *path) {
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
    if (file_size == 0) {
        close(fd);
        return (*env)->NewStringUTF(env, "");
    }
    
    // Allocate buffer for entire file
    char *buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        close(fd);
        return NULL;
    }
    
    // Use larger buffer for system read() calls
    char read_buffer[CHUNK_SIZE];
    size_t total_read = 0;
    
    while (total_read < file_size) {
        size_t to_read = (file_size - total_read > CHUNK_SIZE) ? 
                        CHUNK_SIZE : (file_size - total_read);
        
        ssize_t nread = read(fd, read_buffer, to_read);
        if (nread <= 0) {
            if (nread == -1) {
                free(buffer);
                close(fd);
                return NULL;
            }
            break; // EOF
        }
        
        // Copy to main buffer
        memcpy(buffer + total_read, read_buffer, nread);
        total_read += nread;
    }
    
    close(fd);
    
    // Null terminate and create Java string
    buffer[total_read] = '\0';
    jstring result = (*env)->NewStringUTF(env, buffer);
    free(buffer);
    
    return result;
}

// Core file reading implementation with optimizations
static jstring read_file_native_impl(JNIEnv *env, jstring jpath) {
    // Convert Java string to C string
    const char *path = (*env)->GetStringUTFChars(env, jpath, NULL);
    if (!path) {
        return NULL;
    }
    
    // Try memory mapping first (fastest)
    jstring result = read_file_mmap(env, path);
    
    // Fallback to chunked reading if mmap fails
    if (!result) {
        result = read_file_chunked(env, path);
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
