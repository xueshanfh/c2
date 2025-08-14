
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jni.h>

// Simple, fast file reading optimized for small-medium files
static jstring read_file_optimized(JNIEnv *env, const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0) {
        fclose(file);
        return (*env)->NewStringUTF(env, "");
    }
    
    // Allocate buffer for entire file + null terminator
    char *buffer = malloc(file_size + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }
    
    // Read entire file in one operation
    size_t bytes_read = fread(buffer, 1, file_size, file);
    fclose(file);
    
    if (bytes_read != (size_t)file_size) {
        free(buffer);
        return NULL;
    }
    
    // Null terminate
    buffer[file_size] = '\0';
    
    // Create Java string
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
