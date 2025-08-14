
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <jni.h>

#define CHUNK_SIZE 65536

// JNI function for FastIOFunction.readFileNative called from your Smali patch
JNIEXPORT jstring JNICALL
Java_lluaj_lib_PackageLib_00024loadlib_00024FastIOFunction_readFileNative(JNIEnv *env, jclass clazz, jstring jpath) {
    // Convert Java string to C string
    const char *path = (*env)->GetStringUTFChars(env, jpath, NULL);
    if (!path) {
        return NULL;
    }
    
    // Open file with low-level system calls
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        (*env)->ReleaseStringUTFChars(env, jpath, path);
        return NULL;
    }
    
    // Get file size
    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        (*env)->ReleaseStringUTFChars(env, jpath, path);
        return NULL;
    }
    
    size_t file_size = (size_t)st.st_size;
    if (file_size == 0) {
        close(fd);
        (*env)->ReleaseStringUTFChars(env, jpath, path);
        return (*env)->NewStringUTF(env, "");
    }
    
    // Allocate buffer
    char *buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        close(fd);
        (*env)->ReleaseStringUTFChars(env, jpath, path);
        return NULL;
    }
    
    // Read file in chunks using low-level read()
    size_t total_read = 0;
    while (total_read < file_size) {
        size_t to_read = (file_size - total_read > CHUNK_SIZE) ? 
                        CHUNK_SIZE : (file_size - total_read);
        ssize_t nread = read(fd, buffer + total_read, to_read);
        if (nread <= 0) {
            if (nread == -1) {
                free(buffer);
                close(fd);
                (*env)->ReleaseStringUTFChars(env, jpath, path);
                return NULL;
            }
            break; // EOF
        }
        total_read += nread;
    }
    
    close(fd);
    (*env)->ReleaseStringUTFChars(env, jpath, path);
    
    // Create Java string from buffer
    buffer[total_read] = '\0';
    jstring result = (*env)->NewStringUTF(env, buffer);
    free(buffer);
    
    return result;
}

// Alternative JNI export for different class paths
JNIEXPORT jstring JNICALL
Java_FastIONative_readFileNative(JNIEnv *env, jclass clazz, jstring jpath) {
    return Java_lluaj_lib_PackageLib_00024loadlib_00024FastIOFunction_readFileNative(env, clazz, jpath);
}
