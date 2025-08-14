#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <jni.h>

#define CHUNK_SIZE (1024 * 1024)

// JNI function that can be called directly from Java/LuaJ
JNIEXPORT jstring JNICALL
Java_FastIONative_readFile(JNIEnv *env, jclass clazz, jstring jpath) {
    // Convert Java string to C string
    const char *path = (*env)->GetStringUTFChars(env, jpath, NULL);
    if (!path) {
        return NULL;
    }
    
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        (*env)->ReleaseStringUTFChars(env, jpath, path);
        return NULL;
    }
    
    // Get file size
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (file_size < 0) {
        fclose(fp);
        (*env)->ReleaseStringUTFChars(env, jpath, path);
        return NULL;
    }
    
    // Allocate buffer
    char *buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        fclose(fp);
        (*env)->ReleaseStringUTFChars(env, jpath, path);
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
                (*env)->ReleaseStringUTFChars(env, jpath, path);
                return NULL;
            }
            break;
        }
        total_read += nread;
    }
    
    fclose(fp);
    (*env)->ReleaseStringUTFChars(env, jpath, path);
    
    // Create Java string from buffer
    buffer[total_read] = '\0';
    jstring result = (*env)->NewStringUTF(env, buffer);
    free(buffer);
    
    return result;
}

// Alternative: Generic function callable with different signatures
JNIEXPORT jstring JNICALL
Java_org_luaj_vm2_lib_jse_JsePlatform_readFileNative(JNIEnv *env, jclass clazz, jstring jpath) {
    return Java_FastIONative_readFile(env, clazz, jpath);
}