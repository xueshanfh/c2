
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <jni.h>

// Ultra-fast file reading using direct system calls
static jstring read_file_optimized(JNIEnv *env, const char *path) {
    // Use open() instead of fopen() for less overhead
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        return NULL;
    }
    
    // Get file size using fstat (faster than seek/tell)
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
    
    // Pre-allocate exact buffer size
    char *buffer = malloc(file_size + 1);
    if (!buffer) {
        close(fd);
        return NULL;
    }
    
    // Read entire file with single read() syscall
    ssize_t bytes_read = read(fd, buffer, file_size);
    close(fd);
    
    if (bytes_read != (ssize_t)file_size) {
        free(buffer);
        return NULL;
    }
    
    // Null terminate
    buffer[file_size] = '\0';
    
    // Create Java string and clean up
    jstring result = (*env)->NewStringUTF(env, buffer);
    free(buffer);
    
    return result;
}

// Core implementation
static jstring read_file_native_impl(JNIEnv *env, jstring jpath) {
    if (!jpath) {
        return NULL;
    }
    
    const char *path = (*env)->GetStringUTFChars(env, jpath, NULL);
    if (!path) {
        return NULL;
    }
    
    jstring result = read_file_optimized(env, path);
    (*env)->ReleaseStringUTFChars(env, jpath, path);
    
    return result;
}

// JNI export matching your Smali code
JNIEXPORT jstring JNICALL
Java_luaj_lib_PackageLib_00024loadlib_00024FastIOFunction_readFileNative(JNIEnv *env, jclass clazz, jstring jpath) {
    return read_file_native_impl(env, jpath);
}

// Alternative export
JNIEXPORT jstring JNICALL
Java_FastIONative_readFileNative(JNIEnv *env, jclass clazz, jstring jpath) {
    return read_file_native_impl(env, jpath);
}
