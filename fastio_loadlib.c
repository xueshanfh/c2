#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Minimal Lua definitions for loadlib compatibility
typedef struct lua_State lua_State;

// External Lua functions (provided by host)
extern void lua_pushstring(lua_State *L, const char *s);
extern void lua_pushlstring(lua_State *L, const char *s, size_t len);
extern void lua_pushnil(lua_State *L);
extern void lua_pushinteger(lua_State *L, long long n);
extern const char *lua_tolstring(lua_State *L, int idx, size_t *len);
extern void lua_settop(lua_State *L, int idx);

#define CHUNK_SIZE (1024 * 1024)

// Function that works with package.loadlib
static int fastio_read_file_direct(lua_State *L) {
    size_t len;
    const char *path = lua_tolstring(L, 1, &len);
    if (!path) {
        lua_pushnil(L);
        lua_pushstring(L, "invalid path argument");
        return 2;
    }
    
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2;
    }
    
    // Get file size
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (file_size < 0) {
        fclose(fp);
        lua_pushnil(L);
        lua_pushstring(L, "cannot determine file size");
        return 2;
    }
    
    // Allocate buffer
    char *buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        fclose(fp);
        lua_pushnil(L);
        lua_pushstring(L, "memory allocation failed");
        return 2;
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
                lua_pushnil(L);
                lua_pushstring(L, "read error");
                return 2;
            }
            break;
        }
        total_read += nread;
    }
    
    fclose(fp);
    
    // Return content and size
    lua_pushlstring(L, buffer, total_read);
    lua_pushinteger(L, (long long)total_read);
    free(buffer);
    
    return 2; // content, size
}

// Export for package.loadlib - single function approach
__attribute__((visibility("default"))) 
int fastio_read_chunk(lua_State *L) {
    return fastio_read_file_direct(L);
}