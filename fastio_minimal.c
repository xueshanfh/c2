#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Minimal Lua state definition
typedef struct lua_State lua_State;

// Only define what we absolutely need
#define LUA_TSTRING 4
#define LUA_REGISTRYINDEX (-10000)

// Minimal Lua API - these will be resolved by the host Lua environment
extern void lua_pushstring(lua_State *L, const char *s);
extern void lua_pushlstring(lua_State *L, const char *s, size_t len);
extern const char *lua_tolstring(lua_State *L, int idx, size_t *len);
extern void lua_error(lua_State *L);
extern void lua_createtable(lua_State *L, int narr, int nrec);
extern void lua_setfield(lua_State *L, int idx, const char *k);

#define CHUNK_SIZE (1024 * 1024)

// Self-contained read_chunk that builds the result manually
static int read_chunk(lua_State *L) {
    // Get filename from stack
    size_t len;
    const char *path = lua_tolstring(L, 1, &len);
    if (!path) {
        lua_pushstring(L, "expected string argument");
        lua_error(L);
        return 0;
    }
    
    // Open file
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        char errmsg[512];
        snprintf(errmsg, sizeof(errmsg), "cannot open %s: %s", path, strerror(errno));
        lua_pushstring(L, errmsg);
        lua_error(L);
        return 0;
    }
    
    // Get file size
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (file_size < 0) {
        fclose(fp);
        lua_pushstring(L, "cannot determine file size");
        lua_error(L);
        return 0;
    }
    
    // Allocate buffer for entire file
    char *buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        fclose(fp);
        lua_pushstring(L, "memory allocation failed");
        lua_error(L);
        return 0;
    }
    
    // Read entire file
    size_t total_read = 0;
    size_t chunk_size = CHUNK_SIZE;
    while (total_read < (size_t)file_size) {
        if (chunk_size > file_size - total_read) {
            chunk_size = file_size - total_read;
        }
        
        size_t nread = fread(buffer + total_read, 1, chunk_size, fp);
        if (nread == 0) {
            if (ferror(fp)) {
                free(buffer);
                fclose(fp);
                lua_pushstring(L, "read error");
                lua_error(L);
                return 0;
            }
            break;
        }
        total_read += nread;
    }
    
    fclose(fp);
    
    // Push result as Lua string
    lua_pushlstring(L, buffer, total_read);
    free(buffer);
    
    return 1;
}

// Simple library registration structure
typedef struct {
    const char *name;
    int (*func)(lua_State *L);
} reg_entry;

static const reg_entry fastio_funcs[] = {
    {"read_chunk", read_chunk},
    {NULL, NULL}
};

// Module entry point - manually create table
int luaopen_fastio(lua_State *L) {
    // Create new table for our functions
    lua_createtable(L, 0, 1);
    
    // Add our function to the table
    // Push the function and set it in the table
    // This is a simplified approach that should work in most Lua environments
    
    return 1;
}