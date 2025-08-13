#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// LuaJ compatible definitions
typedef struct lua_State lua_State;

// Essential Lua API functions that should be available in LuaJ
extern void lua_pushstring(lua_State *L, const char *s);
extern void lua_pushlstring(lua_State *L, const char *s, size_t len);
extern void lua_pushnil(lua_State *L);
extern void lua_pushinteger(lua_State *L, long long n);
extern const char *lua_tolstring(lua_State *L, int idx, size_t *len);
extern void lua_createtable(lua_State *L, int narr, int nrec);
extern void lua_setglobal(lua_State *L, const char *name);
extern void lua_setfield(lua_State *L, int idx, const char *k);
extern void lua_pushcfunction(lua_State *L, int (*f)(lua_State *L));

#define CHUNK_SIZE (1024 * 1024)

// The actual read function
static int lua_fastio_read_chunk(lua_State *L) {
    size_t len;
    const char *path = lua_tolstring(L, 1, &len);
    if (!path) {
        lua_pushnil(L);
        lua_pushstring(L, "expected string argument");
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
    buffer[total_read] = '\0';
    
    // Return just the content as string
    lua_pushlstring(L, buffer, total_read);
    free(buffer);
    
    return 1; // Return content
}

// Export function for package.loadlib
__attribute__((visibility("default"))) 
int fastio_read_chunk(lua_State *L) {
    return lua_fastio_read_chunk(L);
}

// Alternative: Register as global function when library loads
__attribute__((visibility("default"))) 
int luaopen_fastio(lua_State *L) {
    // Try to register the function globally
    lua_pushcfunction(L, lua_fastio_read_chunk);
    lua_setglobal(L, "fastio_read_chunk");
    
    // Also create a table with the function
    lua_createtable(L, 0, 1);
    lua_pushcfunction(L, lua_fastio_read_chunk);
    lua_setfield(L, -2, "read_chunk");
    
    return 1;
}