#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>

#define CHUNK_SIZE (1024 * 1024)

// ARM64 optimized file reading function
static int read_chunk(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        luaL_error(L, "cannot open %s: %s", path, strerror(errno));
    }

    char *buf = (char*)malloc(CHUNK_SIZE);
    if (!buf) {
        fclose(fp);
        luaL_error(L, "memory allocation failed");
    }
    
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    size_t nread;

    while ((nread = fread(buf, 1, CHUNK_SIZE, fp)) > 0) {
        luaL_addlstring(&b, buf, nread);
    }

    free(buf);
    
    if (ferror(fp)) {
        fclose(fp);
        luaL_error(L, "read error for %s", path);
    }
    fclose(fp);
    
    luaL_pushresult(&b);
    return 1;
}

static const luaL_Reg fastio_lib[] = {
    {"read_chunk", read_chunk},
    {NULL, NULL}
};

LUAMOD_API __attribute__((visibility("default"))) int luaopen_fastio(lua_State *L) {
    luaL_newlib(L, fastio_lib);
    return 1;
}
